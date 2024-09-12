/*
 * (C) Copyright 2024, SECO Mind Srl
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "telemetry_private.h"

#include "edgehog_private.h"
#include "generated_interfaces.h"
#include "settings.h"

#include <stdlib.h>

#include <zephyr/kernel.h>

#include <astarte_device_sdk/device.h>

#include "log.h"
EDGEHOG_LOG_MODULE_REGISTER(telemetry, CONFIG_EDGEHOG_DEVICE_TELEMETRY_LOG_LEVEL);

/************************************************
 *        Defines, constants and typedef        *
 ***********************************************/

#define SETTINGS_TELEMETRY_KEY "telemetry"
#define SETTINGS_TELEMETRY_KEY_LEN strlen(SETTINGS_TELEMETRY_KEY)
#define SETTINGS_TELEMETRY_PERIODS_KEY "periods"
#define SETTINGS_TELEMETRY_ENABLE_KEY "enable"
#define SETTINGS_TELEMETRY_TYPE_KEY_SIZE 1

#define TELEMETRY_UPDATE_DEFAULT 0

#define MESSAGE_QUEUE_STACK_SIZE 2048
#define MESSAGE_QUEUE_PRIORITY 5
#define TELEMETRY_RUN_BIT (1)

// NOLINTBEGIN(cppcoreguidelines-avoid-non-const-global-variables)
K_THREAD_STACK_DEFINE(message_queue_stack_area, MESSAGE_QUEUE_STACK_SIZE);
// NOLINTEND(cppcoreguidelines-avoid-non-const-global-variables)

/** @brief Data struct for a telemetry instance. */
typedef struct
{
    /** @brief Type of telemetry. */
    telemetry_type_t type;
    /** @brief Interval of period in seconds. */
    int64_t period_seconds;
    /** @brief Enables the telemetry. */
    bool enable;
    /** @brief Struct of telemetry timer. */
    struct k_timer timer;
} telemetry_t;

/**
 * @brief Telemtry data.
 *
 * @details Defines the Telemetry data used by Edgehog telemetry.
 * `telemetries_config` are separated from `telemetries` because if the device receives an unset on
 * enable or period_seconds endpoint, It has to return to the previous state configured in the
 * device.
 *
 */
struct edgehog_telemetry_data
{
    /** @brief Telemetries configured by the user. */
    edgehog_device_telemetry_config_t *telemetries_config;
    /** @brief Len of telemetries configured. */
    size_t telemetries_config_len;
    /** @brief Running telemetries. */
    telemetry_t *telemetries[EDGEHOG_TELEMETRY_LEN];
    /** @brief Thread peeks message from queue. */
    struct k_thread message_queue_thread;
    /** @brief Message queue. */
    struct k_msgq message_queue;
    /** @brief Ring buffer that holds queued messages. */
    char queue_buffer[EDGEHOG_TELEMETRY_LEN * sizeof(telemetry_type_t)];
    /** @brief Ring buffer that holds queued messages. */
    atomic_t telemetry_run_state;
};

/************************************************
 *         Static functions declaration         *
 ***********************************************/

/**
 * @brief Load telemetries from settings.
 *
 * @param[out] telemetries Array of telemetries extracted from settings.
 *
 * @return EDGEHOG_RESULT_OK if the load telemetries is handled successfully, an edgehog_result_t
 * otherwise.
 */
static edgehog_result_t load_telemetries_from_settings(telemetry_t **telemetries);

/**
 * @brief Load telemetries from config.
 *
 * @param[in] telemetries_config Array of telemetries configured by the user.
 * @param[in] telemetry_config_len The length of the configure array.
 * @param[out] telemetries Array of telemetries extracted from config.
 *
 * @return EDGEHOG_RESULT_OK if the load is handled successfully, an edgehog_result_t
 * otherwise.
 */
static void load_telemetries_from_config(edgehog_device_telemetry_config_t *telemetries_config,
    size_t telemetry_config_len, telemetry_t **telemetries);

