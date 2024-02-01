/*
 * (C) Copyright 2024, SECO Mind Srl
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(simple_main); // NOLINT

#include <edgehog_device/edgehog_device.h>

// NOLINTNEXTLINE(hicpp-function-size)
int main(void)
{
    LOG_INF("Hello world! %s\n", CONFIG_BOARD); // NOLINT
    edgehog_device_handle_t edgehog_device = edgehog_device_new();
    if (!edgehog_device) {
        LOG_ERR("Unable to create edgehog device handle"); // NOLINT
        return -1;
    }
    LOG_INF("Edgehog device created"); // NOLINT

    edgehog_device_destroy(edgehog_device);
    LOG_INF("Edgehog app end"); // NOLINT
    return 0;
}
