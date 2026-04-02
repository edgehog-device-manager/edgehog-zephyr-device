# (C) Copyright 2026, SECO Mind Srl
#
# SPDX-License-Identifier: Apache-2.0

import time
import logging

from datetime import datetime, timezone

from configuration import Configuration
from http_requests import http_post_server_data, http_get_server_data

interface_ft_server_to_device = "io.edgehog.devicemanager.fileTransfer.posix.ServerToDevice"
interface_ft_response = "io.edgehog.devicemanager.fileTransfer.Response"
interface_ft_progress = "io.edgehog.devicemanager.fileTransfer.Progress"

logger = logging.getLogger(__name__)

def file_transfer_test(end_to_end_configuration: Configuration):
    ft_data = {
        "url": "https://192.0.2.2:8443/test_data.txt",
        "id": "550e8400-e29b-41d4-a716-446655440000",
        "progress": True,
        "fileSizeBytes": 10000,
        "httpHeaderKey": "Content-Type,foo",
        "httpHeaderValue": "application/json,bar",
    }

    start_time = datetime.now(timezone.utc)

    http_post_server_data(end_to_end_configuration, interface_ft_server_to_device, "/request", ft_data)

    timeout = 30
    start_polling = time.time()
    ft_res = {}

    # loop until a response is received or timeout is expired
    while time.time() - start_polling < timeout:
        ft_res = http_get_server_data(
            end_to_end_configuration,
            interface_ft_response,
            limit=1,
            since_after=start_time
        )

        # response received
        if "request" in ft_res and len(ft_res["request"]) > 0:
            break

        time.sleep(5)

    assert "request" in ft_res, "No response received"
    assert ft_res["request"][0]["code"] == '0'

    # at this point we should have also received at least a Progress data
    ft_res = http_get_server_data(
            end_to_end_configuration,
            interface_ft_progress,
            limit=1,
            since_after=start_time
        )

    time.sleep(1)

    assert "request" in ft_res, "No response received"
    assert ft_res["request"][0]["id"] == ft_data["id"]
