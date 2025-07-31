/*
 * (C) Copyright 2024, SECO Mind Srl
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * This file contains the implementation of the telemetry service.
 * The telemetry for Edgehog devices is comprised of the following elements:
 * - A message queue used to communicate to the telemetry service the transmission requests
 * - A telemetry service task that waits for messages on the queue and when a new message is
 *   present it takes care of transmitting it.
 * - A set of telemetry entries, up to one for each type. Each telemetry entry can be scheduled
 *   at its defined frequency. When an entry is scheduled a zephyr kernel timer is created that
 *   automatically triggers transmission of the telemetry entry with the defined frequency. It
 *   triggers transmission by placing a new message in the telemetry message queue.
 */

#include "telemetry_private.h"

#include "edgehog_private.h"
#include "generated_interfaces.h"
#include "hardware_info.h"
#include "settings.h"
#include "storage_usage.h"
#include "system_status.h"

#include <stdlib.h>

#include <astarte_device_sdk/device.h>

#include "log.h"
EDGEHOG_LOG_MODULE_REGISTER(telemetry, CONFIG_EDGEHOG_DEVICE_TELEMETRY_LOG_LEVEL);

/************************************************
 *        Defines, constants and typedef        *
 ***********************************************/

#define TELEMETRY_SERVICE_THREAD_STACK_SIZE 2048
#define TELEMETRY_SERVICE_THREAD_PRIORITY 5
#define TELEMETRY_SERVICE_THREAD_RUNNING_BIT (1)
#define TELEMETRY_SERVICE_MSGQ_GET_TIMEOUT 100

// NOLINTBEGIN(cppcoreguidelines-avoid-non-const-global-variables)
K_THREAD_STACK_DEFINE(telemetry_service_stack_area, TELEMETRY_SERVICE_THREAD_STACK_SIZE);
// NOLINTEND(cppcoreguidelines-avoid-non-const-global-variables)

/************************************************
 *         Static functions declaration         *
 ***********************************************/

/**
 * @brief Schedule a new telemetry entry.
 *
 * @param[in] telemetry A valid telemetry instance.
 * @param[in] entry The telemetry entry to schedule.
 * @return EDGEHOG_RESULT_OK if successful, an edgehog_result_t otherwise.
 */
static edgehog_result_t schedule_entry(
    edgehog_telemetry_t *telemetry, edgehog_telemetry_entry_t *entry);
/**
 * @brief Parse a telemetry configuration event.
 *
 * @param[in] event The Astarte device event data.
 * @param[out] telemetry_type The telemetry type of the event.
 * @param[out] endpoint The enpoint of Astarte device data event.
 * @return EDGEHOG_RESULT_OK if successful, an edgehog_result_t otherwise.
 */
static edgehog_result_t parse_configuration_event(
    astarte_device_data_event_t *event, edgehog_telemetry_type_t *type, char **endpoint);
/**
 * @brief Get a telemetry type from an Astarte interface name.
 *
 * @param[in] type The type of the telemetry to find.
 * @return The telemetry type if the interface exists, EDGEHOG_TELEMETRY_INVALID otherwise.
 */
static edgehog_telemetry_type_t type_from_interface(const char *interface_name);
/**
 * @brief Remove a telemetry entry from scheduling.
 *
 * @param[in] entry The telemetry entry to remove from the scheduling.
 * @return EDGEHOG_RESULT_OK if successful, an edgehog_result_t otherwise.
 */
static edgehog_result_t unschedule_entry(edgehog_telemetry_entry_t *entry);
/**
 * @brief Check the existence of a telemetry type in the user provided configuration.
 *
 * @param[in] type The telemetry type to search.
 * @param[in] telemetry A valid telemetry instance, from which the configuration will be extracted.
 * @return True if the telemetry type exists, false otherwise.
 */
static bool type_is_in_config(edgehog_telemetry_type_t type, edgehog_telemetry_t *telemetry);
/**
 * @brief Get the telemetry type period from the user configuration.
 *
 * @param[in] type The telemetry type to search.
 * @param[in] telemetry A valid telemetry instance.
 * @return The periods in seconds if the telemetry type exists, -1 otherwise.
 */
static int64_t get_period_from_config(
    edgehog_telemetry_type_t type, edgehog_telemetry_t *telemetry);
/**
 * @brief Publish a telemetry based on the @p type provided.
 *
 * @param device A valid edgehog device handle.
 * @param type The type of telemetry to publish.
 */
static void publish_telemetry(edgehog_device_handle_t device, edgehog_telemetry_type_t type);

/************************************************
 *       Callbacks declaration/definition       *
 ***********************************************/