/**
 * @brief Create a new instance of telemetry_t.
 *
 * @param[in] type The telemetry_type_t to create.
 * @param[in] period_seconds The interval in seconds.
 * @param[in] enable The enable flag.
 *
 * @return The telemetry_t if the creation was successfully, NULL otherwise.
 */
static telemetry_t *telemetry_new(telemetry_type_t type, int64_t period_seconds, bool enable);

/**
 * @brief Check the existence of telemetry type in the telemetries array.
 *
 * @param[in] type The type of telemtry to check.
 * @param[in] telemetries The array of telemetries.
 *
 * @return True if the telemetry exists, false otherwise.
 */
static bool exist(telemetry_type_t type, telemetry_t **telemetries);

/**
 * @brief Get a telemetry_t from the telemetries array.
 *
 * @param[in] type The type of telemtry to get.
 * @param[in] telemetries The array of telemetries.
 *
 * @return The telemtry_t if the telemetry exists, NULL otherwise.
 */
static telemetry_t *get_telemetry(telemetry_type_t type, telemetry_t **telemetries);

/**
 * @brief Get a telemtry_type_t from an Astarte interface name.
 *
 * @param[in] type The type of telemtry to find.
 *
 * @return The telemtry_type_t if the telemetry exists, EDGEHOG_TELEMETRY_INVALID otherwise.
 */
static telemetry_type_t get_telemetry_type(const char *interface_name);

/**
 * @brief Get the telemetry array index for specific telemetry type.
 *
 * @param[in] type The type of telemtry.
 *
 * @return The index of array, -1 otherwise.
 */
static int get_telemetry_index(telemetry_type_t type);

/**
 * @brief Get the telemetry period from the config.
 *
 * @param[in] telemetry_type The telemetry_type_t to search.
 * @param[in] edgehog_telemetry The struct of configured telemetries.
 *
 * @return The periods in seconds if the telemetry exists, -1 otherwise.
 */
static int64_t get_telemetry_period_from_config(
    telemetry_type_t telemetry_type, edgehog_telemetry_t *edgehog_telemetry);

/**
 * @brief Set a telemtry_t to the telemtries.
 *
 * @details If the telemetry to be set is already existing, the previous will be freed.
 *
 * @param[in] telemetry A valid telemtry pointer to set.
 * @param[inout] telemetries The array into set the telemetry.
 */
static void set_telemetry(telemetry_t *telemetry, telemetry_t **telemetries);

/**
 * @brief Store a telemtry_t to the settings.
 *
 * @param[in] telemetry The telemtry to set.
 *
 * @return EDGEHOG_RESULT_OK if the store was successfully, an edgehog_result_t
 * otherwise.
 */
static edgehog_result_t store_telemetry(telemetry_t *telemetry);

/**
 * @brief Remove a telemetry_type_t from the telemetries.
 *
 * @param[in] type The telemetry_type_t to remove.
 * @param[inout] telemetries The array of telemetries.
 */
static void remove_telemetry(telemetry_type_t type, telemetry_t **telemetries);

/**
 * @brief Check the exstince of telemetry_type_t in the config.
 *
 * @param[in] telemetry_type The telemetry_type_t to search.
 * @param[in] edgehog_telemetry The struct of configured telemetries.
 *
 * @return True if the telemetry exists, false otherwise.
 */
static bool telemetry_type_is_present_in_config(
    telemetry_type_t telemetry_type, edgehog_telemetry_t *edgehog_telemetry);

/**
 * @brief Parse an Astarte device data event.
 *
 * @param[in] event The astarte device data event.
 * @param[out] telemetry_type The telemetry type of event.
 * @param[out] endpoint The enpoint of astarte device event.
 *
 * @return EDGEHOG_RESULT_OK if the parsing was successfully, an edgehog_result_t
 * otherwise.
 */
static edgehog_result_t parse_config_event(
    astarte_device_data_event_t *event, telemetry_type_t *type, char **endpoint);

