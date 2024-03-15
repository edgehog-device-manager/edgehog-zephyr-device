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
    edgehog_device_config_t edgehog_conf = {
        .astarte_device = NULL,
    };

    edgehog_device_handle_t edgehog_device = NULL;
    edgehog_result_t edgehog_result = edgehog_device_new(&edgehog_conf, &edgehog_device);

    zassert_true(edgehog_result != EDGEHOG_RESULT_OK,
        "edgehog_result returned by edgehog_device_new function is == EDGEHOG_RESULT_OK");

    zassert_true(edgehog_device == NULL,
        "edgehog_device_handle_t returned by edgehog_device_new function is not valid");

    edgehog_device_destroy(edgehog_device);
}

ZTEST_SUITE(edgehog_device, NULL, NULL, NULL, NULL, NULL);