/**
 * @brief Entry point for the telemetry service thread.
 *
 * @details This thread will wait for a new message from the telemetry message queue. Each time it
 * receives  one it will trigger the pubblish of a telemetry entry of the received type.
 *
 * @param[in] device_ptr Pointer to a valid instance of an edgehog device.
 * @param[in] queue_ptr Pointer to the message queue instance.
 * @param[in] unused Unused parameter.
 */
static void telemetry_service_thread_entry_point(void *device_ptr, void *queue_ptr, void *unused)
{
    ARG_UNUSED(unused);
    edgehog_device_handle_t device = (edgehog_device_handle_t) device_ptr;
    struct k_msgq *msgq = (struct k_msgq *) queue_ptr;
    edgehog_telemetry_type_t type = EDGEHOG_TELEMETRY_INVALID;

    while (
        atomic_test_bit(&device->telemetry->thread_state, TELEMETRY_SERVICE_THREAD_RUNNING_BIT)) {
        if (k_msgq_get(msgq, &type, K_MSEC(TELEMETRY_SERVICE_MSGQ_GET_TIMEOUT)) == 0) {
            publish_telemetry(device, type);
        }
    }
}

/************************************************
 *         Global functions definitions         *
 ***********************************************/

edgehog_telemetry_t *edgehog_telemetry_new(edgehog_telemetry_config_t *configs, size_t configs_len)
{
    // Allocate space for the telemetry internal struct
    edgehog_telemetry_t *telemetry = calloc(1, sizeof(edgehog_telemetry_t));
    if (!telemetry) {
        EDGEHOG_LOG_ERR("Out of memory %s: %d", __FILE__, __LINE__);
        return NULL;
    }

    // Copy the provided settings to the telemetry internal struct
    telemetry->configs_len = configs_len;
    if (configs_len > 0) {
        telemetry->configs = calloc(telemetry->configs_len, sizeof(edgehog_telemetry_config_t));
        if (!telemetry->configs) {
            EDGEHOG_LOG_ERR("Out of memory %s: %d", __FILE__, __LINE__);
            goto error;
        }
        memcpy(telemetry->configs, configs, configs_len * sizeof(edgehog_telemetry_config_t));
    }

    // Load telemetries first from settings, then as a fallback from the provided configuration
    telemetry_entries_load_from_settings(telemetry->entries);
    telemetry_entries_load_from_config(configs, configs_len, telemetry->entries);

    return telemetry;

error:
    edgehog_telemetry_destroy(telemetry);
    return NULL;
}

edgehog_result_t edgehog_telemetry_start(edgehog_device_handle_t device)
{
    edgehog_telemetry_t *telemetry = device->telemetry;

    if (!telemetry) {
        EDGEHOG_LOG_ERR("Unable to start telemetry, reference is null");
        return EDGEHOG_RESULT_TELEMETRY_START_FAIL;
    }

    if (atomic_test_and_set_bit(&telemetry->thread_state, TELEMETRY_SERVICE_THREAD_RUNNING_BIT)) {
        EDGEHOG_LOG_ERR("Failed starting telemetry service as it's already running");
        return EDGEHOG_RESULT_TELEMETRY_START_FAIL;
    }

    k_msgq_init(&telemetry->msgq, telemetry->msgq_buffer, sizeof(edgehog_telemetry_type_t),
        EDGEHOG_TELEMETRY_LEN);

    k_tid_t thread_id = k_thread_create(&telemetry->thread, telemetry_service_stack_area,
        TELEMETRY_SERVICE_THREAD_STACK_SIZE, telemetry_service_thread_entry_point, (void *) device,
        (void *) &telemetry->msgq, NULL, TELEMETRY_SERVICE_THREAD_PRIORITY, 0, K_NO_WAIT);

    if (!thread_id) {
        EDGEHOG_LOG_ERR("Unable to start telemetry message thread");
        atomic_clear_bit(&telemetry->thread_state, TELEMETRY_SERVICE_THREAD_RUNNING_BIT);
        return EDGEHOG_RESULT_TELEMETRY_START_FAIL;
    }

    for (int i = 0; i < EDGEHOG_TELEMETRY_LEN; i++) {
        edgehog_telemetry_entry_t *entry = telemetry->entries[i];
        if (entry && entry->enable) {
            schedule_entry(telemetry, entry);
        }
    }

    return EDGEHOG_RESULT_OK;
}

