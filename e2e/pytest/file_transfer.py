# (C) Copyright 2026, SECO Mind Srl
#
# SPDX-License-Identifier: Apache-2.0

import io
import logging
import hashlib
import time
import uuid
import tarfile
import lz4.frame
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


def is_file_transfer_enabled(dut, feature="CONFIG_EDGEHOG_DEVICE_FILE_TRANSFER") -> bool:
    """Reads the Zephyr build config from the DUT to check if FT is enabled."""
    cfg_file = Path(dut.device_config.build_dir) / "zephyr" / ".config"

    with open(cfg_file, "r") as f:
        return f"{feature}=y" in f.read()


def get_by_path(data, path):
    """Helper to traverse a dict using a string path like 'a/b/c'"""
    keys = path.split("/")
    for key in keys:
        if not isinstance(data, dict):
            return None
        data = data.get(key)
    return data


def _execute_and_wait_for_transfer(
    e2e_cfg: Configuration, interface: str, transfer_data: dict, timeout: int = 30
):
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
                        assert req["code"] == "0", f"Transfer failed with code {req['code']}"
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


def validate_file_transfer_capabilities(e2e_cfg: Configuration):
    logger.info("Testing file transfer: capabilities")

    ft_res = http_get_server_data(e2e_cfg, interface_ft_capabilities)

    expected_capabilities = {
        "deviceToServer": {"filesystem": {"encodings": ["tar"]}, "streaming": {"encodings": None}},
        "serverToDevice": {
            "filesystem": {"encodings": ["lz4", "tar"]},
            "streaming": {"encodings": ["lz4"]},
        },
        "transfer": {
            "deviceToServer": {"targets": ["streaming", "filesystem"]},
            "serverToDevice": {"targets": ["streaming", "filesystem"]},
            "unixPermissions": False,
        },
    }

    assert expected_capabilities == ft_res, "Wrong capabilities"

    logger.info("File transfer test (capabilities) completed successfully")


# --- RAW FILE TRANSFER TESTS ---


def _run_raw_transfer_cycle(e2e_cfg: Configuration, transfer_type: str, device_path: str):
    logger.info(f"Testing raw file transfer loop for type: '{transfer_type}'")

    test_payload = f"Zephyr Raw {transfer_type.capitalize()} Transfer Test Payload! " * 200
    raw_bytes = test_payload.encode("utf-8")

    download_filename = f"{transfer_type}_raw_download.txt"
    upload_filename = f"{transfer_type}_raw_upload.txt"
    file_digest = f"sha256:{hashlib.sha256(raw_bytes).hexdigest()}"

    Path(e2e_cfg.http_server_data_dir, download_filename).write_bytes(raw_bytes)

    # 1. Download
    dl_data = {
        "url": f"https://192.0.2.2:8443/{download_filename}",
        "id": str(uuid.uuid7()),
        "progress": True,
        "digest": file_digest,
        "fileSizeBytes": len(raw_bytes),
        "httpHeaderKeys": ["Content-Type"],
        "httpHeaderValues": ["text/plain"],
        "destinationType": transfer_type,
        "destination": device_path,
    }
    logger.info(f"Initiating raw {transfer_type} download...")
    _execute_and_wait_for_transfer(e2e_cfg, interface_ft_server_to_device, dl_data)

    # 2. Upload
    ul_data = {
        "url": f"https://192.0.2.2:8443/{upload_filename}",
        "id": str(uuid.uuid7()),
        "progress": True,
        "httpHeaderKeys": ["Content-Type"],
        "httpHeaderValues": ["text/plain"],
        "sourceType": transfer_type,
        "source": device_path,
    }
    logger.info(f"Initiating raw {transfer_type} upload...")
    _execute_and_wait_for_transfer(e2e_cfg, interface_ft_device_to_server, ul_data)

    # 3. Verification
    uploaded_file_path = Path(e2e_cfg.http_server_data_dir, upload_filename)
    assert (
        uploaded_file_path.exists()
    ), "The device failed to upload the raw file back to the server"
    assert (
        uploaded_file_path.read_bytes() == raw_bytes
    ), f"Data corruption! Uploaded raw file does not match."
    logger.info(f"Raw transfer loop for '{transfer_type}' completed.")


