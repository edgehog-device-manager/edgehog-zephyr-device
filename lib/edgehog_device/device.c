/*
 * (C) Copyright 2024, SECO Mind Srl
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "edgehog_device/device.h"

#include "base_image.h"
#include "command.h"
#include "edgehog_device/result.h"
#include "edgehog_private.h"
#include "file_transfer/download.h"
#include "file_transfer/upload.h"
#include "generated_interfaces.h"
#include "hardware_info.h"
#include "led.h"
#include "log.h"
#include "network_properties.h"
#include "os_info.h"
#include "runtime_info.h"
#include "settings.h"
#include "storage_usage.h"
#include "system_info.h"
#include "system_status.h"
#include "wifi_scan.h"

#include <stdlib.h>
#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/uuid.h>

#include <astarte_device_sdk/device.h>
#include <astarte_device_sdk/interface.h>

#define COMMANDS_REQUEST_PATH "/request"
#define OTA_REQUEST_PATH "/request"
#define FT_REQUEST_PATH "/request"

#define EDGEHOG_DEVICE_DESTROY_EVENT_BIT BIT(0U)

EDGEHOG_LOG_MODULE_REGISTER(edgehog_device, CONFIG_EDGEHOG_DEVICE_DEVICE_LOG_LEVEL);

/************************************************
 *         Static variables declaration         *
 ***********************************************/

// NOLINTBEGIN(cppcoreguidelines-avoid-non-const-global-variables)
static struct edgehog_device edgehog_device_instance = { 0 };
static bool edgehog_device_initialized = false;
// NOLINTEND(cppcoreguidelines-avoid-non-const-global-variables)

/************************************************
 *         Static functions declarations        *
 ***********************************************/

static void edgehog_device_worker_thread_entry(void *par1, void *par2, void *par3);
static edgehog_result_t initialize_astarte_device(
    astarte_device_config_t *astarte_config, astarte_device_handle_t *astarte_device);
static void initial_publish(edgehog_device_handle_t edgehog_device);
static void handle_astarte_event(
    edgehog_device_handle_t edgehog_device, astarte_device_event_t *event);
static bool is_edgehog_interface(const char *interface_name);
static void handle_connected_event(edgehog_device_handle_t edgehog_device);
static void handle_disconnected_event(edgehog_device_handle_t edgehog_device);
static void handle_datastream_individual_event(edgehog_device_handle_t edgehog_device,
    astarte_device_datastream_individual_event_t *event, bool *handled_by_edgehog);
static void handle_datastream_object_event(edgehog_device_handle_t edgehog_device,
    astarte_device_datastream_object_event_t *event, bool *handled_by_edgehog);
static void handle_property_set_event(edgehog_device_handle_t edgehog_device,
    astarte_device_property_set_event_t *event, bool *handled_by_edgehog);
static void handle_property_unset_event(edgehog_device_handle_t edgehog_device,
    astarte_device_data_event_t *event, bool *handled_by_edgehog);
static void handle_error_event(
    edgehog_device_handle_t edgehog_device, astarte_device_error_event_t *event);

/************************************************
 *         Global functions definition          *
 ***********************************************/

