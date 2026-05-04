# (C) Copyright 2026, SECO Mind Srl
#
# SPDX-License-Identifier: Apache-2.0

import logging
import hashlib
import time

import uuid
from pathlib import Path
from datetime import datetime, timezone

from configuration import Configuration
from http_requests import http_post_server_data, http_get_server_data

interface_ft_capabilities = "io.edgehog.devicemanager.fileTransfer.Capabilities"
interface_ft_server_to_device = "io.edgehog.devicemanager.fileTransfer.ServerToDevice"
interface_ft_device_to_server = "io.edgehog.devicemanager.fileTransfer.DeviceToServer"
interface_ft_response = "io.edgehog.devicemanager.fileTransfer.Response"
interface_ft_progress = "io.edgehog.devicemanager.fileTransfer.Progress"

logger = logging.getLogger(__name__)

def is_file_transfer_enabled(dut, feature = "CONFIG_EDGEHOG_DEVICE_FILE_TRANSFER") -> bool:
    """Reads the Zephyr build config from the DUT to check if FT is enabled."""
    cfg_file = Path(dut.device_config.build_dir) / "zephyr" / ".config"

    with open(cfg_file, "r") as f:
        return f"{feature}=y" in f.read()

def _execute_and_wait_for_transfer(e2e_cfg: Configuration, interface: str, transfer_data: dict, timeout: int = 30):
    """
    Triggers a file transfer via POST and polls for BOTH a successful
    response code ('0') and at least one progress update.
    """
    start_time = datetime.now(timezone.utc)
    target_id = transfer_data["id"]

    http_post_server_data(e2e_cfg, interface, "/request", transfer_data)

    logger.info(f"Waiting up to {timeout}s for confirmation and progress (ID: {target_id})")
    start_polling = time.time()

    response_received = False
    progress_received = False

    while time.time() - start_polling < timeout:
        # 1. Poll for completion response
        if not response_received:
            ft_res = http_get_server_data(
                e2e_cfg, interface_ft_response, limit=5, since_after=start_time, quiet=True
            )
            if "request" in ft_res:
                for req in ft_res["request"]:
                    if req.get("id") == target_id:
                        assert req["code"] == '0', f"Transfer failed with code {req['code']}"
                        response_received = True
                        break

        # 2. Poll for progress update
        if not progress_received:
            ft_prog = http_get_server_data(
                e2e_cfg, interface_ft_progress, limit=5, since_after=start_time, quiet=True
            )
            if "request" in ft_prog:
                for req in ft_prog["request"]:
                    if req.get("id") == target_id:
                        progress_received = True
                        break

        # Break early if both conditions are met
        if response_received and progress_received:
            break

        time.sleep(2)

    assert response_received, f"No successful response received for transfer {target_id}"
    assert progress_received, f"No progress update received for transfer {target_id}"


def _run_full_transfer_cycle(e2e_cfg: Configuration, transfer_type: str, device_path: str):
    """
    Executes a complete Server -> Device -> Server test loop.
    Generates unique content, downloads to the device, uploads back to the server,
    and verifies the data integrity.
    """
    logger.info(f"Testing full file transfer loop for type: {transfer_type}")

    # Generate test data unique to the transfer type
    test_payload = f"Zephyr {transfer_type.capitalize()} Transfer Test Payload! " * 200
    test_bytes = test_payload.encode('utf-8')
    file_digest = f"sha256:{hashlib.sha256(test_bytes).hexdigest()}"

    download_filename = f"{transfer_type}_download.txt"
    upload_filename = f"{transfer_type}_upload.txt"

    dl_id = str(uuid.uuid4())
    ul_id = str(uuid.uuid4())

    # Write the initial file to the HTTP server's data directory
    download_file_path = Path(e2e_cfg.http_server_data_dir) / download_filename
    download_file_path.write_bytes(test_bytes)

    # --- PHASE 1: DOWNLOAD (Server to Device) ---
    dl_data = {
        "url": f"https://192.0.2.2:8443/{download_filename}",
        "id": dl_id,
        "progress": True,
        "digest": file_digest,
        "fileSizeBytes": len(test_bytes),
        "httpHeaderKeys": ["Content-Type"],
        "httpHeaderValues": ["text/plain"],
        "destinationType": transfer_type,
        "destination": device_path,
    }
    logger.info(f"Initiating {transfer_type} download...")
    _execute_and_wait_for_transfer(e2e_cfg, interface_ft_server_to_device, dl_data)

    # --- PHASE 2: UPLOAD (Device to Server) ---
    ul_data = {
        "url": f"https://192.0.2.2:8443/{upload_filename}",
        "id": ul_id,
        "progress": True,
        "httpHeaderKeys": ["Content-Type"],
        "httpHeaderValues": ["text/plain"],
        "sourceType": transfer_type,
        "source": device_path,
    }
    logger.info(f"Initiating {transfer_type} upload...")
    _execute_and_wait_for_transfer(e2e_cfg, interface_ft_device_to_server, ul_data)

    # --- PHASE 3: VERIFICATION ---
    uploaded_file_path = Path(e2e_cfg.http_server_data_dir) / upload_filename
    assert uploaded_file_path.exists(), "The device failed to upload the file back to the server"

    actual_payload = uploaded_file_path.read_bytes()
    assert actual_payload == test_bytes, f"Data corruption! Uploaded file does not match downloaded file for {transfer_type}."

    logger.info(f"Full transfer loop for '{transfer_type}' completed.")


def validate_file_transfer_capabilities(e2e_cfg: Configuration):
    logger.info("Testing file transfer: capabilities")

    ft_res = http_get_server_data(e2e_cfg, interface_ft_capabilities)

    assert (i in ft_res for i in ["encodings", "targets", "unixPermissions"]), "Wrong capabilities"
    assert ft_res.get("encodings") is None, "Wrong encodings"
    assert ft_res.get("targets") == ["streaming", "filesystem"], "Wrong targets"
    assert not ft_res.get("unixPermissions"), "Wrong unix permissions"

    logger.info("File transfer test (capabilities) completed successfully")


def validate_file_transfer_stream(e2e_cfg: Configuration):
    _run_full_transfer_cycle(e2e_cfg, transfer_type="stream", device_path="loopback")


def validate_file_transfer_filesystem(e2e_cfg: Configuration):
    _run_full_transfer_cycle(e2e_cfg, transfer_type="filesystem", device_path="/lfs1/test_fs_transfer.txt")