def validate_file_transfer_stream(e2e_cfg: Configuration):
    _run_raw_transfer_cycle(e2e_cfg, transfer_type="stream", device_path="loopback")


def validate_file_transfer_filesystem(e2e_cfg: Configuration):
    _run_raw_transfer_cycle(
        e2e_cfg, transfer_type="filesystem", device_path="/lfs1/test_fs_transfer.txt"
    )


# --- LZ4 TRANSFER TESTS ---


def _run_lz4_transfer_cycle(e2e_cfg: Configuration, transfer_type: str, device_path: str):
    """
    Downloads LZ4 compressed data and uploads uncompressed data to ensure correct handling on the device.
    """
    logger.info(f"Testing LZ4 file transfer loop for type: '{transfer_type}'")

    raw_bytes = (f"Zephyr LZ4 {transfer_type.capitalize()} Transfer Test Payload! " * 200).encode(
        "utf-8"
    )
    lz4_bytes = lz4.frame.compress(raw_bytes)

    download_filename = f"{transfer_type}_lz4_download.lz4"
    upload_filename = f"{transfer_type}_lz4_upload.txt"

    # Device computes digest on the UNCOMPRESSED stream
    file_digest = f"sha256:{hashlib.sha256(raw_bytes).hexdigest()}"
    Path(e2e_cfg.http_server_data_dir, download_filename).write_bytes(lz4_bytes)

    # 1. Download Compressed
    dl_data = {
        "url": f"https://192.0.2.2:8443/{download_filename}",
        "id": str(uuid.uuid7()),
        "progress": True,
        "digest": file_digest,
        "fileSizeBytes": len(raw_bytes),
        "httpHeaderKeys": ["Content-Type"],
        "httpHeaderValues": ["application/octet-stream"],
        "destinationType": transfer_type,
        "destination": device_path,
        "encoding": "lz4",
    }
    logger.info(f"Initiating LZ4 {transfer_type} download...")
    _execute_and_wait_for_transfer(e2e_cfg, interface_ft_server_to_device, dl_data)

    # 2. Upload Uncompressed
    ul_data = {
        "url": f"https://192.0.2.2:8443/{upload_filename}",
        "id": str(uuid.uuid7()),
        "progress": True,
        "httpHeaderKeys": ["Content-Type"],
        "httpHeaderValues": ["text/plain"],
        "sourceType": transfer_type,
        "source": device_path,
        # Intentionally omitting encoding to test uncompressed retrieval
    }
    logger.info(f"Initiating uncompressed {transfer_type} upload...")
    _execute_and_wait_for_transfer(e2e_cfg, interface_ft_device_to_server, ul_data)

    # 3. Verification
    uploaded_file_path = Path(e2e_cfg.http_server_data_dir, upload_filename)
    assert uploaded_file_path.exists(), "The device failed to upload the uncompressed file."
    assert (
        uploaded_file_path.read_bytes() == raw_bytes
    ), "Data mismatch! Retrieved uncompressed file does not match original."
    logger.info(f"LZ4 transfer loop for '{transfer_type}' completed.")


def validate_file_transfer_stream_lz4(e2e_cfg: Configuration):
    _run_lz4_transfer_cycle(e2e_cfg, transfer_type="stream", device_path="loopback")


def validate_file_transfer_filesystem_lz4(e2e_cfg: Configuration):
    _run_lz4_transfer_cycle(
        e2e_cfg, transfer_type="filesystem", device_path="/lfs1/test_fs_transfer_lz4.txt"
    )


# --- TAR TRANSFER TESTS ---


