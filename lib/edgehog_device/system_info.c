/*
 * (C) Copyright 2024, SECO Mind Srl
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "system_info.h"

#include "edgehog_device/device.h"
#include "edgehog_private.h"
#include "generated_interfaces.h"
#include "log.h"

#include <stdio.h>

#include <zephyr/drivers/hwinfo.h>
#include <zephyr/kernel.h>

#include <astarte_device_sdk/device.h>
#include <astarte_device_sdk/result.h>

EDGEHOG_LOG_MODULE_REGISTER(system_info, CONFIG_EDGEHOG_DEVICE_SYSTEM_INFO_LOG_LEVEL);

/************************************************
 * Static functions declaration
 ***********************************************/

static void publish_serial_number(edgehog_device_handle_t edgehog_device);

static void publish_part_number(edgehog_device_handle_t edgehog_device);

/************************************************
 * Constants and defines
 ***********************************************/

#define SERIAL_NUMBER_PROP "/serialNumber"
#define PART_NUMBER_PROP "/partNumber"

/************************************************
 * Global functions definition
 ***********************************************/

void publish_system_info(edgehog_device_handle_t edgehog_device)
{
    publish_serial_number(edgehog_device);

    publish_part_number(edgehog_device);
}

/************************************************
 * Static functions definition
 ***********************************************/

static void publish_serial_number(edgehog_device_handle_t edgehog_device)
{
#if defined(CONFIG_EDGEHOG_DEVICE_SERIAL_NUMBER)
    astarte_result_t res = astarte_device_set_property(edgehog_device->astarte_device,
        io_edgehog_devicemanager_SystemInfo.name, SERIAL_NUMBER_PROP,
        astarte_value_from_string(CONFIG_EDGEHOG_DEVICE_SERIAL_NUMBER));

    if (res != ASTARTE_RESULT_OK) {
        EDGEHOG_LOG_ERR("Unable to publish " SERIAL_NUMBER_PROP);
        return;
    }
#endif
}

static void publish_part_number(edgehog_device_handle_t edgehog_device)
{
#if defined(CONFIG_EDGEHOG_DEVICE_PART_NUMBER)
    astarte_result_t res = astarte_device_set_property(edgehog_device->astarte_device,
        io_edgehog_devicemanager_SystemInfo.name, PART_NUMBER_PROP,
        astarte_value_from_string(CONFIG_EDGEHOG_DEVICE_PART_NUMBER));

    if (res != ASTARTE_RESULT_OK) {
        EDGEHOG_LOG_ERR("Unable to publish " PART_NUMBER_PROP);
        return;
    }
#endif
}
