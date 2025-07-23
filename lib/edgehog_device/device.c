/*
 * (C) Copyright 2024, SECO Mind Srl
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "edgehog_device/device.h"

#include "base_image.h"
#include "command.h"
#include "edgehog_device/result.h"
#include "edgehog_device/wifi_scan.h"
#include "edgehog_private.h"
#include "generated_interfaces.h"
#include "hardware_info.h"
#include "led.h"
#include "log.h"
#include "os_info.h"
#include "runtime_info.h"
#include "settings.h"
#include "storage_usage.h"
#include "system_info.h"
#include "system_status.h"
#include "uuid.h"

#include <stdlib.h>

#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>

#include <astarte_device_sdk/device.h>
#include <astarte_device_sdk/interface.h>

EDGEHOG_LOG_MODULE_REGISTER(edgehog_device, CONFIG_EDGEHOG_DEVICE_DEVICE_LOG_LEVEL);

/************************************************
 * Static functions declaration
 ***********************************************/

static edgehog_result_t add_interfaces(astarte_device_handle_t astarte_device);

static void edgehog_initial_publish(edgehog_device_handle_t edgehog_device);

/************************************************
 *       Callbacks declaration/definition       *
 ***********************************************/

static void astarte_connection_cbk(astarte_device_connection_event_t event)
{
    EDGEHOG_LOG_DBG("Astarte device connected");

    edgehog_device_handle_t edgehog_device = (edgehog_device_handle_t) event.user_data;

    edgehog_device->state = EDGEHOG_DEVICE_CONNECTED;

    if (edgehog_device->user_connection_cbk) {
        event.user_data = edgehog_device->user_cbk_user_data;
        edgehog_device->user_connection_cbk(event);
    }
}

static void astarte_disconnection_cbk(astarte_device_disconnection_event_t event)
{
    EDGEHOG_LOG_DBG("Astarte device disconnected");

    edgehog_device_handle_t edgehog_device = (edgehog_device_handle_t) event.user_data;

    if (edgehog_device->state != EDGEHOG_DEVICE_STOPPED) {
        edgehog_device->state = EDGEHOG_DEVICE_STARTED;
    }

    if (edgehog_device->user_disconnection_cbk) {
        event.user_data = edgehog_device->user_cbk_user_data;
        edgehog_device->user_disconnection_cbk(event);
    }
}

static void astarte_datastream_individual_cbk(astarte_device_datastream_individual_event_t event)
{
    EDGEHOG_LOG_DBG("Astarte datastream individual received");
    astarte_device_data_event_t base_event = event.base_event;
    edgehog_device_handle_t edgehog_device = (edgehog_device_handle_t) base_event.user_data;

    if ((strcmp(base_event.interface_name, io_edgehog_devicemanager_Commands.name) == 0)
        && (strcmp(base_event.path, "/request") == 0)) {
        edgehog_result_t ota_result = edgehog_command_event(&event);
        if (ota_result != EDGEHOG_RESULT_OK) {
            EDGEHOG_LOG_ERR("Unable to handle Command request");
        }
        return;
    }

    if ((strcmp(base_event.interface_name, io_edgehog_devicemanager_LedBehavior.name) == 0)
        && (strcmp(base_event.path, "/indicator/behavior") == 0)) {
        edgehog_result_t led_result = edgehog_led_event(edgehog_device, &event);
        if (led_result != EDGEHOG_RESULT_OK) {
            EDGEHOG_LOG_ERR("Unable to handle LED event request");
        }
        return;
    }

    if (edgehog_device->user_datastream_individual_cbk) {
        event.base_event.user_data = edgehog_device->user_cbk_user_data;
        edgehog_device->user_datastream_individual_cbk(event);
    }
}

static void astarte_datastream_object_cbk(astarte_device_datastream_object_event_t event)
{
    EDGEHOG_LOG_DBG("Astarte datastream object received");
    astarte_device_data_event_t base_event = event.base_event;
    edgehog_device_handle_t edgehog_device = (edgehog_device_handle_t) base_event.user_data;

    if (strcmp(base_event.interface_name, io_edgehog_devicemanager_OTARequest.name) == 0) {
        if (strcmp(base_event.path, "/request") != 0) {
            EDGEHOG_LOG_ERR("Received OTA request on incorrect common path: '%s'", base_event.path);
            return;
        }
        edgehog_result_t ota_result = edgehog_ota_event(edgehog_device, &event);
        if (ota_result != EDGEHOG_RESULT_OK) {
            EDGEHOG_LOG_ERR("Unable to handle OTA update request");
        }
        return;
    }

    if (edgehog_device->user_datastream_object_cbk) {
        event.base_event.user_data = edgehog_device->user_cbk_user_data;
        edgehog_device->user_datastream_object_cbk(event);
    }
}

