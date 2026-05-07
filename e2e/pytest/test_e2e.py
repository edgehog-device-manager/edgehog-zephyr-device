# (C) Copyright 2024, SECO Mind Srl
#
# SPDX-License-Identifier: Apache-2.0

import time
import logging
import datetime

from configuration import Configuration
from http_server import start_server, stop_server
from telemetry import validate_initial_telemetry, validate_telemetry_frequency

logger = logging.getLogger(__name__)

SHELL_IS_READY = "dvcshellcmd Device shell ready$"
SHELL_IS_CLOSING = "dvcshellcmd Device shell closing$"
SHELL_CMD_DISCONNECT = "dvcshellcmd_disconnect"


def test_device(end_to_end_configuration: Configuration):

    initial_time = datetime.datetime.now(datetime.timezone.utc)

    logger.info("Starting the http server")

    # Start the local https server in the background
    start_server(port=end_to_end_configuration.http_server_port,
                 cert_file=end_to_end_configuration.http_server_cert,
                 key_file=end_to_end_configuration.http_server_key,
                 data_dir=end_to_end_configuration.http_server_data_dir)

    logger.info("Launching the device")

    end_to_end_configuration.dut.launch()
    end_to_end_configuration.dut.readlines_until(regex=SHELL_IS_READY, timeout=60)

    # Wait a couple of seconds
    time.sleep(10)

    # TODO: Perform the end to end tests
    validate_initial_telemetry(end_to_end_configuration, initial_time)
    validate_telemetry_frequency(end_to_end_configuration)

    # Wait a couple of seconds
    time.sleep(1)

    end_to_end_configuration.dut.readlines()
    end_to_end_configuration.shell.exec_command(SHELL_CMD_DISCONNECT)
    end_to_end_configuration.dut.readlines_until(regex=SHELL_IS_CLOSING, timeout=60)

    # Stop the local https server
    logger.info("Stopping the http server")
    stop_server()