edgehog_result_t edgehog_device_new(
    edgehog_device_config_t *config, edgehog_device_handle_t *edgehog_handle)
{
    edgehog_device_handle_t edgehog_device = NULL;
    astarte_device_handle_t astarte_device = NULL;
    edgehog_result_t eres = EDGEHOG_RESULT_OK;

    if (!config || !edgehog_handle) {
        EDGEHOG_LOG_ERR("Unable to init Edgehog device, missing config or device handle.");
        return EDGEHOG_RESULT_INVALID_PARAM;
    }

    if (edgehog_device_initialized) {
        EDGEHOG_LOG_ERR("Device is already initialized. Only a single instance is allowed.");
        return EDGEHOG_RESULT_INVALID_PARAM;
    }

    // Initialize the Edgehog settings
    eres = edgehog_settings_init();
    if (eres != EDGEHOG_RESULT_OK) {
        EDGEHOG_LOG_ERR("Edgehog Settings Init failed");
        goto failure;
    }

    // Use the statically allocated Edgehog device instance
    memset(&edgehog_device_instance, 0, sizeof(struct edgehog_device));
    edgehog_device = &edgehog_device_instance;

    // Initialize the Astarte device
    eres = initialize_astarte_device(&config->astarte_device_config, &astarte_device);
    if (eres != EDGEHOG_RESULT_OK) {
        EDGEHOG_LOG_ERR("Unable to initialize Astarte Device SDK");
        goto failure;
    }

    // Initialize the Edgehog device boot ID
    struct uuid boot_id = { 0 };
    char boot_id_str[UUID_STR_LEN] = { 0 };
    int res = uuid_generate_v4(&boot_id);
    if (res == 0) {
        res = uuid_to_string(&boot_id, boot_id_str);
    }
    if (res != 0) {
        EDGEHOG_LOG_ERR("Unable to generate Edgehog boot ID: %d", res);
        goto failure;
    }

    // Initialize the telemetry for the Edgehog device
    edgehog_telemetry_t *telemetry
        = edgehog_telemetry_new(config->telemetry_config, config->telemetry_config_len);
    if (!telemetry) {
        EDGEHOG_LOG_ERR("Unable to create edgehog telemetry update");
        goto failure;
    }

#ifdef CONFIG_EDGEHOG_DEVICE_FILE_TRANSFER
    // Initialize the file transfer for the Edgehog device
    edgehog_ft_t *file_transfer = edgehog_ft_new(config->file_transfer_cbks,
        config->file_transfer_partitions, config->file_transfer_partitions_len);
    if (!file_transfer) {
        EDGEHOG_LOG_ERR("Unable to create edgehog file transfer");
        goto failure;
    }
#endif

    // Fill in the Edgehog device struct
    *edgehog_device = (struct edgehog_device){
        .state = EDGEHOG_DEVICE_STOPPED,
        .initial_publish = false,
        .astarte_device = astarte_device,
        .astarte_error = ASTARTE_RESULT_OK,
        .telemetry = telemetry,
#ifdef CONFIG_EDGEHOG_DEVICE_FILE_TRANSFER
        .file_transfer = file_transfer,
#endif
        .storage_partitions = config->storage_partitions,
        .storage_partitions_len = config->storage_partitions_len,
        .user_event_cbk = config->event_cbk,
        .user_cbk_user_data = config->cbk_user_data,
    };

    k_sem_init(&edgehog_device->sync_ota_ft_sem, 1, 1);
    memcpy(edgehog_device->boot_id, boot_id_str, UUID_STR_LEN);

    // Initialize worker thread and event synchronization
    k_event_init(&edgehog_device->events);
    k_thread_create(&edgehog_device->worker_thread, edgehog_device->worker_thread_stack,
        K_THREAD_STACK_SIZEOF(edgehog_device->worker_thread_stack),
        (k_thread_entry_t) edgehog_device_worker_thread_entry, edgehog_device, NULL, NULL,
        K_PRIO_PREEMPT(CONFIG_EDGEHOG_DEVICE_WORKER_THREAD_PRIORITY), 0, K_NO_WAIT);

    *edgehog_handle = edgehog_device;
    edgehog_device_initialized = true;

    // Initialize the WiFi scan driver
#ifdef CONFIG_WIFI
    edgehog_wifi_scan_init(edgehog_device);
#endif

    return eres;

failure:
    astarte_device_destroy(astarte_device);
    edgehog_device_initialized = false;
    return eres;
}

