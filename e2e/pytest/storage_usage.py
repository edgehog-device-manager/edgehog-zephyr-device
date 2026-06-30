# (C) Copyright 2026, SECO Mind Srl
#
# SPDX-License-Identifier: Apache-2.0

"""
End-to-end tests for the StorageUsage feature.

The e2e device publishes storage telemetry on the
``io.edgehog.devicemanager.StorageUsage`` interface.

Two partitions are reported in this e2e setup:

  * ``storage`` - the NVS settings partition (published automatically by
    ``publish_edgehog_storage_usage()`` because ``CONFIG_SETTINGS_NVS=y``).
    It maps to the board's default ``storage_partition`` node (label
    "storage"), because no ``zephyr,settings-partition`` chosen entry is
    present in the overlays.
  * ``lfs1`` - the LittleFS filesystem mounted at ``/lfs1`` and explicitly
    registered in ``device_handler.c`` via ``storage_partitions``.

The Astarte AppEngine REST API returns data keyed by label:

  {
    "storage": [ { "totalBytes": <int>, "freeBytes": <int>, "timestamp": "..." } ],
    "lfs1":    [ { "totalBytes": <int>, "freeBytes": <int>, "timestamp": "..." } ]
  }
"""

import logging
import time
import datetime
import math

from configuration import Configuration
from http_requests import http_get_server_data, http_post_server_data, http_delete_server_data

logger = logging.getLogger(__name__)

interface_storage_usage = "io.edgehog.devicemanager.StorageUsage"
interface_tel_cfg = "io.edgehog.devicemanager.config.Telemetry"

# Labels the e2e device is expected to publish.
# - "storage": the NVS settings partition (auto-published by edgehog)
# - "lfs1":    the /lfs1 LittleFS partition (registered via storage_partitions)
EXPECTED_STORAGE_LABELS = {"storage", "lfs1"}


# ---------------------------------------------------------------------------
# Internal helpers
# ---------------------------------------------------------------------------


def _get_storage_data(e2e_cfg: Configuration, since_after=None, quiet: bool = False):
    """Fetch StorageUsage data from Astarte AppEngine.

    Returns the raw dict keyed by label, e.g.:
        {
            "storage": [ { "totalBytes": 16384, "freeBytes": 8000, ... } ],
            "lfs1":    [ { "totalBytes": 262144, "freeBytes": 200000, ... } ],
        }
    """
    # 1. Query the root interface first.
    # This safely returns {} (200 OK) if there is no data in the time window,
    # preventing 404 errors during the "silence" check.
    root_data = http_get_server_data(
        e2e_cfg,
        interface_storage_usage,
        since_after=since_after,
        quiet=quiet,
    )

    if not root_data:
        return {}

    result = {}

    # 2. For each label that actually has data, fetch the historical array
    for label in root_data.keys():
        interface_path = f"{interface_storage_usage}/{label}"
        path_data = http_get_server_data(
            e2e_cfg,
            interface_path,
            since_after=since_after,
            quiet=quiet,
        )

        if path_data:
            result[label] = path_data if isinstance(path_data, list) else [path_data]

    return result


def _assert_entry_is_sane(label: str, entry: dict):
    """Assert that a single storage entry has valid numeric values."""
    total = entry.get("totalBytes")
    free = entry.get("freeBytes")

    assert total is not None, f"'totalBytes' missing in storage entry for label '{label}'"
    assert free is not None, f"'freeBytes' missing in storage entry for label '{label}'"

    total = int(total)
    free = int(free)

    assert total > 0, f"'totalBytes' must be > 0 for label '{label}', got {total}"
    assert free >= 0, f"'freeBytes' must be >= 0 for label '{label}', got {free}"
    assert (
        free <= total
    ), f"'freeBytes' ({free}) must be <= 'totalBytes' ({total}) for label '{label}'"


# ---------------------------------------------------------------------------
# Public test functions
# ---------------------------------------------------------------------------


def validate_storage_usage_published(e2e_cfg: Configuration, since_after):
    """Verify that both expected storage labels are published after boot.

    The device must report:
      * ``storage`` - the internal NVS settings partition.
      * ``lfs1``    - the user-registered LittleFS partition.
    """
    logger.info("Validating that initial storage usage data was published")

    storage_data = _get_storage_data(e2e_cfg, since_after=since_after)

    assert storage_data, "No storage usage data received from the device"

    received_labels = set(storage_data.keys())
    missing_labels = EXPECTED_STORAGE_LABELS - received_labels
    assert not missing_labels, (
        f"Expected storage labels not found: {missing_labels}. "
        f"Received labels: {received_labels}"
    )

    for label in EXPECTED_STORAGE_LABELS:
        entries = storage_data[label]
        assert (
            isinstance(entries, list) and len(entries) >= 1
        ), f"Expected at least one entry for label '{label}', got: {entries}"

    logger.info(
        "Initial storage usage data validated - labels received: %s",
        sorted(received_labels),
    )


