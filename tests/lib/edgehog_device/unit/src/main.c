/*
 * (C) Copyright 2024, SECO Mind Srl
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * @file edgehog-zephyr-device/tests/lib/edgehog_device/unit/src/main.c
 *
 * @details Test suite with a template implementation for the unit tests.
 *
 * @note This should be run with the latest version of zephyr present on master (or 3.6.0)
 */

#include <zephyr/ztest.h>

// Define a minimal_log function to resolve the `undefined reference to z_log_minimal_printk` error,
// because the log environment is missing in the unit_testing platform.
void z_log_minimal_printk(const char *fmt, ...) {}

ZTEST(edgehog_device, test_edgehog_device_simple)
{
    zassert_true(true, "");
}

ZTEST_SUITE(edgehog_device, NULL, NULL, NULL, NULL, NULL);