void edgehog_device_destroy(edgehog_device_handle_t edgehog_device)
{
    if (!edgehog_device) {
        return;
    }

    if (!edgehog_device_initialized) {
        EDGEHOG_LOG_ERR("Device is not initialized. Cannot destroy it.");
        return;
    }

    // Signal worker thread to terminate cleanly
    k_event_post(&edgehog_device->events, EDGEHOG_DEVICE_DESTROY_EVENT_BIT);
    k_thread_join(&edgehog_device->worker_thread, K_FOREVER);

    astarte_result_t ares = astarte_device_destroy(edgehog_device->astarte_device);
    if (ares != ASTARTE_RESULT_OK) {
        edgehog_device->astarte_error = ares;
        EDGEHOG_LOG_ERR("Astarte device destroy error: %s", astarte_result_to_name(ares));
    }

    edgehog_telemetry_destroy(edgehog_device->telemetry);
#ifdef CONFIG_EDGEHOG_DEVICE_FILE_TRANSFER
    edgehog_ft_destroy(edgehog_device->file_transfer);
#endif

    edgehog_device_initialized = false;
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

edgehog_result_t edgehog_device_stop(edgehog_device_handle_t edgehog_device, k_timeout_t timeout)
{
    edgehog_result_t eres = edgehog_telemetry_stop(edgehog_device->telemetry, timeout);
    if (eres != EDGEHOG_RESULT_OK) {
        EDGEHOG_LOG_ERR("Unable to stop the Edgehog device within the timeout");
        return eres;
    }
#ifdef CONFIG_EDGEHOG_DEVICE_FILE_TRANSFER
    eres = edgehog_ft_stop(edgehog_device->file_transfer, timeout);
    if (eres != EDGEHOG_RESULT_OK) {
        EDGEHOG_LOG_ERR("Unable to stop the Edgehog device within the timeout");
        return eres;
    }
#endif
#ifdef CONFIG_WIFI
    eres = edgehog_wifi_scan_destroy(edgehog_device, timeout);
    if (eres != EDGEHOG_RESULT_OK) {
        EDGEHOG_LOG_ERR("Unable to stop the Edgehog device within the timeout");
        return eres;
    }
#endif
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

/************************************************
 *         Static functions definitions         *
 ***********************************************/

static void edgehog_device_worker_thread_entry(void *par1, void * /*par2*/, void * /*par3*/)
{
    edgehog_device_handle_t edgehog_device = (edgehog_device_handle_t) par1;

    while (true) {
        uint32_t events = k_event_test(&edgehog_device->events, EDGEHOG_DEVICE_DESTROY_EVENT_BIT);
        if (events & EDGEHOG_DEVICE_DESTROY_EVENT_BIT) {
            break;
        }

        astarte_device_event_t event = { 0 };
        astarte_result_t ares
            = astarte_device_get_event(edgehog_device->astarte_device, &event, K_MSEC(100));

        if (ares == ASTARTE_RESULT_OK) {
            handle_astarte_event(edgehog_device, &event);
            astarte_device_event_cleanup(&event);
        }
    }
}

static edgehog_result_t initialize_astarte_device(
    astarte_device_config_t *astarte_config, astarte_device_handle_t *astarte_device)
{
    const astarte_interface_t *const edgehog_interfaces[] = {
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
        &io_edgehog_devicemanager_NetworkInterfaceProperties,
#if DT_NODE_HAS_STATUS(EDGEHOG_LED_NODE, okay)
        &io_edgehog_devicemanager_LedBehavior,
#endif
#ifdef CONFIG_WIFI
        &io_edgehog_devicemanager_WiFiScanResults,
#endif
        &io_edgehog_devicemanager_config_Telemetry,
#ifdef CONFIG_EDGEHOG_DEVICE_FILE_TRANSFER
        &io_edgehog_devicemanager_fileTransfer_Capabilities,
        &io_edgehog_devicemanager_fileTransfer_ServerToDevice,
        &io_edgehog_devicemanager_fileTransfer_Response,
        &io_edgehog_devicemanager_fileTransfer_Progress,
        &io_edgehog_devicemanager_fileTransfer_DeviceToServer,
#endif
    };

    size_t edgehog_interfaces_size = ARRAY_SIZE(edgehog_interfaces);
    size_t user_interfaces_size = astarte_config->interfaces_size;
    size_t total_interfaces_size = edgehog_interfaces_size + user_interfaces_size;

    const astarte_interface_t **combined_interfaces = (const astarte_interface_t **) calloc(
        total_interfaces_size, sizeof(astarte_interface_t *));
    if (!combined_interfaces) {
        EDGEHOG_LOG_ERR("Out of memory %s: %d", __FILE__, __LINE__);
        return EDGEHOG_RESULT_OUT_OF_MEMORY;
    }

    // Merge user interfaces
    if (user_interfaces_size > 0 && astarte_config->interfaces != NULL) {
        memcpy((void *) combined_interfaces, (void *) astarte_config->interfaces,
            user_interfaces_size * sizeof(astarte_interface_t *));
    }

    // Merge Edgehog interfaces
    memcpy((void *) (combined_interfaces + user_interfaces_size), (void *) edgehog_interfaces,
        edgehog_interfaces_size * sizeof(astarte_interface_t *));

    // Cache original user config
    const astarte_interface_t **original_interfaces = astarte_config->interfaces;
    size_t original_size = astarte_config->interfaces_size;

    // Apply combined interfaces
    astarte_config->interfaces = combined_interfaces;
    astarte_config->interfaces_size = total_interfaces_size;

    // Initialize the Astarte device synchronously
    astarte_result_t ares = astarte_device_new(astarte_config, astarte_device);

    // Restore user config and free combined array
    astarte_config->interfaces = original_interfaces;
    astarte_config->interfaces_size = original_size;
    free((void *) combined_interfaces);

    if (ares != ASTARTE_RESULT_OK) {
        EDGEHOG_LOG_ERR("Astarte device creation error: %s", astarte_result_to_name(ares));
        return EDGEHOG_RESULT_ASTARTE_ERROR;
    }

    return EDGEHOG_RESULT_OK;
}

static void initial_publish(edgehog_device_handle_t edgehog_device)
{
    EDGEHOG_LOG_DBG("Initial publish for the edgehog device");
    edgehog_ota_init(edgehog_device);
    publish_hardware_info(edgehog_device);
    publish_os_info(edgehog_device);
    publish_system_info(edgehog_device);
    publish_base_image(edgehog_device);
    publish_runtime_info(edgehog_device);
    publish_system_status(edgehog_device);
    publish_storage_usage(edgehog_device);
    publish_network_properties(edgehog_device);
#ifdef CONFIG_WIFI
    edgehog_wifi_scan_start(edgehog_device);
#endif
#ifdef CONFIG_EDGEHOG_DEVICE_FILE_TRANSFER
    edgeghog_ft_publish_capabilities(edgehog_device);
#endif
}

static void handle_astarte_event(
    edgehog_device_handle_t edgehog_device, astarte_device_event_t *event)
{
    bool handled_by_edgehog = false;

    switch (event->type) {
        case ASTARTE_DEVICE_EVENT_CONNECTED:
            handle_connected_event(edgehog_device);
            break;
        case ASTARTE_DEVICE_EVENT_DISCONNECTED:
            handle_disconnected_event(edgehog_device);
            break;
        case ASTARTE_DEVICE_EVENT_DATASTREAM_INDIVIDUAL:
            handle_datastream_individual_event(
                edgehog_device, &event->data.datastream_individual, &handled_by_edgehog);
            break;
        case ASTARTE_DEVICE_EVENT_DATASTREAM_OBJECT:
            handle_datastream_object_event(
                edgehog_device, &event->data.datastream_object, &handled_by_edgehog);
            break;
        case ASTARTE_DEVICE_EVENT_PROPERTY_SET:
            handle_property_set_event(
                edgehog_device, &event->data.property_set, &handled_by_edgehog);
            break;
        case ASTARTE_DEVICE_EVENT_PROPERTY_UNSET:
            handle_property_unset_event(
                edgehog_device, &event->data.property_unset, &handled_by_edgehog);
            break;
        case ASTARTE_DEVICE_EVENT_ERROR:
            handle_error_event(edgehog_device, &event->data.error);
            break;
        default:
            EDGEHOG_LOG_ERR("Astarte error event received of unknown type %d", event->type);
            break;
    }

    // Forward the event to the user callback if it was NOT consumed by Edgehog,
    // or if it is a lifecycle event (CONNECTED / DISCONNECTED / ERROR)
    if (!handled_by_edgehog && edgehog_device->user_event_cbk) {
        edgehog_device->user_event_cbk(*event, edgehog_device->user_cbk_user_data);
    }
}

static bool is_edgehog_interface(const char *interface_name)
{
    if (!interface_name) {
        return false;
    }
    // All Edgehog device manager interfaces use the "io.edgehog.devicemanager" namespace
    return (strncmp(interface_name, "io.edgehog.devicemanager", strlen("io.edgehog.devicemanager"))
        == 0);
}

static void handle_connected_event(edgehog_device_handle_t edgehog_device)
{
    EDGEHOG_LOG_DBG("Astarte device connected");
    edgehog_device->state = EDGEHOG_DEVICE_CONNECTED;

    if (!edgehog_device->initial_publish) {
        initial_publish(edgehog_device);
        edgehog_device->initial_publish = true;
    }

    edgehog_telemetry_t *telemetry = edgehog_device->telemetry;
    if (!edgehog_telemetry_is_running(telemetry)) {
        edgehog_result_t eres = edgehog_telemetry_start(edgehog_device);
        if (eres != EDGEHOG_RESULT_OK) {
            EDGEHOG_LOG_ERR("Unable to start Edgehog telemetry service");
        }
    }

#ifdef CONFIG_EDGEHOG_DEVICE_FILE_TRANSFER
    edgehog_ft_t *file_transfer = edgehog_device->file_transfer;
    if (!edgehog_ft_is_running(file_transfer)) {
        EDGEHOG_LOG_DBG("Starting the file transfer service.");
        edgehog_result_t eres = edgehog_ft_start(edgehog_device);
        if (eres != EDGEHOG_RESULT_OK) {
            EDGEHOG_LOG_ERR("Unable to start Edgehog file transfer service");
        }
    }
#endif
}

static void handle_disconnected_event(edgehog_device_handle_t edgehog_device)
{
    EDGEHOG_LOG_DBG("Astarte device disconnected");
    if (edgehog_device->state != EDGEHOG_DEVICE_STOPPED) {
        edgehog_device->state = EDGEHOG_DEVICE_STARTED;
    }
}

// NOLINTNEXTLINE(misc-unused-parameters)
static void handle_datastream_individual_event(edgehog_device_handle_t edgehog_device,
    astarte_device_datastream_individual_event_t *event, bool *handled_by_edgehog)
{
    EDGEHOG_LOG_DBG("Astarte datastream individual received");
    astarte_device_data_event_t base_event = event->base_event;
    if (!is_edgehog_interface(base_event.interface_name)) {
        *handled_by_edgehog = false;
        return;
    }

    *handled_by_edgehog = true;

    if ((strcmp(base_event.interface_name, io_edgehog_devicemanager_Commands.name) == 0)
        && (strcmp(base_event.path, COMMANDS_REQUEST_PATH) == 0)) {
        edgehog_result_t cmd_result = edgehog_command_event(event);
        if (cmd_result != EDGEHOG_RESULT_OK) {
            EDGEHOG_LOG_ERR("Unable to handle Command request");
        }
        return;
    }

#if DT_NODE_HAS_STATUS(EDGEHOG_LED_NODE, okay)
    if ((strcmp(base_event.interface_name, io_edgehog_devicemanager_LedBehavior.name) == 0)
        && (strcmp(base_event.path, "/indicator/behavior") == 0)) {
        edgehog_result_t led_result = edgehog_led_event(edgehog_device, event);
        if (led_result != EDGEHOG_RESULT_OK) {
            EDGEHOG_LOG_ERR("Unable to handle LED event request");
        }
    }
#endif
}

static void handle_datastream_object_event(edgehog_device_handle_t edgehog_device,
    astarte_device_datastream_object_event_t *event, bool *handled_by_edgehog)
{
    EDGEHOG_LOG_DBG("Astarte datastream object received");
    astarte_device_data_event_t base_event = event->base_event;

    if (!is_edgehog_interface(base_event.interface_name)) {
        *handled_by_edgehog = false;
        return;
    }

    *handled_by_edgehog = true;

    if (strcmp(base_event.interface_name, io_edgehog_devicemanager_OTARequest.name) == 0) {
        if (strcmp(base_event.path, OTA_REQUEST_PATH) != 0) {
            EDGEHOG_LOG_ERR("Received OTA request on incorrect common path: '%s'", base_event.path);
            return;
        }

        edgehog_result_t ota_result = edgehog_ota_event(edgehog_device, event);
        if (ota_result != EDGEHOG_RESULT_OK) {
            EDGEHOG_LOG_ERR("Unable to handle OTA update request");
        }
        return;
    }

#ifdef CONFIG_EDGEHOG_DEVICE_FILE_TRANSFER
    if (strcmp(base_event.interface_name, io_edgehog_devicemanager_fileTransfer_ServerToDevice.name)
        == 0) {
        EDGEHOG_LOG_INF("Received file transfer server to device event");
        if (strcmp(base_event.path, FT_REQUEST_PATH) != 0) {
            EDGEHOG_LOG_ERR(
                "Received file transfer request on incorrect common path: '%s'", base_event.path);
            return;
        }

        edgehog_result_t ft_result = edgehog_ft_server_to_device_event(edgehog_device, event);
        if (ft_result != EDGEHOG_RESULT_OK) {
            EDGEHOG_LOG_ERR("Unable to handle FT server to device request");
        }
        return;
    }

    if (strcmp(base_event.interface_name, io_edgehog_devicemanager_fileTransfer_DeviceToServer.name)
        == 0) {
        EDGEHOG_LOG_INF("Received file transfer device to server event");
        if (strcmp(base_event.path, FT_REQUEST_PATH) != 0) {
            EDGEHOG_LOG_ERR(
                "Received file transfer request on incorrect common path: '%s'", base_event.path);
            return;
        }

        edgehog_result_t ft_result = edgehog_ft_device_to_server_event(edgehog_device, event);
        if (ft_result != EDGEHOG_RESULT_OK) {
            EDGEHOG_LOG_ERR("Unable to handle FT device to server request");
        }
    }
#endif
}

static void handle_property_set_event(edgehog_device_handle_t edgehog_device,
    astarte_device_property_set_event_t *event, bool *handled_by_edgehog)
{
    EDGEHOG_LOG_DBG("Astarte property set received");
    astarte_device_data_event_t base_event = event->base_event;

    if (!is_edgehog_interface(base_event.interface_name)) {
        *handled_by_edgehog = false;
        return;
    }

    *handled_by_edgehog = true;

    if (strcmp(base_event.interface_name, io_edgehog_devicemanager_config_Telemetry.name) == 0) {
        edgehog_result_t eres
            = edgehog_telemetry_config_set_event(edgehog_device->telemetry, event);
        if (eres != EDGEHOG_RESULT_OK) {
            EDGEHOG_LOG_ERR("Unable to handle Telemetry set event request");
        }
    }
}

static void handle_property_unset_event(edgehog_device_handle_t edgehog_device,
    astarte_device_data_event_t *event, bool *handled_by_edgehog)
{

    EDGEHOG_LOG_DBG("Astarte property unset received");

    if (!is_edgehog_interface(event->interface_name)) {
        *handled_by_edgehog = false;
        return;
    }

    *handled_by_edgehog = true;

    if (strcmp(event->interface_name, io_edgehog_devicemanager_config_Telemetry.name) == 0) {
        edgehog_result_t eres
            = edgehog_telemetry_config_unset_event(edgehog_device->telemetry, event);
        if (eres != EDGEHOG_RESULT_OK) {
            EDGEHOG_LOG_ERR("Unable to handle Telemetry unset event request");
        }
    }
}

static void handle_error_event(
    edgehog_device_handle_t edgehog_device, astarte_device_error_event_t *event)
{
    EDGEHOG_LOG_ERR("Astarte error event received: %s (context: %s)",
        astarte_result_to_name(event->result), event->context ? event->context : "N/A");
    edgehog_device->astarte_error = event->result;
}
