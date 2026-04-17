# (C) Copyright 2026, SECO Mind Srl
#
# SPDX-License-Identifier: Apache-2.0

import logging
import time
import datetime
import math

from http_requests import http_get_server_data, http_post_server_data, http_delete_server_data

logger = logging.getLogger(__name__)

interface_hwinfo = "io.edgehog.devicemanager.HardwareInfo"
interface_sysstat = "io.edgehog.devicemanager.SystemStatus"
interface_storage_usage = "io.edgehog.devicemanager.StorageUsage"
interface_tel_cfg = "io.edgehog.devicemanager.config.Telemetry"

def validate_initial_telemetry(e2e_cfg, since_after):

    logger.info(f"Validating initial telemetry since {since_after.isoformat()}")

    hw_info = http_get_server_data(e2e_cfg, interface_hwinfo, since_after=since_after)
    assert hw_info, "Failed to retrieve hardware info telemetry"
    hw_info_cpu = hw_info.get("cpu", None)
    assert hw_info_cpu, "CPU information missing in hardware info telemetry"
    assert hw_info_cpu.get("architecture", None), "CPU architecture information missing in hardware info telemetry"
    assert hw_info_cpu.get("modelName", None), "CPU model name information missing in hardware info telemetry"

    sysstat_info = http_get_server_data(e2e_cfg, interface_sysstat, since_after=since_after)
    assert sysstat_info, "Failed to retrieve system status telemetry"
    sysstat_info = sysstat_info.get("systemStatus", None)
    assert sysstat_info, "System status information missing in system status telemetry"
    assert len(sysstat_info) == 1, "Unexpected number of system status entries in system status telemetry"
    sysstat_info = sysstat_info[0]
    assert sysstat_info.get("availMemoryBytes", None), "Available memory information missing in system status telemetry"
    assert sysstat_info.get("bootId", None), "System status boot ID information missing in system status telemetry"
    assert sysstat_info.get("taskCount", None), "System status task count information missing in system status telemetry"
    assert sysstat_info.get("uptimeMillis", None), "System status uptime information missing in system status telemetry"

    storage_info = http_get_server_data(e2e_cfg, interface_storage_usage, since_after=since_after)
    assert storage_info, "Failed to retrieve storage usage telemetry"
    storage_info = storage_info.get("storage", None)
    assert storage_info, "Storage information missing in storage usage telemetry"
    assert len(storage_info) == 1, "Unexpected number of storage entries in storage usage telemetry"
    storage_info = storage_info[0]
    assert storage_info.get("freeBytes", None), "Free memory information missing in storage usage telemetry"
    assert storage_info.get("totalBytes", None), "Total memory information missing in storage usage telemetry"

    logger.info(f"Initial telemetry validation successful")

def validate_telemetry_frequency(e2e_cfg):

    logger.info("Validating telemetry frequency")

    telemetry_period = 10
    test_duration = 60 + (telemetry_period / 10)

    logger.info(f"Enabling telemetry on interface {interface_storage_usage} with a period of {telemetry_period} seconds")

    tel_cfg_period_path = f"/request/{interface_storage_usage}/periodSeconds"
    http_post_server_data(e2e_cfg, interface_tel_cfg, tel_cfg_period_path, telemetry_period)
    tel_cfg_enable_path = f"/request/{interface_storage_usage}/enable"
    http_post_server_data(e2e_cfg, interface_tel_cfg, tel_cfg_enable_path, True)

    start_time = datetime.datetime.now(datetime.timezone.utc)

    # Wait for a while so the device can send the telemetry a few times
    logger.info(f"Waiting for {test_duration} seconds to collect telemetry data")
    time.sleep(test_duration)

    logger.info(f"Getting telemetry data since {start_time.isoformat()}")
    storage_info = http_get_server_data(e2e_cfg, interface_storage_usage, since_after=start_time)
    assert storage_info, "Failed to retrieve storage usage telemetry"
    storage_info = storage_info.get("storage", None)
    assert storage_info, "Storage information missing in storage usage telemetry"
    assert len(storage_info) == math.floor(test_duration/telemetry_period), "Unexpected number of storage entries in storage usage telemetry"

    # Now disable the telemetry and check that no new data is received after a while
    logger.info(f"Disabling telemetry on interface {interface_storage_usage}")

    tel_cfg_enable_path = f"/request/{interface_storage_usage}/enable"
    http_post_server_data(e2e_cfg, interface_tel_cfg, tel_cfg_enable_path, False)

    start_time = datetime.datetime.now(datetime.timezone.utc)

    logger.info(f"Waiting for {telemetry_period*3} seconds to check that no new telemetry data is received")
    time.sleep(telemetry_period*3)

    storage_info = http_get_server_data(e2e_cfg, interface_storage_usage, since_after=start_time)
    assert storage_info == {}, "Failed to retrieve storage usage telemetry"

    logger.info("Telemetry frequency validation successful")

    http_delete_server_data(e2e_cfg, interface_tel_cfg, tel_cfg_period_path)
    http_delete_server_data(e2e_cfg, interface_tel_cfg, tel_cfg_enable_path)
