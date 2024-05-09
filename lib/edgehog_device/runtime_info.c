/*
 * (C) Copyright 2024, SECO Mind Srl
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime_info.h"

#include "edgehog_private.h"

#include <zephyr/kernel.h>

#include <astarte_device_sdk/device.h>
#include <astarte_device_sdk/result.h>

#include "generated_interfaces.h"
#include "log.h"
EDGEHOG_LOG_MODULE_REGISTER(runtime_info, CONFIG_EDGEHOG_DEVICE_RUNTIME_INFO_LOG_LEVEL);

/************************************************
 * Constants and defines
 ***********************************************/

#define RUNTIME_NAME_PROP "/name"
#define RUNTIME_URL_PROP "/url"
#define RUNTIME_VERSION_PROP "/version"
#define RUNTIME_ENV_PROP "/environment"
#define RUNTIME_NAME "edgehog-zephyr-device"
#define RUNTIME_URL "https://github.com/secomind/edgehog-zephyr-device"
// RUNTIME_VERSION will be like: "99.99.99" resulting 8 chars.
#define RUNTIME_VERSION_SIZE 8
// ENVIRONMENT will be like: "Zephyr 255.255.255" resulting 18 chars.
#define ENVIRONMENT_SIZE 18

/************************************************
 * Static functions definition
 ***********************************************/

void publish_runtime_info(edgehog_device_handle_t edgehog_device)
{
    astarte_result_t res = astarte_device_set_property(edgehog_device->astarte_device,
        io_edgehog_devicemanager_RuntimeInfo.name, RUNTIME_NAME_PROP,
        astarte_value_from_string(RUNTIME_NAME));
    if (res != ASTARTE_RESULT_OK) {
        EDGEHOG_LOG_ERR("Unable to publish runtime name");
        return;
    }

    res = astarte_device_set_property(edgehog_device->astarte_device,
        io_edgehog_devicemanager_RuntimeInfo.name, RUNTIME_URL_PROP,
        astarte_value_from_string(RUNTIME_URL));
    if (res != ASTARTE_RESULT_OK) {
        EDGEHOG_LOG_ERR("Unable to publish runtime url");
        return;
    }

    char runtime_version[RUNTIME_VERSION_SIZE + 1] = { 0 };
    int snprintf_rc = snprintf(runtime_version, RUNTIME_VERSION_SIZE, "%d.%d.%d",
        EDGEHOG_DEVICE_MAJOR, // NOLINT(hicpp-signed-bitwise)
        EDGEHOG_DEVICE_MINOR, // NOLINT(hicpp-signed-bitwise)
        EDGEHOG_DEVICE_PATCH // NOLINT(hicpp-signed-bitwise)
    );
    if (snprintf_rc > RUNTIME_VERSION_SIZE) {
        EDGEHOG_LOG_ERR("Incorrect length/format for runtime version");
        return;
    }

    res = astarte_device_set_property(edgehog_device->astarte_device,
        io_edgehog_devicemanager_RuntimeInfo.name, RUNTIME_VERSION_PROP,
        astarte_value_from_string(runtime_version));

    if (res != ASTARTE_RESULT_OK) {
        EDGEHOG_LOG_ERR("Unable to publish runtime version");
        return;
    }

    uint32_t kernel_version = sys_kernel_version_get();

    char environment[ENVIRONMENT_SIZE + 1] = { 0 };
    snprintf_rc = snprintf(environment, ENVIRONMENT_SIZE, "Zephyr %d.%d.%d",
        SYS_KERNEL_VER_MAJOR(kernel_version), // NOLINT(hicpp-signed-bitwise)
        SYS_KERNEL_VER_MINOR(kernel_version), // NOLINT(hicpp-signed-bitwise)
        SYS_KERNEL_VER_PATCHLEVEL(kernel_version)); // NOLINT(hicpp-signed-bitwise)

    if (snprintf_rc > ENVIRONMENT_SIZE) {
        EDGEHOG_LOG_ERR("Incorrect length/format for environment");
        return;
    }

    res = astarte_device_set_property(edgehog_device->astarte_device,
        io_edgehog_devicemanager_RuntimeInfo.name, RUNTIME_ENV_PROP,
        astarte_value_from_string(environment));

    if (res != ASTARTE_RESULT_OK) {
        EDGEHOG_LOG_ERR("Unable to publish runtime environment");
    }
}