static void astarte_property_set_cbk(astarte_device_property_set_event_t event)
{
    EDGEHOG_LOG_DBG("Astarte property set received");

    astarte_device_data_event_t base_event = event.base_event;
    edgehog_device_handle_t edgehog_device = (edgehog_device_handle_t) base_event.user_data;

    if (strcmp(base_event.interface_name, io_edgehog_devicemanager_config_Telemetry.name) == 0) {
        edgehog_result_t eres
            = edgehog_telemetry_config_set_event(edgehog_device->telemetry, &event);
        if (eres != EDGEHOG_RESULT_OK) {
            EDGEHOG_LOG_ERR("Unable to handle Telemetry set event request");
        }
        return;
    }

    if (edgehog_device->user_property_set_cbk) {
        base_event.user_data = edgehog_device->user_cbk_user_data;
        edgehog_device->user_property_set_cbk(event);
    }
}

static void astarte_property_unset_cbk(astarte_device_data_event_t event)
{
    EDGEHOG_LOG_DBG("Astarte property unset received");

    edgehog_device_handle_t edgehog_device = (edgehog_device_handle_t) event.user_data;

    if (strcmp(event.interface_name, io_edgehog_devicemanager_config_Telemetry.name) == 0) {
        edgehog_result_t eres
            = edgehog_telemetry_config_unset_event(edgehog_device->telemetry, &event);
        if (eres != EDGEHOG_RESULT_OK) {
            EDGEHOG_LOG_ERR("Unable to handle Telemetry unset event request");
        }
        return;
    }

    if (edgehog_device->user_property_unset_cbk) {
        event.user_data = edgehog_device->user_cbk_user_data;
        edgehog_device->user_property_unset_cbk(event);
    }
}

/************************************************
 * Global functions definition
 ***********************************************/

