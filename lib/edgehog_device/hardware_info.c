/*
 * (C) Copyright 2024, SECO Mind Srl
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "hardware_info.h"

#include "edgehog_private.h"

#include <zephyr/kernel.h>

#include <astarte_device_sdk/device.h>
#include <astarte_device_sdk/result.h>

#include "generated_interfaces.h"
#include "log.h"
EDGEHOG_LOG_MODULE_REGISTER(hardware_info, CONFIG_EDGEHOG_DEVICE_HARDWARE_INFO_LOG_LEVEL);

/************************************************
 * Constants and defines
 ***********************************************/

/************************************************
 * Global functions definition
 ***********************************************/

void publish_hardware_info(edgehog_device_handle_t edgehog_device)
{
    astarte_result_t res = astarte_device_set_property(edgehog_device->astarte_device,
        io_edgehog_devicemanager_HardwareInfo.name, "/cpu/architecture",
        astarte_value_from_string(CONFIG_ARCH));
    if (res != ASTARTE_RESULT_OK) {
        EDGEHOG_LOG_ERR("Unable to publish cpu/architecture");
        return;
    }
#ifdef CONFIG_SOC_SERIES
    res = astarte_device_set_property(edgehog_device->astarte_device,
        io_edgehog_devicemanager_HardwareInfo.name, "/cpu/model",
        astarte_value_from_string(CONFIG_SOC_SERIES));
    if (res != ASTARTE_RESULT_OK) {
        EDGEHOG_LOG_ERR("Unable to publish cpu/model");
        return;
    }
#endif
    res = astarte_device_set_property(edgehog_device->astarte_device,
        io_edgehog_devicemanager_HardwareInfo.name, "/cpu/modelName",
        astarte_value_from_string(CONFIG_BOARD));
    if (res != ASTARTE_RESULT_OK) {
        EDGEHOG_LOG_ERR("Unable to publish cpu/modelName");
        return;
    }
#ifdef CONFIG_SOC_FAMILY
    res = astarte_device_set_property(edgehog_device->astarte_device,
        io_edgehog_devicemanager_HardwareInfo.name, "/cpu/vendor",
        astarte_value_from_string(CONFIG_SOC_FAMILY));
    if (res != ASTARTE_RESULT_OK) {
        EDGEHOG_LOG_ERR("Unable to publish cpu/vendor");
        return;
    }
#endif
    // TODO Add /mem/totalBytes for harware_info intarface
}