def validate_file_transfer_filesystem_tar(e2e_cfg: Configuration):
    logger.info("Testing TAR file transfer loop with 3 files (including a large file)")

    # USTAR blocks are 512 bytes. Making file1 sufficiently large (~14KB) to exceed the frame size.
    large_payload = (
        b"This is a large file payload intended to exceed standard USTAR frame sizes. " * 200
    )

    files = {
        "file1.txt": large_payload,
        "file2.txt": b"Content for the second TAR file test",
        "file3.txt": b"Content for the third TAR file test",
    }

    tar_stream = io.BytesIO()
    with tarfile.open(fileobj=tar_stream, mode="w", format=tarfile.USTAR_FORMAT) as tar:
        for name, content in files.items():
            tarinfo = tarfile.TarInfo(name=name)
            tarinfo.size = len(content)
            tar.addfile(tarinfo, io.BytesIO(content))
    tar_bytes = tar_stream.getvalue()

    download_filename = "tar_download.tar"
    upload_filename = "tar_upload.tar"
    file_digest = f"sha256:{hashlib.sha256(tar_bytes).hexdigest()}"

    Path(e2e_cfg.http_server_data_dir, download_filename).write_bytes(tar_bytes)

    # 1. Download TAR
    dl_data = {
        "url": f"https://192.0.2.2:8443/{download_filename}",
        "id": str(uuid.uuid7()),
        "progress": True,
        "digest": file_digest,
        "fileSizeBytes": len(tar_bytes),
        "httpHeaderKeys": ["Content-Type"],
        "httpHeaderValues": ["application/octet-stream"],
        "destinationType": "filesystem",
        "destination": "/lfs1/tar_test_dir",
        "encoding": "tar",
    }
    logger.info("Initiating TAR folder download...")
    _execute_and_wait_for_transfer(e2e_cfg, interface_ft_server_to_device, dl_data)

    # 2. Upload TAR
    ul_data = {
        "url": f"https://192.0.2.2:8443/{upload_filename}",
        "id": str(uuid.uuid7()),
        "progress": True,
        "httpHeaderKeys": ["Content-Type"],
        "httpHeaderValues": ["application/octet-stream"],
        "sourceType": "filesystem",
        "source": "/lfs1/tar_test_dir",
        "encoding": "tar",
    }
    logger.info("Initiating TAR folder upload...")
    _execute_and_wait_for_transfer(e2e_cfg, interface_ft_device_to_server, ul_data)

    # 3. Verification (Server extracts incoming TARs via http_server.py)
    for name, content in files.items():
        extracted_path = Path(e2e_cfg.http_server_data_dir, name)
        assert extracted_path.exists(), f"Extracted file {name} is missing from the server."
        assert extracted_path.read_bytes() == content, f"Content mismatch in extracted file {name}."
    logger.info("TAR transfer loop completed.")