edgehog_result_t edgehog_device_new(
    edgehog_device_config_t *config, edgehog_device_handle_t *edgehog_handle)
{
    edgehog_device_handle_t edgehog_device = NULL;
    astarte_device_handle_t astarte_device = NULL;
    edgehog_result_t eres = EDGEHOG_RESULT_OK;
    astarte_result_t ares = ASTARTE_RESULT_OK;

    if (!config || !edgehog_handle) {
        EDGEHOG_LOG_ERR("Unable to init Edgehog device, missing config or device handle.");
        return EDGEHOG_RESULT_INVALID_PARAM;
    }

    // Step 1: Initialize the Edgehog settings
    eres = edgehog_settings_init();
    if (eres != EDGEHOG_RESULT_OK) {
        EDGEHOG_LOG_ERR("Edgehog Settings Init failed");
        goto failure;
    }

    // Step 2: Allocate space for the Edgehog device
    edgehog_device = calloc(1, sizeof(struct edgehog_device));
    if (!edgehog_device) {
        EDGEHOG_LOG_ERR("Out of memory %s: %d", __FILE__, __LINE__);
        eres = EDGEHOG_RESULT_OUT_OF_MEMORY;
        goto failure;
    }

    // Step 3: Initialize the Astarte device
    astarte_device_config_t *astarte_device_config = &config->astarte_device_config;
    astarte_device_connection_cbk_t user_connection_cbk = astarte_device_config->connection_cbk;
    astarte_device_disconnection_cbk_t user_disconnection_cbk
        = astarte_device_config->disconnection_cbk;
    astarte_device_datastream_individual_cbk_t user_datastream_individual_cbk
        = astarte_device_config->datastream_individual_cbk;
    astarte_device_datastream_object_cbk_t user_datastream_object_cbk
        = astarte_device_config->datastream_object_cbk;
    astarte_device_property_set_cbk_t user_property_set_cbk
        = astarte_device_config->property_set_cbk;
    astarte_device_property_unset_cbk_t user_property_unset_cbk
        = astarte_device_config->property_unset_cbk;
    void *user_cbk_user_data = astarte_device_config->cbk_user_data;

    astarte_device_config->connection_cbk = astarte_connection_cbk;
    astarte_device_config->disconnection_cbk = astarte_disconnection_cbk;
    astarte_device_config->datastream_individual_cbk = astarte_datastream_individual_cbk;
    astarte_device_config->datastream_object_cbk = astarte_datastream_object_cbk;
    astarte_device_config->property_set_cbk = astarte_property_set_cbk;
    astarte_device_config->property_unset_cbk = astarte_property_unset_cbk;
    astarte_device_config->cbk_user_data = edgehog_device;

    ares = astarte_device_new(&config->astarte_device_config, &astarte_device);
    if (ares != ASTARTE_RESULT_OK) {
        EDGEHOG_LOG_ERR("Astarte device creation error: %s", astarte_result_to_name(ares));
        eres = EDGEHOG_RESULT_ASTARTE_ERROR;
        goto failure;
    }

    // Step 4: Add the edgehog interfaces to the Astarte device
    eres = add_interfaces(astarte_device);
    if (eres != EDGEHOG_RESULT_OK) {
        EDGEHOG_LOG_ERR("Unable to add interface into Astarte Device SDK");
        goto failure;
    }

    // Step 5: Initialize the Edgehog device boot ID
    char boot_id[UUID_STR_LEN + 1] = { 0 };
    eres = uuid_generate_v4_string(boot_id);
    if (eres != EDGEHOG_RESULT_OK) {
        EDGEHOG_LOG_ERR("Unable to generate edgehog boot id");
        goto failure;
    }

    // Step 6: Initialize the telemetry for the Edgehog device
    edgehog_telemetry_t *telemetry
        = edgehog_telemetry_new(config->telemetry_config, config->telemetry_config_len);
    if (!telemetry) {
        EDGEHOG_LOG_ERR("Unable to create edgehog telemetry update");
        goto failure;
    }

    // Step 7: Fill in the Edgehog device struct
    *edgehog_device = (struct edgehog_device) {
        .state = EDGEHOG_DEVICE_STOPPED,
        .initial_publish = false,
        .astarte_device = astarte_device,
        .astarte_error = ASTARTE_RESULT_OK,
        .user_connection_cbk = user_connection_cbk,
        .user_disconnection_cbk = user_disconnection_cbk,
        .user_datastream_individual_cbk = user_datastream_individual_cbk,
        .user_datastream_object_cbk = user_datastream_object_cbk,
        .user_property_set_cbk = user_property_set_cbk,
        .user_property_unset_cbk = user_property_unset_cbk,
        .user_cbk_user_data = user_cbk_user_data,
        .telemetry = telemetry,
    };
    memcpy(edgehog_device->boot_id, boot_id, UUID_STR_LEN + 1);
    *edgehog_handle = edgehog_device;

    return eres;

failure:
    astarte_device_destroy(astarte_device);
    free(edgehog_device);
    return eres;
}

void edgehog_device_destroy(edgehog_device_handle_t edgehog_device)
{
    if (!edgehog_device) {
        return;
    }
    astarte_result_t ares = astarte_device_destroy(edgehog_device->astarte_device);
    if (ares != ASTARTE_RESULT_OK) {
        edgehog_device->astarte_error = ares;
        EDGEHOG_LOG_ERR("Astarte device destroy error: %s", astarte_result_to_name(ares));
    }
    edgehog_telemetry_destroy(edgehog_device->telemetry);
    free(edgehog_device);
}

edgehog_result_t edgehog_device_start(edgehog_device_handle_t edgehog_device)
{
    astarte_result_t ares = astarte_device_connect(edgehog_device->astarte_device);
    if (ares != ASTARTE_RESULT_OK) {
        edgehog_device->astarte_error = ares;
        EDGEHOG_LOG_ERR("Astarte device connection error: %s", astarte_result_to_name(ares));
        return EDGEHOG_RESULT_ASTARTE_ERROR;
    }
    edgehog_device->state = EDGEHOG_DEVICE_STARTED;
    return EDGEHOG_RESULT_OK;
}

edgehog_result_t edgehog_device_poll(edgehog_device_handle_t edgehog_device)
{
    astarte_result_t ares = astarte_device_poll(edgehog_device->astarte_device);
    if (ares != ASTARTE_RESULT_OK) {
        edgehog_device->astarte_error = ares;
        EDGEHOG_LOG_ERR("Astarte device poll failure.");
        return EDGEHOG_RESULT_ASTARTE_ERROR;
    }

    edgehog_result_t eres = EDGEHOG_RESULT_OK;
    if (edgehog_device->state == EDGEHOG_DEVICE_CONNECTED) {
        if (!edgehog_device->initial_publish) {
            edgehog_initial_publish(edgehog_device);
            edgehog_device->initial_publish = true;
        }

        edgehog_telemetry_t *telemetry = edgehog_device->telemetry;
        if (!edgehog_telemetry_is_running(telemetry)) {
            eres = edgehog_telemetry_start(edgehog_device);
            if (eres != EDGEHOG_RESULT_OK) {
                EDGEHOG_LOG_ERR("Unable to start Edgehog telemetry service");
            }
        }
    }
    return eres;
}

