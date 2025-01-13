/*
 * (C) Copyright 2024, SECO Mind Srl
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * @file edgehog-zephyr-device/tests/lib/edgehog_device_sdk/unit/src/main.c
 *
 * @details This test suite verifies that the methods provided with the edgehog_device
 * module works correctly.
 *
 * @note This should be run with the latest version of zephyr present on master (or 3.6.0)
 */

#include <zephyr/ztest.h>

#include <edgehog_device/device.h>

// Define a minimal_log function to resolve the `undefined reference to z_log_minimal_printk` error,
// because the log environment is missing in the unit_testing platform.
void z_log_minimal_printk(const char *fmt, ...) {}

ZTEST(edgehog_device, test_edgehog_device_new)
{
    // Configuration options for the Astarte device
    astarte_device_config_t astarte_device_config = {
        .http_timeout_ms = 1000,
        .mqtt_connection_timeout_ms = 100,
        .mqtt_poll_timeout_ms = 100,
        .cred_secr = "Placeholder",
        .device_id = "Placeholder",
    };

    edgehog_device_config_t edgehog_conf = {
        .astarte_device_config = astarte_device_config,
    };

    edgehog_device_handle_t edgehog_device = NULL;
    edgehog_result_t edgehog_result = edgehog_device_new(&edgehog_conf, &edgehog_device);

    zassert_true(edgehog_result != EDGEHOG_RESULT_OK,
        "Result of edgehog device creation with edgehog_device_new is EDGEHOG_RESULT_OK");

    zassert_true(edgehog_device == NULL, "Device created with edgehog_device_new is not NULL");
}

ZTEST_SUITE(edgehog_device, NULL, NULL, NULL, NULL, NULL);