/**
 * @brief Schedule a new telemetry.
 *
 * @param[in] telemetry The telemetry_t to schedule.
 * @param[in] device A valid handle of Edgehog device.
 *
 * @return EDGEHOG_RESULT_OK if the schedule was successfully, an edgehog_result_t
 * otherwise.
 */
static edgehog_result_t telemetry_schedule(telemetry_t *telemetry, edgehog_device_handle_t device);

/**
 * @brief Stop a telemetry.
 *
 * @param[in] telemetry The telemetry_t to schedule.
 *
 * @return EDGEHOG_RESULT_OK if the telemetry has been successfully stopped, an edgehog_result_t
 * otherwise.
 */
static edgehog_result_t telemetry_stop(telemetry_t *telemetry);

/************************************************
 *       Callbacks declaration/definition       *
 ***********************************************/

/**
 * @brief Handle telemetry settings loading.
 *
 * @param[in] key The name with skipped part that was used as name in handler registration.
 * @param[in] len The size of the data found in the backend.
 * @param[in] read_cb Function provided to read the data from the backend.
 * @param[inout] cb_arg Arguments for the read function provided by the backend.
 * @param[inout] param Parameter given to the settings_load_subtree_direct function.
 *
 * @return When nonzero value is returned, further subtree searching is stopped.
 */
static int telemetry_settings_loader(
    const char *key, size_t len, settings_read_cb read_cb, void *cb_arg, void *param)
{
    ARG_UNUSED(len);

    const char *next = NULL;
    telemetry_t **telemetries = (telemetry_t **) param;

    size_t next_len = settings_name_next(key, &next);

    if (next_len == 0 || !next) {
        return -ENOENT;
    }

    const int base = 10;
    telemetry_type_t type = (telemetry_type_t) strtol(&key[0], NULL, base);

    if (type <= EDGEHOG_TELEMETRY_INVALID || type >= EDGEHOG_TELEMETRY_LEN) {
        return -ENOENT;
    }

    telemetry_t *telemetry = get_telemetry(type, telemetries);
    if (!telemetry) {
        telemetry = telemetry_new(type, TELEMETRY_UPDATE_DEFAULT, false);

        if (!telemetry) {
            return -ENOENT;
        }

        set_telemetry(telemetry, telemetries);
    }

    next_len = settings_name_next(key, &next);

    if (!next) {
        return 0;
    }

    if (strncmp(next, SETTINGS_TELEMETRY_PERIODS_KEY, next_len) == 0) {
        int res = read_cb(cb_arg, &telemetry->period_seconds, sizeof(telemetry->period_seconds));
        if (res < 0) {
            EDGEHOG_LOG_ERR("Unable to read telemetry periods from settings: %d", res);
            return res;
        }
        return 0;
    }

    if (strncmp(next, SETTINGS_TELEMETRY_ENABLE_KEY, next_len) == 0) {
        int res = read_cb(cb_arg, &telemetry->enable, sizeof(telemetry->enable));
        if (res < 0) {
            EDGEHOG_LOG_ERR("Unable to read telemetry enable from settings: %d", res);
            return res;
        }
        return 0;
    }

    return -ENOENT;
}

/**
 * @brief Function to invoke each time the timer expires.
 *
 * @param[in] timer Address of timer.
 */
static void telemetry_expiry_fn(struct k_timer *timer)
{
    telemetry_t *telemetry = CONTAINER_OF(timer, telemetry_t, timer);
    struct k_msgq *msgq = (struct k_msgq *) k_timer_user_data_get(timer);

    k_msgq_put(msgq, &telemetry->type, K_NO_WAIT);
}

/**
 * @brief Message queue thread entry point.
 */
static void message_queue_entry_point(void *device_ptr, void *queue_ptr, void *unused)
{
    ARG_UNUSED(unused);
    edgehog_device_handle_t edgehog_device = (edgehog_device_handle_t) device_ptr;
    struct k_msgq *msgq = (struct k_msgq *) queue_ptr;
    telemetry_type_t type = EDGEHOG_TELEMETRY_INVALID;

    while (true) {
        int res = k_msgq_get(msgq, &type, K_FOREVER);
        if (res == 0) {
            edgehog_device_publish_telemetry(edgehog_device, type);
        } else {
            EDGEHOG_LOG_ERR("Unable to receive a message from a message queue: %d", res);
        }
    }
}

