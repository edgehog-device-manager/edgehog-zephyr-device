# (C) Copyright 2026, SECO Mind Srl
#
# SPDX-License-Identifier: Apache-2.0

import time
import logging
from pathlib import Path

from datetime import datetime, timezone

from configuration import Configuration
from http_requests import http_post_server_data, http_get_server_data

interface_ft_server_to_device = "io.edgehog.devicemanager.fileTransfer.ServerToDevice"
interface_ft_device_to_server = "io.edgehog.devicemanager.fileTransfer.DeviceToServer"
interface_ft_response = "io.edgehog.devicemanager.fileTransfer.Response"
interface_ft_progress = "io.edgehog.devicemanager.fileTransfer.Progress"

logger = logging.getLogger(__name__)

def validate_file_transfer_server_to_device(e2e_cfg: Configuration):

    logger.info("Testing file transfer: server to device")

    transfer_id = "770e8400-e29b-41d4-a716-446655441111"
    destination_filename = "server_to_device_test_data.txt"

    ft_data = {
        "url": f"https://192.0.2.2:8443/{destination_filename}",
        "id": transfer_id,
        "progress": True,
        "fileSizeBytes": 10000,
        "httpHeaderKeys": ["Content-Type", "foo"],
        "httpHeaderValues": ["application/json", "bar"],
        "destinationType": "storage",
        "destination": "",
    }
    start_time = datetime.now(timezone.utc)
    http_post_server_data(e2e_cfg, interface_ft_server_to_device, "/request", ft_data)

    timeout = 30
    logger.info(f"Waiting {timeout} seconds for file transfer response from device")

    start_polling = time.time()
    ft_res = {}

    # loop until a response is received or timeout is expired
    while time.time() - start_polling < timeout:
        ft_res = http_get_server_data(
            e2e_cfg,
            interface_ft_response,
            limit=1,
            since_after=start_time,
            quiet=True
        )

        # Check if a response is received, if not wait for a few seconds and poll again
        if "request" in ft_res and len(ft_res["request"]) > 0:
            break

        time.sleep(5)

    assert "request" in ft_res, "No response received"
    assert ft_res["request"][0]["code"] == '0', "Response received with error code"

    # at this point we should have also received at least a Progress data
    ft_res = http_get_server_data(
            e2e_cfg,
            interface_ft_progress,
            limit=1,
            since_after=start_time
        )

    time.sleep(1)

    assert "request" in ft_res, "No progress received"
    assert ft_res["request"][0]["id"] == ft_data["id"], "Progress received for a different ID"

    logger.info("File transfer test (server to device) completed successfully")

def validate_file_transfer_device_to_server(e2e_cfg: Configuration):

    logger.info("Testing file transfer: device to server")

    transfer_id = "770e8400-e29b-41d4-a716-446655441111"
    destination_filename = "device_to_server_test_data.txt"

    ft_data = {
        "url": f"https://192.0.2.2:8443/{destination_filename}",
        "id": transfer_id,
        "progress": True,
        "httpHeaderKeys": ["Content-Type"],
        "httpHeaderValues": ["text/plain"],
        "sourceType": "storage",
        "source": "dummy_source_path",
    }

    start_time = datetime.now(timezone.utc)

    # Trigger the Device-to-Server upload via Astarte
    logger.info(f"Triggering upload for ID: {transfer_id}")
    http_post_server_data(e2e_cfg, interface_ft_device_to_server, "/request", ft_data)

    # Poll for the Response object from the device
    timeout = 30
    logger.info(f"Waiting {timeout} seconds for file transfer response from device")

    start_polling = time.time()
    ft_res = {}

    while time.time() - start_polling < timeout:
        ft_res = http_get_server_data(
            e2e_cfg,
            interface_ft_response,
            limit=1,
            since_after=start_time,
            quiet=True
        )

        if "request" in ft_res and len(ft_res["request"]) > 0:
            # Ensure we are looking at the response for our specific upload ID
            if ft_res["request"][0]["id"] == transfer_id:
                break

        time.sleep(2)

    # Assertions on Astarte Response
    assert "request" in ft_res, "No response received from device"
    assert ft_res["request"][0]["code"] == '0', f"Upload failed with code {ft_res['request'][0]['code']}"
    assert ft_res["request"][0]["type"] == "device_to_server"

    # Verify the file actually exists on the HTTP Server's data directory
    # The http_server.py saves files relative to 'data_dir'
    uploaded_file_path = Path(e2e_cfg.http_server_data_dir) / destination_filename

    assert uploaded_file_path.exists(), f"Uploaded file {destination_filename} not found on server"
    assert uploaded_file_path.stat().st_size > 0, "Uploaded file is empty"

    # At this point we should have also received at least a Progress data
    ft_res = http_get_server_data(
            e2e_cfg,
            interface_ft_progress,
            limit=1,
            since_after=start_time
        )

    assert "request" in ft_res, "No response received"
    assert ft_res["request"][0]["id"] == ft_data["id"]

    logger.info("File transfer test (device to server) completed successfully")