edgehog_result_t edgehog_telemetry_config_set_event(
    edgehog_telemetry_t *telemetry, astarte_device_property_set_event_t *event)
{
    char *endpoint = NULL;
    edgehog_telemetry_type_t type = EDGEHOG_TELEMETRY_INVALID;
    edgehog_result_t res = parse_configuration_event(&event->base_event, &type, &endpoint);

    if (res != EDGEHOG_RESULT_OK) {
        return res;
    }

    if (!endpoint) {
        EDGEHOG_LOG_ERR("Unable to handle config telemetry update, endpoint empty");
        return EDGEHOG_RESULT_ASTARTE_ERROR;
    }

    edgehog_telemetry_entry_t *entry = telemetry_entry_get(type, telemetry->entries);

    if (!entry) {
        entry = telemetry_entry_new(type, TELEMETRY_UPDATE_DEFAULT, false);

        if (!entry) {
            return EDGEHOG_RESULT_TELEMETRY_START_FAIL;
        }

        telemetry_entry_set(entry, telemetry->entries);
    }

    astarte_data_t data = event->data;
    if (strcmp(endpoint, "enable") == 0) {
        if (data.tag == ASTARTE_MAPPING_TYPE_BOOLEAN) {
            entry->enable = data.data.boolean;
        }
    } else if (strcmp(endpoint, "periodSeconds") == 0) {
        if (data.tag == ASTARTE_MAPPING_TYPE_LONGINTEGER) {
            entry->period_seconds = data.data.longinteger;
        }
    }

    if (entry->enable) {
        return schedule_entry(telemetry, entry);
    }
    return unschedule_entry(entry);
}

edgehog_result_t edgehog_telemetry_config_unset_event(
    edgehog_telemetry_t *telemetry, astarte_device_data_event_t *event)
{
    char *endpoint = NULL;
    edgehog_telemetry_type_t type = EDGEHOG_TELEMETRY_INVALID;
    edgehog_result_t res = parse_configuration_event(event, &type, &endpoint);

    if (res != EDGEHOG_RESULT_OK) {
        return res;
    }

    if (!endpoint) {
        EDGEHOG_LOG_ERR("Unable to handle config telemetry update, endpoint empty");
        return EDGEHOG_RESULT_ASTARTE_ERROR;
    }

    edgehog_telemetry_entry_t *entry = telemetry_entry_get(type, telemetry->entries);

    if (!entry) {
        return EDGEHOG_RESULT_TELEMETRY_START_FAIL;
    }

    if (strcmp("enable", endpoint) == 0) {
        entry->enable = type_is_in_config(entry->type, telemetry);
    } else if (strcmp("periodSeconds", endpoint) == 0) {
        entry->period_seconds = get_period_from_config(entry->type, telemetry);
    }

    if (entry->enable) {
        return schedule_entry(telemetry, entry);
    }
    return unschedule_entry(entry);
}

edgehog_result_t edgehog_telemetry_stop(edgehog_telemetry_t *telemetry, k_timeout_t timeout)
{
    // Request the thread to self exit
    atomic_clear_bit(&telemetry->thread_state, TELEMETRY_SERVICE_THREAD_RUNNING_BIT);
    // Wait for the thread to self exit
    int res = k_thread_join(&telemetry->thread, timeout);
    switch (res) {
        case 0:
            return EDGEHOG_RESULT_OK;
        case -EAGAIN:
            return EDGEHOG_RESULT_TELEMETRY_STOP_TIMEOUT;
        default:
            return EDGEHOG_RESULT_INTERNAL_ERROR;
    }
}

void edgehog_telemetry_destroy(edgehog_telemetry_t *telemetry)
{
    for (int i = 0; i < EDGEHOG_TELEMETRY_LEN; i++) {
        if ((telemetry->entries[i])
            && (k_timer_remaining_get(&telemetry->entries[i]->timer) != 0)) {
            k_timer_stop(&telemetry->entries[i]->timer);
        }
        free(telemetry->entries[i]);
    }
    free(telemetry->configs);
    free(telemetry);
}

bool edgehog_telemetry_is_running(edgehog_telemetry_t *telemetry)
{
    if (!telemetry) {
        return false;
    }
    return atomic_test_bit(&telemetry->thread_state, TELEMETRY_SERVICE_THREAD_RUNNING_BIT);
}

/************************************************
 *         Static functions definitions         *
 ***********************************************/

static edgehog_result_t schedule_entry(
    edgehog_telemetry_t *telemetry, edgehog_telemetry_entry_t *entry)
{
    if (!entry) {
        EDGEHOG_LOG_ERR("Telemetry undefined");
        goto error;
    }

    if (entry->type <= EDGEHOG_TELEMETRY_INVALID || entry->type >= EDGEHOG_TELEMETRY_LEN) {
        EDGEHOG_LOG_ERR("Telemetry type %d invalid", entry->type);
        goto error;
    }

    if (entry->period_seconds <= 0) {
        EDGEHOG_LOG_WRN("Telemetry type %d invalid period", entry->type);
        goto error;
    }

    telemetry_entry_store(entry);

    if (!atomic_test_bit(&telemetry->thread_state, TELEMETRY_SERVICE_THREAD_RUNNING_BIT)) {
        return EDGEHOG_RESULT_OK;
    }

    struct k_timer *timer = &entry->timer;
    if (k_timer_remaining_get(timer) == 0) {

        // Note: the timer callback is set when the entry is created
        k_timer_user_data_set(timer, &telemetry->msgq);
        k_timer_start(timer, K_SECONDS(entry->period_seconds), K_SECONDS(entry->period_seconds));

        if (k_timer_remaining_get(timer) == 0) {
            EDGEHOG_LOG_WRN("The timer %d could not be set into the Active state", entry->type);
            k_timer_stop(timer);
            telemetry_entry_remove(entry->type, telemetry->entries);
            goto error;
        };

    } else {
        k_timer_stop(timer);
        k_timer_start(timer, K_SECONDS(entry->period_seconds), K_SECONDS(entry->period_seconds));
    }

    return EDGEHOG_RESULT_OK;

error:
    EDGEHOG_LOG_ERR("Unable to schedule new telemetry");
    return EDGEHOG_RESULT_TELEMETRY_START_FAIL;
}

