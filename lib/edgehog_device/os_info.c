/*
 * (C) Copyright 2024, SECO Mind Srl
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "os_info.h"

#include "edgehog_private.h"

#include <stdio.h>

#include <zephyr/kernel.h>

#include <astarte_device_sdk/device.h>
#include <astarte_device_sdk/result.h>

#include "generated_interfaces.h"
#include "log.h"

EDGEHOG_LOG_MODULE_REGISTER(os_info, CONFIG_EDGEHOG_DEVICE_OS_INFO_LOG_LEVEL);

/************************************************
 * Constants and defines
 ***********************************************/

// Version will be like: "255.255.255" resulting 11 chars.
#define OS_VERSION_SIZE 11

/************************************************
 * Global functions definition
 ***********************************************/

void publish_os_info(edgehog_device_handle_t edgehog_device)
{
    astarte_result_t res = astarte_device_set_property(edgehog_device->astarte_device,
        io_edgehog_devicemanager_OSInfo.name, "/osName", astarte_value_from_string("Zephyr"));

    if (res != ASTARTE_RESULT_OK) {
        EDGEHOG_LOG_ERR("Unable to publish osName");
        return;
    }

    uint32_t kernel_version = sys_kernel_version_get();

    char os_version[OS_VERSION_SIZE + 1] = { 0 };

    int snprintf_rc = snprintf(os_version, OS_VERSION_SIZE, "%d.%d.%d",
        SYS_KERNEL_VER_MAJOR(kernel_version), // NOLINT(hicpp-signed-bitwise)
        SYS_KERNEL_VER_MINOR(kernel_version), // NOLINT(hicpp-signed-bitwise)
        SYS_KERNEL_VER_PATCHLEVEL(kernel_version)); // NOLINT(hicpp-signed-bitwise)

    if (snprintf_rc != OS_VERSION_SIZE) {
        EDGEHOG_LOG_ERR("Incorrect length/format for os version");
        return;
    }

    res = astarte_device_set_property(edgehog_device->astarte_device,
        io_edgehog_devicemanager_OSInfo.name, "/osVersion", astarte_value_from_string(os_version));

    if (res != ASTARTE_RESULT_OK) {
        EDGEHOG_LOG_ERR("Unable to publish osVersion");
        return;
    }
}
