/*
 * (C) Copyright 2024, SECO Mind Srl
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "edgehog_device/device.h"
#include "edgehog_device/result.h"
#include "edgehog_private.h"
#include "hardware_info.h"
#include "os_info.h"

#include <stdlib.h>

#include <zephyr/kernel.h>

#include <astarte_device_sdk/interface.h>
#include <astarte_device_sdk/uuid.h>

#include "log.h"
EDGEHOG_LOG_MODULE_REGISTER(edgehog_device, CONFIG_EDGEHOG_DEVICE_DEVICE_LOG_LEVEL);

/************************************************
 * Static functions declaration
 ***********************************************/

static edgehog_result_t add_interfaces(astarte_device_handle_t astarte_device);

static void edgehog_initial_publish(edgehog_device_handle_t edgehog_device);

/************************************************
 * Global functions definition
 ***********************************************/

edgehog_result_t edgehog_device_new(
    edgehog_device_config_t *config, edgehog_device_handle_t *edgehog_handle)
{
    edgehog_result_t res = EDGEHOG_RESULT_OK;
    if (!config) {
        EDGEHOG_LOG_ERR("Unable to init Edgehog device, no config provided");
        return EDGEHOG_RESULT_INVALID_CONFIGURATION;
    }

    if (!config->astarte_device) {
        EDGEHOG_LOG_ERR("Unable to init Edgehog device, Astarte device was NULL");
        return EDGEHOG_RESULT_INVALID_CONFIGURATION;
    }

    edgehog_device_handle_t edgehog_device
        = (edgehog_device_handle_t) calloc(1, sizeof(struct edgehog_device_t));
    if (!edgehog_device) {
        EDGEHOG_LOG_ERR("Out of memory %s: %d", __FILE__, __LINE__);
        return EDGEHOG_RESULT_OUT_OF_MEMORY;
    }

    edgehog_device->astarte_device = config->astarte_device;

    astarte_uuid_t boot_id;
    astarte_uuid_generate_v4(boot_id);
    astarte_result_t astarte_result
        = astarte_uuid_to_string(boot_id, edgehog_device->boot_id, ASTARTE_UUID_LEN);

    if (astarte_result != ASTARTE_RESULT_OK) {
        EDGEHOG_LOG_ERR("Unable to generate edgehog boot id");
        goto failure;
    }

    res = add_interfaces(config->astarte_device);

    if (res != EDGEHOG_RESULT_OK) {
        EDGEHOG_LOG_ERR("Unable to add interface into Astarte Device SDK");
        goto failure;
    }

    *edgehog_handle = edgehog_device;

    return res;

failure:
    free(edgehog_device);
    return res;
}

void edgehog_device_destroy(edgehog_device_handle_t edgehog_device)
{
    free(edgehog_device);
}

edgehog_result_t edgehog_device_start(edgehog_device_handle_t edgehog_device)
{
    edgehog_initial_publish(edgehog_device);

    return EDGEHOG_RESULT_OK;
}

/************************************************
 * Static functions definition
 ***********************************************/

static edgehog_result_t add_interfaces(astarte_device_handle_t device)
{
    const astarte_interface_t *const interfaces[] = {
        &hardware_info_interface,
        &os_info_interface,
    };

    int len = sizeof(interfaces) / sizeof(const astarte_interface_t *);

    for (int i = 0; i < len; i++) {
        astarte_result_t ret = astarte_device_add_interface(device, interfaces[i]);
        if (ret != ASTARTE_RESULT_OK) {
            EDGEHOG_LOG_ERR(
                "Unable to add Astarte Interface ( %s ) error code: %d", interfaces[i]->name, ret);
            return EDGEHOG_RESULT_ASTARTE_ERROR;
        }
    }

    return EDGEHOG_RESULT_OK;
}

static void edgehog_initial_publish(edgehog_device_handle_t edgehog_device)
{
    publish_hardware_info(edgehog_device);
    publish_os_info(edgehog_device);
}