/************************************************
 *         Global functions definitions         *
 ***********************************************/

edgehog_telemetry_t *edgehog_telemetry_new(
    edgehog_device_telemetry_config_t *telemetry_config, size_t telemetry_config_len)
{

    edgehog_telemetry_t *edgehog_telemetry = calloc(1, sizeof(edgehog_telemetry_t));

    if (!edgehog_telemetry) {
        EDGEHOG_LOG_ERR("Out of memory %s: %d", __FILE__, __LINE__);
        return NULL;
    }

    edgehog_telemetry->telemetries_config_len = telemetry_config_len;

    if (telemetry_config_len > 0) {
        edgehog_telemetry->telemetries_config = calloc(
            edgehog_telemetry->telemetries_config_len, sizeof(edgehog_device_telemetry_config_t));
        if (!edgehog_telemetry->telemetries_config) {
            EDGEHOG_LOG_ERR("Out of memory %s: %d", __FILE__, __LINE__);
            goto error;
        }
        memcpy(edgehog_telemetry->telemetries_config, telemetry_config,
            telemetry_config_len * sizeof(edgehog_device_telemetry_config_t));
    }

    load_telemetries_from_settings(edgehog_telemetry->telemetries);
    load_telemetries_from_config(
        telemetry_config, telemetry_config_len, edgehog_telemetry->telemetries);

    return edgehog_telemetry;

error:
    edgehog_telemetry_destroy(edgehog_telemetry);
    return NULL;
}

edgehog_result_t edgehog_telemetry_start(
    edgehog_device_handle_t edgehog_device, edgehog_telemetry_t *edgehog_telemetry)
{

    if (!edgehog_telemetry) {
        EDGEHOG_LOG_ERR("Unable to start telemetry, reference is null");
        return EDGEHOG_RESULT_TELEMETRY_START_FAIL;
    }

    if (atomic_test_and_set_bit(&edgehog_telemetry->telemetry_run_state, TELEMETRY_RUN_BIT)) {
        EDGEHOG_LOG_ERR("Unable to set TELEMETRY RUN BIT");
        return EDGEHOG_RESULT_TELEMETRY_START_FAIL;
    }

    k_msgq_init(&edgehog_telemetry->message_queue, edgehog_telemetry->queue_buffer,
        sizeof(telemetry_type_t), EDGEHOG_TELEMETRY_LEN);

    k_tid_t thread_id
        = k_thread_create(&edgehog_telemetry->message_queue_thread, message_queue_stack_area,
            MESSAGE_QUEUE_STACK_SIZE, message_queue_entry_point, (void *) edgehog_device,
            (void *) &edgehog_telemetry->message_queue, NULL, MESSAGE_QUEUE_PRIORITY, 0, K_NO_WAIT);

    if (!thread_id) {
        EDGEHOG_LOG_ERR("Unable to start telemetry message thread");
        atomic_clear_bit(&edgehog_telemetry->telemetry_run_state, TELEMETRY_RUN_BIT);
        return EDGEHOG_RESULT_TELEMETRY_START_FAIL;
    }

    for (int i = 0; i < EDGEHOG_TELEMETRY_LEN; i++) {
        telemetry_t *telemetry = edgehog_telemetry->telemetries[i];
        if (telemetry && telemetry->enable) {
            telemetry_schedule(telemetry, edgehog_device);
        }
    }

    return EDGEHOG_RESULT_OK;
}

