# (C) Copyright 2024, SECO Mind Srl

# SPDX-License-Identifier: Apache-2.0

import time
import logging
import datetime
import pytest

from configuration import Configuration
from http_server import start_server, stop_server
from telemetry import validate_initial_telemetry, validate_telemetry_frequency
from file_transfer import (
    is_file_transfer_enabled, validate_file_transfer_stream, validate_file_transfer_capabilities,
    validate_file_transfer_filesystem, validate_file_transfer_stream_lz4, validate_file_transfer_filesystem_lz4
    )

logger = logging.getLogger(__name__)
logging.getLogger("urllib3").setLevel(logging.INFO)
logging.getLogger("requests").setLevel(logging.INFO)

SHELL_IS_READY = "dvcshellcmd Device shell ready$"
SHELL_IS_CLOSING = "dvcshellcmd Device shell closing$"
SHELL_CMD_DISCONNECT = "dvcshellcmd_disconnect"

@pytest.fixture(scope="function")
def e2e_device_env(end_to_end_configuration: Configuration):

    initial_time = datetime.datetime.now(datetime.timezone.utc)

    logger.info("Starting the http server")
    start_server(port=end_to_end_configuration.http_server_port,
                 cert_file=end_to_end_configuration.http_server_cert,
                 key_file=end_to_end_configuration.http_server_key,
                 data_dir=end_to_end_configuration.http_server_data_dir)

    logger.info("Launching the device")
    end_to_end_configuration.dut.launch()
    end_to_end_configuration.dut.readlines_until(SHELL_IS_READY, timeout=60)
    time.sleep(1)

    yield end_to_end_configuration, initial_time

    logger.info("Dumping final device logs and tearing down")

    # Read any remaining logs to ensure they are captured by Pytest
    logs = end_to_end_configuration.dut.readlines()
    for line in logs:
        logger.info(f"DEVICE LOG: {line.strip()}")

    # Safely disconnect
    end_to_end_configuration.shell.exec_command(SHELL_CMD_DISCONNECT)
    end_to_end_configuration.dut.readlines_until(SHELL_IS_CLOSING, timeout=60)

    logger.info("Stopping the http server")
    stop_server()

@pytest.mark.default
def test_telemetry(e2e_device_env):
    cfg, initial_time = e2e_device_env
    validate_initial_telemetry(cfg, initial_time)
    validate_telemetry_frequency(cfg)
    time.sleep(1)

@pytest.mark.file_transfer
def test_file_transfer(e2e_device_env):
    cfg, _ = e2e_device_env

    if not is_file_transfer_enabled(cfg.dut):
        pytest.skip("CONFIG_EDGEHOG_DEVICE_FILE_TRANSFER is not enabled in the Zephyr build")

    validate_file_transfer_capabilities(cfg)
    validate_file_transfer_stream(cfg)
    validate_file_transfer_filesystem(cfg)
    validate_file_transfer_stream_lz4(cfg)
    validate_file_transfer_filesystem_lz4(cfg)

    time.sleep(1)