edgehog_result_t edgehog_device_stop(edgehog_device_handle_t edgehog_device, k_timeout_t timeout)
{
    edgehog_result_t eres = edgehog_telemetry_stop(edgehog_device->telemetry, timeout);
    if (eres != EDGEHOG_RESULT_OK) {
        EDGEHOG_LOG_ERR("Unable to stop the Edgehog device within the timeout");
        return eres;
    }
    astarte_result_t ares = astarte_device_disconnect(edgehog_device->astarte_device, timeout);
    if (ares != ASTARTE_RESULT_OK) {
        edgehog_device->astarte_error = ares;
        EDGEHOG_LOG_ERR("Astarte device disconnection failure %s.", astarte_result_to_name(ares));
        return EDGEHOG_RESULT_ASTARTE_ERROR;
    }
    return EDGEHOG_RESULT_OK;
}

astarte_device_handle_t edgehog_device_get_astarte_device(edgehog_device_handle_t edgehog_device)
{
    return edgehog_device->astarte_device;
}

astarte_result_t edgehog_device_get_astarte_error(edgehog_device_handle_t edgehog_device)
{
    return edgehog_device->astarte_error;
}

void edgehog_device_publish_telemetry(edgehog_device_handle_t device, edgehog_telemetry_type_t type)
{
    switch (type) {
        case EDGEHOG_TELEMETRY_HW_INFO:
            publish_hardware_info(device);
            break;
            // #ifdef CONFIG_WIFI
            //         case EDGEHOG_TELEMETRY_WIFI_SCAN:
            //             edgehog_wifi_scan_start(device->astarte_device);
            //             break;
            // #endif
        case EDGEHOG_TELEMETRY_SYSTEM_STATUS:
            publish_system_status(device);
            break;
        case EDGEHOG_TELEMETRY_STORAGE_USAGE:
            publish_storage_usage(device);
            break;
        default:
            return;
    }
}

/************************************************
 * Static functions definition
 ***********************************************/

static edgehog_result_t add_interfaces(astarte_device_handle_t astarte_device)
{
    const astarte_interface_t *const interfaces[] = {
        &io_edgehog_devicemanager_HardwareInfo,
        &io_edgehog_devicemanager_OSInfo,
        &io_edgehog_devicemanager_SystemInfo,
        &io_edgehog_devicemanager_OTAEvent,
        &io_edgehog_devicemanager_OTARequest,
        &io_edgehog_devicemanager_BaseImage,
        &io_edgehog_devicemanager_Commands,
        &io_edgehog_devicemanager_RuntimeInfo,
        &io_edgehog_devicemanager_SystemStatus,
        &io_edgehog_devicemanager_StorageUsage,
#if DT_NODE_HAS_STATUS(EDGEHOG_LED_NODE, okay)
        &io_edgehog_devicemanager_LedBehavior,
#endif
        // #ifdef CONFIG_WIFI
        //         &io_edgehog_devicemanager_WiFiScanResults,
        // #endif
        &io_edgehog_devicemanager_config_Telemetry,
    };

    for (int i = 0; i < ARRAY_SIZE(interfaces); i++) {
        astarte_result_t ret = astarte_device_add_interface(astarte_device, interfaces[i]);
        if (ret != ASTARTE_RESULT_OK) {
            EDGEHOG_LOG_ERR("Unable to add Astarte interface ( %s ): %s", interfaces[i]->name,
                astarte_result_to_name(ret));
            return EDGEHOG_RESULT_ASTARTE_ERROR;
        }
    }

    return EDGEHOG_RESULT_OK;
}

static void edgehog_initial_publish(edgehog_device_handle_t edgehog_device)
{
    edgehog_ota_init(edgehog_device);
    publish_hardware_info(edgehog_device);
    publish_os_info(edgehog_device);
    publish_system_info(edgehog_device);
    publish_base_image(edgehog_device);
    publish_runtime_info(edgehog_device);
    publish_system_status(edgehog_device);
    publish_storage_usage(edgehog_device);
    // #ifdef CONFIG_WIFI
    //     edgehog_wifi_scan_start(edgehog_device->astarte_device);
    // #endif
}