edgehog_result_t edgehog_telemetry_config_set_event(
    edgehog_device_handle_t edgehog_dev, astarte_device_property_set_event_t *event)
{

    char *endpoint = NULL;
    telemetry_type_t type = EDGEHOG_TELEMETRY_INVALID;
    edgehog_result_t res = parse_config_event(&event->data_event, &type, &endpoint);

    if (res != EDGEHOG_RESULT_OK) {
        return res;
    }

    if (!endpoint) {
        EDGEHOG_LOG_ERR("Unable to handle config telemetry update, endpoint empty");
        return EDGEHOG_RESULT_ASTARTE_ERROR;
    }

    telemetry_t *telemetry = get_telemetry(type, edgehog_dev->edgehog_telemetry->telemetries);

    if (!telemetry) {
        telemetry = telemetry_new(type, TELEMETRY_UPDATE_DEFAULT, false);

        if (!telemetry) {
            return EDGEHOG_RESULT_TELEMETRY_START_FAIL;
        }

        set_telemetry(telemetry, edgehog_dev->edgehog_telemetry->telemetries);
    }

    astarte_individual_t value = event->individual;
    if (strcmp(endpoint, "enable") == 0) {
        if (value.tag == ASTARTE_MAPPING_TYPE_BOOLEAN) {
            telemetry->enable = value.data.boolean;
        }
    } else if (strcmp(endpoint, "periodSeconds") == 0) {
        if (value.tag == ASTARTE_MAPPING_TYPE_LONGINTEGER) {
            telemetry->period_seconds = value.data.longinteger;
        }
    }

    if (telemetry->enable) {
        return telemetry_schedule(telemetry, edgehog_dev);
    }
    return telemetry_stop(telemetry);
}

edgehog_result_t edgehog_telemetry_config_unset_event(
    edgehog_device_handle_t edgehog_dev, astarte_device_data_event_t *event)
{
    char *endpoint = NULL;
    telemetry_type_t type = EDGEHOG_TELEMETRY_INVALID;
    edgehog_result_t res = parse_config_event(event, &type, &endpoint);

    if (res != EDGEHOG_RESULT_OK) {
        return res;
    }

    if (!endpoint) {
        EDGEHOG_LOG_ERR("Unable to handle config telemetry update, endpoint empty");
        return EDGEHOG_RESULT_ASTARTE_ERROR;
    }

    telemetry_t *telemetry = get_telemetry(type, edgehog_dev->edgehog_telemetry->telemetries);

    if (!telemetry) {
        return EDGEHOG_RESULT_TELEMETRY_START_FAIL;
    }

    if (strcmp("enable", endpoint) == 0) {
        telemetry->enable
            = telemetry_type_is_present_in_config(telemetry->type, edgehog_dev->edgehog_telemetry);
    } else if (strcmp("periodSeconds", endpoint) == 0) {
        telemetry->period_seconds
            = get_telemetry_period_from_config(telemetry->type, edgehog_dev->edgehog_telemetry);
    }

    if (telemetry->enable) {
        return telemetry_schedule(telemetry, edgehog_dev);
    }
    return telemetry_stop(telemetry);
}

edgehog_result_t edgehog_telemetry_destroy(edgehog_telemetry_t *edgehog_telemetry)
{
    for (int i = 0; i < EDGEHOG_TELEMETRY_LEN; i++) {
        telemetry_t *telemetry = edgehog_telemetry->telemetries[i];
        free(telemetry);
        edgehog_telemetry->telemetries[i] = NULL;
    }

    for (int i = 0; i < edgehog_telemetry->telemetries_config_len; i++) {
        edgehog_device_telemetry_config_t *telemetry = edgehog_telemetry->telemetries_config;
        free(telemetry);
    }

    free(edgehog_telemetry);

    return EDGEHOG_RESULT_OK;
}

/************************************************
 *         Static functions definitions         *
 ***********************************************/

static edgehog_result_t load_telemetries_from_settings(telemetry_t **telemetries)
{
    return edgehog_settings_load(
        SETTINGS_TELEMETRY_KEY, telemetry_settings_loader, (void *) telemetries);
}

static void load_telemetries_from_config(edgehog_device_telemetry_config_t *telemetries_config,
    size_t telemetry_config_len, telemetry_t **telemetries)
{
    for (int i = 0; i < telemetry_config_len; i++) {
        edgehog_device_telemetry_config_t telemetry_config = telemetries_config[i];
        if (!exist(telemetry_config.type, telemetries)) {
            telemetry_t *telemetry
                = telemetry_new(telemetry_config.type, telemetry_config.period_seconds, true);
            if (telemetry) {
                set_telemetry(telemetry, telemetries);
            }
        }
    }
}

