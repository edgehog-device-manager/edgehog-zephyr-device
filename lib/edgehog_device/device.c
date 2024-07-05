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

#include <stdlib.h>

#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>

#include <astarte_device_sdk/device.h>
#include <astarte_device_sdk/interface.h>
#include <astarte_device_sdk/uuid.h>

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

    res = edgehog_settings_init();
    if (res != EDGEHOG_RESULT_OK) {
        EDGEHOG_LOG_ERR("Edgehog Settings Init failed");
        goto failure;
    }

    astarte_uuid_t boot_id;
    astarte_uuid_generate_v4(boot_id);
    astarte_result_t astarte_result
        = astarte_uuid_to_string(boot_id, edgehog_device->boot_id, ASTARTE_UUID_STR_LEN + 1);

    if (astarte_result != ASTARTE_RESULT_OK) {
        EDGEHOG_LOG_ERR("Unable to generate edgehog boot id");
        goto failure;
    }

    edgehog_telemetry_t *edgehog_telemetry
        = edgehog_telemetry_new(config->telemetry_config, config->telemetry_config_len);
    if (!edgehog_telemetry) {
        EDGEHOG_LOG_ERR("Unable to create edgehog telemetry update");
        goto failure;
    }
    edgehog_device->edgehog_telemetry = edgehog_telemetry;

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

    edgehog_result_t res
        = edgehog_telemetry_start(edgehog_device, edgehog_device->edgehog_telemetry);
    if (res != EDGEHOG_RESULT_OK) {
        EDGEHOG_LOG_ERR("Unable to start Edgehog device");
    }

    return res;
}

void edgehog_device_datastream_object_events_handler(
    edgehog_device_handle_t edgehog_device, astarte_device_datastream_object_event_t event)
{
    if (!edgehog_device) {
        EDGEHOG_LOG_ERR("Unable to handle event, Edgehog device undefined");
        return;
    }

    astarte_device_data_event_t rx_event = event.data_event;

    if ((strcmp(rx_event.interface_name, io_edgehog_devicemanager_OTARequest.name) == 0)
        && (strcmp(rx_event.path, "/request") == 0)) {
        edgehog_result_t ota_result = edgehog_ota_event(edgehog_device, &event);
        if (ota_result != EDGEHOG_RESULT_OK) {
            EDGEHOG_LOG_ERR("Unable to handle OTA update request");
        }
    }
}

void edgehog_device_datastream_individual_events_handler(
    edgehog_device_handle_t edgehog_device, astarte_device_datastream_individual_event_t event)
{
    if (!edgehog_device) {
        EDGEHOG_LOG_ERR("Unable to handle event, Edgehog device undefined");
        return;
    }

    astarte_device_data_event_t rx_event = event.data_event;

    if ((strcmp(rx_event.interface_name, io_edgehog_devicemanager_Commands.name) == 0)
        && (strcmp(rx_event.path, "/request") == 0)) {
        edgehog_result_t ota_result = edgehog_command_event(&event);
        if (ota_result != EDGEHOG_RESULT_OK) {
            EDGEHOG_LOG_ERR("Unable to handle Command request");
        }
    } else if ((strcmp(rx_event.interface_name, io_edgehog_devicemanager_LedBehavior.name) == 0)
        && (strcmp(rx_event.path, "/indicator/behavior") == 0)) {
        edgehog_result_t led_result = edgehog_led_event(edgehog_device, &event);
        if (led_result != EDGEHOG_RESULT_OK) {
            EDGEHOG_LOG_ERR("Unable to handle LED event request");
        }
    }
}

void edgehog_device_property_set_events_handler(
    edgehog_device_handle_t edgehog_device, astarte_device_property_set_event_t event)
{
    if (!edgehog_device) {
        EDGEHOG_LOG_ERR("Unable to handle event, Edgehog device undefined");
        return;
    }

    astarte_device_data_event_t rx_event = event.data_event;

    if (strcmp(rx_event.interface_name, io_edgehog_devicemanager_config_Telemetry.name) == 0) {
        edgehog_result_t telemetry_result
            = edgehog_telemetry_config_set_event(edgehog_device, &event);
        if (telemetry_result != EDGEHOG_RESULT_OK) {
            EDGEHOG_LOG_ERR("Unable to handle Telemetry set event request");
        }
    }
}

void edgehog_device_property_unset_events_handler(
    edgehog_device_handle_t edgehog_device, astarte_device_data_event_t event)
{
    if (!edgehog_device) {
        EDGEHOG_LOG_ERR("Unable to handle event, Edgehog device undefined");
        return;
    }

    if (strcmp(event.interface_name, io_edgehog_devicemanager_config_Telemetry.name) == 0) {
        edgehog_result_t telemetry_result
            = edgehog_telemetry_config_unset_event(edgehog_device, &event);
        if (telemetry_result != EDGEHOG_RESULT_OK) {
            EDGEHOG_LOG_ERR("Unable to handle Telemetry unset event request");
        }
    }
}

/************************************************
 * Static functions definition
 ***********************************************/

static edgehog_result_t add_interfaces(astarte_device_handle_t device)
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
#ifdef CONFIG_WIFI
        &io_edgehog_devicemanager_WiFiScanResults,
#endif
        &io_edgehog_devicemanager_config_Telemetry,
    };

    for (int i = 0; i < ARRAY_SIZE(interfaces); i++) {
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
    edgehog_ota_init(edgehog_device);
    publish_hardware_info(edgehog_device);
    publish_os_info(edgehog_device);
    publish_system_info(edgehog_device);
    publish_base_image(edgehog_device);
    publish_runtime_info(edgehog_device);
    publish_system_status(edgehog_device);
    publish_storage_usage(edgehog_device);
#ifdef CONFIG_WIFI
    edgehog_wifi_scan_start(edgehog_device->astarte_device);
#endif
}

void edgehog_device_publish_telemetry(edgehog_device_handle_t device, telemetry_type_t type)
{
    switch (type) {
        case EDGEHOG_TELEMETRY_HW_INFO:
            publish_hardware_info(device);
            break;
#ifdef CONFIG_WIFI
        case EDGEHOG_TELEMETRY_WIFI_SCAN:
            edgehog_wifi_scan_start(device->astarte_device);
            break;
#endif
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