def _wait_for_astarte_response(e2e_cfg, start_time, transfer_id, timeout=30):
    logger.info(f"Waiting {timeout} seconds for file transfer response from device")
    start_polling = time.time()
    ft_res = {}

    while time.time() - start_polling < timeout:
        ft_res = http_get_server_data(
            e2e_cfg,
            interface_ft_response,
            limit=1,
            since_after=start_time,
            quiet=True
        )

        if "request" in ft_res and len(ft_res["request"]) > 0:
            if ft_res["request"][0]["id"] == transfer_id:
                break

        time.sleep(2)

    assert "request" in ft_res, "No response received from device"
    assert ft_res["request"][0]["code"] == '0', f"Transfer failed with code {ft_res['request'][0]['code']}"
    return ft_res

def validate_file_transfer_loopback(e2e_cfg: Configuration):
    logger.info("Testing stream loopback: Server -> Device -> Server")

    # Generate roughly 8KB of test data (fits well within the 16KB pipe)
    test_payload = "Zephyr Infinite RAM Loopback Test Payload! " * 200
    download_filename = "loopback_download.txt"
    upload_filename = "loopback_upload.txt"

    download_file_path = Path(e2e_cfg.http_server_data_dir) / download_filename
    download_file_path.write_text(test_payload)

    # ---------------------------------------------------------
    # PHASE 1: DOWNLOAD (Server to Device)
    # ---------------------------------------------------------
    dl_data = {
        "url": f"https://192.0.2.2:8443/{download_filename}",
        "id": "550e8400-e29b-41d4-a716-446655440000",
        "progress": True,
        "fileSizeBytes": len(test_payload),
        "httpHeaderKeys": ["Content-Type"],
        "httpHeaderValues": ["text/plain"],
        "destinationType": "stream",
        "destination": "loopback",
    }

    dl_start_time = datetime.now(timezone.utc)
    http_post_server_data(e2e_cfg, interface_ft_server_to_device, "/request", dl_data)

    _wait_for_astarte_response(e2e_cfg, dl_start_time, dl_data["id"])
    logger.info("Download to device RAM complete. Initiating upload...")

    # ---------------------------------------------------------
    # PHASE 2: UPLOAD (Device to Server)
    # ---------------------------------------------------------
    ul_data = {
        "url": f"https://192.0.2.2:8443/{upload_filename}",
        "id": "770e8400-e29b-41d4-a716-446655441111",
        "progress": True,
        "httpHeaderKeys": ["Content-Type"],
        "httpHeaderValues": ["text/plain"],
        "sourceType": "stream",
        "source": "loopback",
    }

    ul_start_time = datetime.now(timezone.utc)
    http_post_server_data(e2e_cfg, interface_ft_device_to_server, "/request", ul_data)

    _wait_for_astarte_response(e2e_cfg, ul_start_time, ul_data["id"])

    # ---------------------------------------------------------
    # PHASE 3: VERIFICATION
    # ---------------------------------------------------------
    uploaded_file_path = Path(e2e_cfg.http_server_data_dir) / upload_filename
    assert uploaded_file_path.exists(), "The device failed to upload the file back to the server"

    actual_payload = uploaded_file_path.read_text()
    assert actual_payload == test_payload, "Data corruption! The uploaded file does not match the downloaded file."

    logger.info("Loopback test completed perfectly. Data integrity verified.")