static telemetry_t *telemetry_new(telemetry_type_t type, int64_t period_seconds, bool enable)
{
    telemetry_t *telemetry = (telemetry_t *) calloc(1, sizeof(telemetry_t));

    if (!telemetry) {
        EDGEHOG_LOG_ERR("Out of memory %s: %d", __FILE__, __LINE__);
        return NULL;
    }

    telemetry->type = type;
    telemetry->period_seconds = period_seconds;
    telemetry->enable = enable;
    k_timer_init(&telemetry->timer, telemetry_expiry_fn, NULL);

    return telemetry;
}

static telemetry_type_t get_telemetry_type(const char *interface_name)
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

static int get_telemetry_index(telemetry_type_t type)
{
    if (type > EDGEHOG_TELEMETRY_INVALID && type < EDGEHOG_TELEMETRY_LEN) {
        return (int) type - 1;
    }
    return -1;
}

static bool exist(telemetry_type_t type, telemetry_t **telemetries)
{
    int telemetry_idx = get_telemetry_index(type);

    if (telemetry_idx < 0) {
        EDGEHOG_LOG_ERR("Invalid telemetry index %d", type);
        return false;
    }

    telemetry_t *telemetry = telemetries[telemetry_idx];
    if (telemetry) {
        return true;
    }

    return false;
}

static telemetry_t *get_telemetry(telemetry_type_t type, telemetry_t **telemetries)
{
    int telemetry_idx = get_telemetry_index(type);

    if (telemetry_idx < 0) {
        EDGEHOG_LOG_ERR("Invalid telemetry index %d", type);
        return NULL;
    }

    return telemetries[telemetry_idx];
}

static int64_t get_telemetry_period_from_config(
    telemetry_type_t telemetry_type, edgehog_telemetry_t *edgehog_telemetry)
{
    for (int i = 0; i < edgehog_telemetry->telemetries_config_len; i++) {
        if (telemetry_type == edgehog_telemetry->telemetries_config[i].type) {
            return edgehog_telemetry->telemetries_config[i].period_seconds;
        }
    }
    return -1;
}

static void set_telemetry(telemetry_t *telemetry, telemetry_t **telemetries)
{
    int telemetry_idx = get_telemetry_index(telemetry->type);

    if (telemetry_idx < 0) {
        EDGEHOG_LOG_ERR("Invalid telemetry index %d", telemetry->type);
        return;
    }

    telemetry_t *current_telemetry = telemetries[telemetry_idx];
    free(current_telemetry);
    telemetries[telemetry_idx] = telemetry;
}

static void remove_telemetry(telemetry_type_t type, telemetry_t **telemetries)
{
    int telemetry_idx = get_telemetry_index(type);

    if (telemetry_idx < 0) {
        EDGEHOG_LOG_ERR("Invalid telemetry index %d", type);
        return;
    }

    telemetry_t *current_telemetry = telemetries[telemetry_idx];
    free(current_telemetry);
    telemetries[telemetry_idx] = NULL;
}

static edgehog_result_t store_telemetry(telemetry_t *telemetry)
{
    size_t type_key_len = SETTINGS_TELEMETRY_KEY_LEN + 2;
    char type_key[type_key_len + 1];

    int snprintf_rc
        = snprintf(type_key, type_key_len + 1, SETTINGS_TELEMETRY_KEY "/%d", telemetry->type);

    if (snprintf_rc != type_key_len) {
        EDGEHOG_LOG_ERR("Failure in formatting the Telemetry type key.");
        return EDGEHOG_RESULT_TELEMETRY_STORE_FAIL;
    }

    edgehog_result_t res = edgehog_settings_save(type_key, SETTINGS_TELEMETRY_PERIODS_KEY,
        &telemetry->period_seconds, sizeof(telemetry->period_seconds));

    if (res != EDGEHOG_RESULT_OK) {
        return res;
    }

    return edgehog_settings_save(
        type_key, SETTINGS_TELEMETRY_ENABLE_KEY, &telemetry->enable, sizeof(telemetry->enable));
}