def validate_file_transfer_filesystem_tar_nested(e2e_cfg: Configuration):
    logger.info("Testing TAR file transfer loop with nested directories")

    # Creating a mix of root-level and nested files
    files = {
        "root.txt": b"Root content",
        "folder_a/file_a.txt": b"Content A",
        "folder_a/folder_b/folder_c/file_b.txt": b"Content B",
    }

    tar_stream = io.BytesIO()
    with tarfile.open(fileobj=tar_stream, mode="w", format=tarfile.USTAR_FORMAT) as tar:
        for name, content in files.items():
            tarinfo = tarfile.TarInfo(name=name)
            tarinfo.size = len(content)
            tar.addfile(tarinfo, io.BytesIO(content))
    tar_bytes = tar_stream.getvalue()

    download_filename = "nested_tar_download.tar"
    upload_filename = "nested_tar_upload.tar"
    file_digest = f"sha256:{hashlib.sha256(tar_bytes).hexdigest()}"

    Path(e2e_cfg.http_server_data_dir, download_filename).write_bytes(tar_bytes)

    # 1. Download
    dl_data = {
        "url": f"https://192.0.2.2:8443/{download_filename}",
        "id": str(uuid.uuid7()),
        "progress": True,
        "digest": file_digest,
        "fileSizeBytes": len(tar_bytes),
        "httpHeaderKeys": ["Content-Type"],
        "httpHeaderValues": ["application/octet-stream"],
        "destinationType": "filesystem",
        "destination": "/lfs1/nested_tar_test",
        "encoding": "tar",
    }
    logger.info("Initiating Nested TAR download...")
    _execute_and_wait_for_transfer(e2e_cfg, interface_ft_server_to_device, dl_data)

    # 2. Upload
    ul_data = {
        "url": f"https://192.0.2.2:8443/{upload_filename}",
        "id": str(uuid.uuid7()),
        "progress": True,
        "httpHeaderKeys": ["Content-Type"],
        "httpHeaderValues": ["application/octet-stream"],
        "sourceType": "filesystem",
        "source": "/lfs1/nested_tar_test",
        "encoding": "tar",
    }
    logger.info("Initiating Nested TAR upload...")
    _execute_and_wait_for_transfer(e2e_cfg, interface_ft_device_to_server, ul_data)

    # 3. Verification
    for name, content in files.items():
        extracted_path = Path(e2e_cfg.http_server_data_dir, name)
        assert extracted_path.exists(), f"Extracted nested file {name} is missing."
        assert extracted_path.read_bytes() == content, f"Content mismatch in nested file {name}."
    logger.info("Nested TAR transfer loop completed.")


def validate_file_transfer_filesystem_tar_empty(e2e_cfg: Configuration):
    logger.info("Testing empty TAR file transfer loop")

    tar_stream = io.BytesIO()
    with tarfile.open(fileobj=tar_stream, mode="w", format=tarfile.USTAR_FORMAT) as tar:
        pass  # Create an empty TAR structure
    tar_bytes = tar_stream.getvalue()

    download_filename = "empty_tar_download.tar"
    upload_filename = "empty_tar_upload.tar"
    file_digest = f"sha256:{hashlib.sha256(tar_bytes).hexdigest()}"

    Path(e2e_cfg.http_server_data_dir, download_filename).write_bytes(tar_bytes)

    # 1. Download Empty TAR
    dl_data = {
        "url": f"https://192.0.2.2:8443/{download_filename}",
        "id": str(uuid.uuid7()),
        "progress": True,
        "digest": file_digest,
        "fileSizeBytes": len(tar_bytes),
        "httpHeaderKeys": ["Content-Type"],
        "httpHeaderValues": ["application/octet-stream"],
        "destinationType": "filesystem",
        "destination": "/lfs1/empty_tar_dir",
        "encoding": "tar",
    }
    logger.info("Initiating Empty TAR folder download...")
    _execute_and_wait_for_transfer(e2e_cfg, interface_ft_server_to_device, dl_data)

    # 2. Upload Empty TAR
    ul_data = {
        "url": f"https://192.0.2.2:8443/{upload_filename}",
        "id": str(uuid.uuid7()),
        "progress": True,
        "httpHeaderKeys": ["Content-Type"],
        "httpHeaderValues": ["application/octet-stream"],
        "sourceType": "filesystem",
        "source": "/lfs1/empty_tar_dir",
        "encoding": "tar",
    }
    logger.info("Initiating Empty TAR folder upload...")
    _execute_and_wait_for_transfer(e2e_cfg, interface_ft_device_to_server, ul_data)

    # 3. Verification
    uploaded_path = Path(e2e_cfg.http_server_data_dir, upload_filename)
    assert uploaded_path.exists(), "The empty TAR file was not uploaded to the server."
    assert tarfile.is_tarfile(uploaded_path), "Uploaded file is not a valid TAR."
    logger.info("Empty TAR transfer loop completed.")
