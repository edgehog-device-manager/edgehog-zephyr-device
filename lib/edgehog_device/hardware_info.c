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
        astarte_individual_from_string(CONFIG_ARCH));
    if (res != ASTARTE_RESULT_OK) {
        EDGEHOG_LOG_ERR("Unable to publish cpu/architecture");
        return;
    }
#ifdef CONFIG_SOC_SERIES
    res = astarte_device_set_property(edgehog_device->astarte_device,
        io_edgehog_devicemanager_HardwareInfo.name, "/cpu/model",
        astarte_individual_from_string(CONFIG_SOC_SERIES));
    if (res != ASTARTE_RESULT_OK) {
        EDGEHOG_LOG_ERR("Unable to publish cpu/model");
        return;
    }
#endif
    res = astarte_device_set_property(edgehog_device->astarte_device,
        io_edgehog_devicemanager_HardwareInfo.name, "/cpu/modelName",
        astarte_individual_from_string(CONFIG_BOARD));
    if (res != ASTARTE_RESULT_OK) {
        EDGEHOG_LOG_ERR("Unable to publish cpu/modelName");
        return;
    }
#ifdef CONFIG_SOC_FAMILY
    res = astarte_device_set_property(edgehog_device->astarte_device,
        io_edgehog_devicemanager_HardwareInfo.name, "/cpu/vendor",
        astarte_individual_from_string(CONFIG_SOC_FAMILY));
    if (res != ASTARTE_RESULT_OK) {
        EDGEHOG_LOG_ERR("Unable to publish cpu/vendor");
        return;
    }
#endif

    size_t memory_size = 0;

    if (hardware_info_get_memory_size(&memory_size)) {
        res = astarte_device_set_property(edgehog_device->astarte_device,
            io_edgehog_devicemanager_HardwareInfo.name, "/mem/totalBytes",
            astarte_individual_from_longinteger(memory_size));
        if (res != ASTARTE_RESULT_OK) {
            EDGEHOG_LOG_ERR("Unable to publish /mem/totalBytes");
            return;
        }
    }
}

bool hardware_info_get_memory_size(size_t *const memory_size)
{
    if (!memory_size) {
        EDGEHOG_LOG_ERR("reg_size provided is not valid");
        return false;
    }

    *memory_size = 0;
    bool found = false;

#if DT_HAS_CHOSEN(zephyr_sram)
    *memory_size = DT_REG_SIZE(DT_CHOSEN(zephyr_sram));
    found = true;
#endif
#if DT_NODE_HAS_PROP(DT_NODELABEL(psram0), reg)
    *memory_size = *memory_size + DT_REG_SIZE(DT_NODELABEL(psram0));
    found = true;
#endif

    return found;
}
