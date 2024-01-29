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

#include <edgehog_device/edgehog_device.h>
#include <lib/edgehog_device/edgehog_device.c>

ZTEST(edgehog_device, test_edgehog_device_new)
{
    edgehog_device_handle_t edgehog_device = edgehog_device_new();
    zassert_true(edgehog_device != NULL,
        "edgehog_device_handle_t returned by edgehog_device_new function is not valid");
    edgehog_device_destroy(edgehog_device);
}

ZTEST_SUITE(edgehog_device, NULL, NULL, NULL, NULL, NULL);