static bool telemetry_type_is_present_in_config(
    telemetry_type_t telemetry_type, edgehog_telemetry_t *edgehog_telemetry)
{
    for (int i = 0; i < edgehog_telemetry->telemetries_config_len; i++) {
        if (telemetry_type == edgehog_telemetry->telemetries_config[i].type) {
            return true;
        }
    }
    return false;
}

static edgehog_result_t parse_config_event(
    astarte_device_data_event_t *event, telemetry_type_t *type, char **endpoint)
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

    telemetry_type_t telemetry_type = get_telemetry_type(interface_name);
    if (telemetry_type == EDGEHOG_TELEMETRY_INVALID) {
        EDGEHOG_LOG_ERR("Unable to handle config telemetry update, telemetry type %s not supported",
            interface_name);
        return EDGEHOG_RESULT_ASTARTE_ERROR;
    }

    *type = telemetry_type;

    return EDGEHOG_RESULT_OK;
}

static edgehog_result_t telemetry_schedule(telemetry_t *telemetry, edgehog_device_handle_t device)
{
    if (!telemetry) {
        EDGEHOG_LOG_ERR("Telemetry undefined");
        goto error;
    }

    if (telemetry->type <= EDGEHOG_TELEMETRY_INVALID || telemetry->type >= EDGEHOG_TELEMETRY_LEN) {
        EDGEHOG_LOG_ERR("Telemetry type %d invalid", telemetry->type);
        goto error;
    }

    if (telemetry->period_seconds <= 0) {
        EDGEHOG_LOG_WRN("Telemetry type %d invalid period", telemetry->type);
        goto error;
    }

    store_telemetry(telemetry);

    if (!atomic_test_bit(&device->edgehog_telemetry->telemetry_run_state, TELEMETRY_RUN_BIT)) {
        return EDGEHOG_RESULT_OK;
    }

    if (k_timer_remaining_get(&telemetry->timer) == 0) {

        k_timer_user_data_set(&telemetry->timer, &device->edgehog_telemetry->message_queue);
        k_timer_start(&telemetry->timer, K_SECONDS(telemetry->period_seconds),
            K_SECONDS(telemetry->period_seconds));

        if (k_timer_remaining_get(&telemetry->timer) == 0) {
            EDGEHOG_LOG_WRN("The timer %d could not be set into the Active state", telemetry->type);
            k_timer_stop(&telemetry->timer);
            remove_telemetry(telemetry->type, device->edgehog_telemetry->telemetries);
            goto error;
        };

    } else {
        k_timer_stop(&telemetry->timer);
        k_timer_start(&telemetry->timer, K_SECONDS(telemetry->period_seconds),
            K_SECONDS(telemetry->period_seconds));
    }

    return EDGEHOG_RESULT_OK;

error:
    EDGEHOG_LOG_ERR("Unable to schedule new telemetry");
    return EDGEHOG_RESULT_TELEMETRY_START_FAIL;
}

static edgehog_result_t telemetry_stop(telemetry_t *telemetry)
{
    if (!telemetry) {
        EDGEHOG_LOG_ERR("Telemetry undefined");
        return EDGEHOG_RESULT_TELEMETRY_STOP_FAIL;
    }

    if (telemetry->type <= EDGEHOG_TELEMETRY_INVALID || telemetry->type >= EDGEHOG_TELEMETRY_LEN) {
        EDGEHOG_LOG_ERR("Telemetry type invalid %d", telemetry->type);
        return EDGEHOG_RESULT_TELEMETRY_STOP_FAIL;
    }

    store_telemetry(telemetry);

    if (k_timer_remaining_get(&telemetry->timer) != 0) {
        k_timer_stop(&telemetry->timer);
    }

    return EDGEHOG_RESULT_OK;
}