static edgehog_result_t parse_configuration_event(
    astarte_device_data_event_t *event, edgehog_telemetry_type_t *type, char **endpoint)
{
    if (!event) {
        EDGEHOG_LOG_ERR("Unable to handle event, event undefined");
        return EDGEHOG_RESULT_ASTARTE_ERROR;
    }

    char *save_ptr = NULL;
    strtok_r((char *) event->path, "/", &save_ptr); // skip /request
    const char *interface_name = strtok_r(NULL, "/", &save_ptr);
    *endpoint = strtok_r(NULL, "/", &save_ptr);

    if (!interface_name || !endpoint) {
        EDGEHOG_LOG_ERR("Unable to handle config telemetry update, parameter empty");
        return EDGEHOG_RESULT_ASTARTE_ERROR;
    }

    edgehog_telemetry_type_t type_tmp = type_from_interface(interface_name);
    if (type_tmp == EDGEHOG_TELEMETRY_INVALID) {
        EDGEHOG_LOG_ERR("Unable to handle config telemetry update, telemetry type %s not supported",
            interface_name);
        return EDGEHOG_RESULT_ASTARTE_ERROR;
    }

    *type = type_tmp;

    return EDGEHOG_RESULT_OK;
}

static edgehog_telemetry_type_t type_from_interface(const char *interface_name)
{
    if (strcmp(interface_name, io_edgehog_devicemanager_HardwareInfo.name) == 0) {
        return EDGEHOG_TELEMETRY_HW_INFO;
    }
    if (strcmp(interface_name, io_edgehog_devicemanager_WiFiScanResults.name) == 0) {
        return EDGEHOG_TELEMETRY_WIFI_SCAN;
    }
    if (strcmp(interface_name, io_edgehog_devicemanager_SystemStatus.name) == 0) {
        return EDGEHOG_TELEMETRY_SYSTEM_STATUS;
    }
    if (strcmp(interface_name, io_edgehog_devicemanager_StorageUsage.name) == 0) {
        return EDGEHOG_TELEMETRY_STORAGE_USAGE;
    }
    return EDGEHOG_TELEMETRY_INVALID;
}

static edgehog_result_t unschedule_entry(edgehog_telemetry_entry_t *entry)
{
    if (!entry) {
        EDGEHOG_LOG_ERR("Telemetry undefined");
        return EDGEHOG_RESULT_TELEMETRY_STOP_FAIL;
    }

    if (entry->type <= EDGEHOG_TELEMETRY_INVALID || entry->type >= EDGEHOG_TELEMETRY_LEN) {
        EDGEHOG_LOG_ERR("Telemetry type invalid %d", entry->type);
        return EDGEHOG_RESULT_TELEMETRY_STOP_FAIL;
    }

    telemetry_entry_store(entry);

    if (k_timer_remaining_get(&entry->timer) != 0) {
        k_timer_stop(&entry->timer);
    }

    return EDGEHOG_RESULT_OK;
}

static bool type_is_in_config(edgehog_telemetry_type_t type, edgehog_telemetry_t *telemetry)
{
    for (int i = 0; i < telemetry->configs_len; i++) {
        if (type == telemetry->configs[i].type) {
            return true;
        }
    }
    return false;
}

static int64_t get_period_from_config(edgehog_telemetry_type_t type, edgehog_telemetry_t *telemetry)
{
    for (int i = 0; i < telemetry->configs_len; i++) {
        if (type == telemetry->configs[i].type) {
            return telemetry->configs[i].period_seconds;
        }
    }
    return -1;
}

static void publish_telemetry(edgehog_device_handle_t device, edgehog_telemetry_type_t type)
{
    switch (type) {
        case EDGEHOG_TELEMETRY_HW_INFO:
            publish_hardware_info(device);
            break;
#ifdef CONFIG_WIFI
        case EDGEHOG_TELEMETRY_WIFI_SCAN:
            edgehog_wifi_scan_start(device);
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