def validate_storage_usage_values(e2e_cfg: Configuration, since_after):
    """Check that the received storage entries carry sane numeric values.

    For every label:
      * ``totalBytes`` must be a positive integer.
      * ``freeBytes``  must be a non-negative integer <= ``totalBytes``.
    """
    logger.info("Validating storage usage value sanity")

    storage_data = _get_storage_data(e2e_cfg, since_after=since_after)

    assert storage_data, "No storage usage data received from the device"

    for label, entries in storage_data.items():
        assert (
            isinstance(entries, list) and len(entries) >= 1
        ), f"Expected at least one entry under label '{label}'"
        for entry in entries:
            _assert_entry_is_sane(label, entry)

    logger.info("All storage usage values are sane for labels: %s", sorted(storage_data.keys()))


def validate_storage_usage_telemetry_frequency(e2e_cfg: Configuration):
    """Enable periodic storage-usage telemetry and verify publication rate.

    Steps:
      1. Enable ``StorageUsage`` telemetry with a period of 10 s.
      2. Wait long enough to collect several samples.
      3. For the ``storage`` label, assert the number of samples equals
         ``floor(test_duration / period)``.
      4. Sanity-check the values of every sample for both labels.
      5. Disable telemetry and verify that no new data arrives.
      6. Clean up the telemetry configuration.
    """
    logger.info("Validating storage usage telemetry frequency")

    telemetry_period = 10  # seconds
    test_duration = 60 + (telemetry_period / 2)

    tel_cfg_period_path = f"/request/{interface_storage_usage}/periodSeconds"
    tel_cfg_enable_path = f"/request/{interface_storage_usage}/enable"

    logger.info("Enabling StorageUsage telemetry with period=%ds", telemetry_period)
    http_post_server_data(e2e_cfg, interface_tel_cfg, tel_cfg_period_path, telemetry_period)
    http_post_server_data(e2e_cfg, interface_tel_cfg, tel_cfg_enable_path, True)

    start_time = datetime.datetime.now(datetime.timezone.utc)

    logger.info("Waiting %ds to collect telemetry samples", test_duration)
    time.sleep(test_duration)

    storage_data = _get_storage_data(e2e_cfg, since_after=start_time)

    assert storage_data, "No storage usage data received during telemetry collection window"

    expected_count = math.floor(test_duration / telemetry_period)

    # Check the sample count on the NVS label, which is guaranteed to be present
    nvs_label = "storage"
    assert (
        nvs_label in storage_data
    ), f"Expected '{nvs_label}' label in telemetry data, got: {list(storage_data.keys())}"
    nvs_entries = storage_data[nvs_label]
    assert (
        len(nvs_entries) == expected_count
    ), f"Expected {expected_count} entries for label '{nvs_label}', got {len(nvs_entries)}"

    # Sanity-check values for all received labels
    for label, entries in storage_data.items():
        for entry in entries:
            _assert_entry_is_sane(label, entry)

    # Disable telemetry and verify silence
    logger.info("Disabling StorageUsage telemetry")
    http_post_server_data(e2e_cfg, interface_tel_cfg, tel_cfg_enable_path, False)

    silence_start = datetime.datetime.now(datetime.timezone.utc)

    logger.info("Waiting %ds to confirm no new data is published", telemetry_period * 3)
    time.sleep(telemetry_period * 3)

    storage_data_after = _get_storage_data(e2e_cfg, since_after=silence_start)
    assert storage_data_after == {}, (
        "Received unexpected storage usage data after telemetry was disabled: "
        f"{storage_data_after}"
    )

    logger.info("Storage usage telemetry frequency validation successful")

    # Cleanup telemetry configuration
    http_delete_server_data(e2e_cfg, interface_tel_cfg, tel_cfg_period_path)
    http_delete_server_data(e2e_cfg, interface_tel_cfg, tel_cfg_enable_path)


def validate_storage_usage_multiple_labels(e2e_cfg: Configuration, since_after):
    """Verify that both storage partitions are individually reported.

    The device must publish separate entries for both ``storage`` (NVS
    settings partition) and ``lfs1`` (LittleFS user partition).  Each
    entry must carry sane ``totalBytes`` / ``freeBytes`` values.
    """
    logger.info("Validating storage usage for both partition labels")

    storage_data = _get_storage_data(e2e_cfg, since_after=since_after)

    assert storage_data, "No storage usage data received from the device"

    received_labels = set(storage_data.keys())
    missing_labels = EXPECTED_STORAGE_LABELS - received_labels
    assert not missing_labels, (
        f"Missing expected storage labels: {missing_labels}. " f"Received: {received_labels}"
    )

    for label in EXPECTED_STORAGE_LABELS:
        entries = storage_data[label]
        assert isinstance(entries, list) and len(entries) >= 1, f"Label '{label}' has no entries"
        for entry in entries:
            _assert_entry_is_sane(label, entry)

    logger.info(
        "Multiple-label storage usage validation successful - labels: %s",
        sorted(received_labels),
    )
