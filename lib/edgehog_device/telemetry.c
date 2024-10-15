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

#define MSGQ_THREAD_STACK_SIZE 2048
#define MSGQ_THREAD_PRIORITY 5
#define MSGQ_THREAD_RUNNING_BIT (1)
#define MSGQ_GET_TIMEOUT 100

// NOLINTBEGIN(cppcoreguidelines-avoid-non-const-global-variables)
K_THREAD_STACK_DEFINE(message_queue_stack_area, MSGQ_THREAD_STACK_SIZE);
// NOLINTEND(cppcoreguidelines-avoid-non-const-global-variables)

/** @brief Data struct for a telemetry entry. */
typedef struct
{
    /** @brief Type of telemetry. */
    edgehog_telemetry_type_t type;
    /** @brief Interval of period in seconds. */
    int64_t period_seconds;
    /** @brief Enables the telemetry. */
    bool enable;
    /** @brief Struct of telemetry timer. */
    struct k_timer timer;
} telemetry_entry_t;

/**
 * @brief Data struct for a telemetry instance.
 *
 * @details Defines the Telemetry data used by Edgehog telemetry.
 * The two arrays `configs` and `entries` are separated because following a set-unset cycle
 * each telemetry entry has to return to the initially configured state.
 *
 */
struct edgehog_telemetry_data
{
    /** @brief Base configurations provided by the user. */
    edgehog_telemetry_config_t *configs;
    /** @brief Length of base configurations. */
    size_t configs_len;
    /** @brief Telemetry entries list. */
    telemetry_entry_t *entries[EDGEHOG_TELEMETRY_LEN];
    /** @brief Message queue. */
    struct k_msgq msgq;
    /** @brief Ring buffer that holds queued messages. */
    char msgq_buffer[EDGEHOG_TELEMETRY_LEN * sizeof(edgehog_telemetry_type_t)];
    /** @brief Thread that peeks message from queue. */
    struct k_thread msgq_thread;
    /** @brief Run state for the message queue thread. */
    atomic_t msgq_thread_state;
};

/************************************************
 *         Static functions declaration         *
 ***********************************************/

/**
 * @brief Load telemetry entries from settings.
 *
 * @param[out] entries Array of telemetry entries extracted from settings.
 *
 * @return EDGEHOG_RESULT_OK if successful, an edgehog_result_t otherwise.
 */
static edgehog_result_t load_entries_from_settings(telemetry_entry_t **entries);

/**
 * @brief Load telemetry entries from a list of telemetry base configurations.
 *
 * @note This function will load a telemetry entry only if it's not already in the @p entries array.
 *
 * @param[in] configs Array of configurations provided by the user.
 * @param[in] configs_len The length of the @p configs array.
 * @param[inout] entries Array of entries extracted from config.
 *
 * @return EDGEHOG_RESULT_OK if successful, an edgehog_result_t otherwise.
 */
static void load_entries_from_config(
    edgehog_telemetry_config_t *configs, size_t configs_len, telemetry_entry_t **entries);

/**
 * @brief Create a new instance of the a telemetry entry.
 *
 * @param[in] type The type of the telemetry entry to create.
 * @param[in] period_seconds The interval in seconds for the telemetry entry.
 * @param[in] enable The enable flag.
 *
 * @return The telemetry entry if the creation was successfully, NULL otherwise.
 */
static telemetry_entry_t *telemetry_entry_new(
    edgehog_telemetry_type_t type, int64_t period_seconds, bool enable);

/**
 * @brief Check the existence of telemetry type in the entries array.
 *
 * @param[in] type The type of telemtry to check.
 * @param[in] entries The array of entries.
 *
 * @return True if the telemetry exists, false otherwise.
 */
static bool telemetry_entry_exist(edgehog_telemetry_type_t type, telemetry_entry_t **entries);

/**
 * @brief Get a telemetry entry from the entries array.
 *
 * @param[in] type The type of telemetry entry to get.
 * @param[in] entries The array of telemetry entries.
 *
 * @return The entry if it exists, NULL otherwise.
 */
static telemetry_entry_t *get_telemetry_entry(
    edgehog_telemetry_type_t type, telemetry_entry_t **entries);

/**
 * @brief Get a telemetry entry type from an Astarte interface name.
 *
 * @param[in] type The type of the telemetry entry to find.
 *
 * @return The telemetry entry type if the entry exists, EDGEHOG_TELEMETRY_INVALID otherwise.
 */
static edgehog_telemetry_type_t get_telemetry_entry_type(const char *interface_name);

/**
 * @brief Get the telemetry entry array index for specific telemetry type.
 *
 * @param[in] type The type of telemetry.
 *
 * @return The index of array, -1 otherwise.
 */
static int get_telemetry_entry_index(edgehog_telemetry_type_t type);

/**
 * @brief Get the telemetry entry period from the user configuration.
 *
 * @param[in] type The entry type to search.
 * @param[in] telemetry A valid telemetry instance.
 *
 * @return The periods in seconds if the telemetry exists, -1 otherwise.
 */
static int64_t get_telemetry_entry_period_from_config(
    edgehog_telemetry_type_t type, edgehog_telemetry_t *telemetry);

/**
 * @brief Set a telemetry entry in the telemetry entries array.
 *
 * @details If the entry to be set is already existing, the previous will be freed.
 *
 * @param[in] new_entry A valid telemetry entry to set.
 * @param[inout] entries The array in which to set the entry.
 */
static void set_telemetry_entry(telemetry_entry_t *new_entry, telemetry_entry_t **entries);

/**
 * @brief Store a telemetry entry in the settings.
 *
 * @param[in] entry The telemtry entry to store.
 *
 * @return EDGEHOG_RESULT_OK if successful, an edgehog_result_t otherwise.
 */
static edgehog_result_t store_telemetry_entry(telemetry_entry_t *entry);

/**
 * @brief Remove an telemetry entry from the entries array.
 *
 * @param[in] type The type for the entry to remove.
 * @param[inout] entries The array of entries.
 */
static void remove_telemetry_entry(edgehog_telemetry_type_t type, telemetry_entry_t **entries);

/**
 * @brief Check the existence of a telemetry type in the user provided configuration.
 *
 * @param[in] type The telemetry entry type to search.
 * @param[in] telemetry A valid telemetry instance, from which the configuration will be extracted.
 *
 * @return True if the telemetry type exists, false otherwise.
 */
static bool telemetry_type_is_present_in_config(
    edgehog_telemetry_type_t type, edgehog_telemetry_t *telemetry);

/**
 * @brief Parse a telemetry configuration event.
 *
 * @param[in] event The Astarte device event data.
 * @param[out] telemetry_type The telemetry type of the event.
 * @param[out] endpoint The enpoint of Astarte device data event.
 *
 * @return EDGEHOG_RESULT_OK if successful, an edgehog_result_t otherwise.
 */
static edgehog_result_t parse_config_event(
    astarte_device_data_event_t *event, edgehog_telemetry_type_t *type, char **endpoint);

/**
 * @brief Schedule a new telemetry entry.
 *
 * @param[in] telemetry A valid telemetry instance.
 * @param[in] entry The telemetry entry to schedule.
 *
 * @return EDGEHOG_RESULT_OK if successful, an edgehog_result_t otherwise.
 */
static edgehog_result_t telemetry_schedule_entry(
    edgehog_telemetry_t *telemetry, telemetry_entry_t *entry);

/**
 * @brief Remove a telemetry entry from scheduling.
 *
 * @param[in] entry The telemetry entry to remove from the scheduling.
 *
 * @return EDGEHOG_RESULT_OK if successful, an edgehog_result_t otherwise.
 */
static edgehog_result_t telemetry_unschedule_entry(telemetry_entry_t *entry);

/************************************************
 *       Callbacks declaration/definition       *
 ***********************************************/

/**
 * @brief Handle telemetry entries loading from settings.
 *
 * @param[in] key The name with skipped part that was used as name in handler registration.
 * @param[in] len The size of the data found in the backend.
 * @param[in] read_cb Function provided to read the data from the backend.
 * @param[inout] cb_arg Arguments for the read function provided by the backend.
 * @param[inout] param Parameter given to the settings_load_subtree_direct function.
 *
 * @return When nonzero value is returned, further subtree searching is stopped.
 */
static int settings_entry_loader(
    const char *key, size_t len, settings_read_cb read_cb, void *cb_arg, void *param)
{
    ARG_UNUSED(len);

    const char *next = NULL;
    telemetry_entry_t **entries = (telemetry_entry_t **) param;

    size_t next_len = settings_name_next(key, &next);

    if (next_len == 0 || !next) {
        return -ENOENT;
    }

    const int base = 10;
    edgehog_telemetry_type_t type = (edgehog_telemetry_type_t) strtol(&key[0], NULL, base);

    if (type <= EDGEHOG_TELEMETRY_INVALID || type >= EDGEHOG_TELEMETRY_LEN) {
        return -ENOENT;
    }

    telemetry_entry_t *entry = get_telemetry_entry(type, entries);
    if (!entry) {
        entry = telemetry_entry_new(type, TELEMETRY_UPDATE_DEFAULT, false);

        if (!entry) {
            return -ENOENT;
        }

        set_telemetry_entry(entry, entries);
    }

    next_len = settings_name_next(key, &next);

    if (!next) {
        return 0;
    }

    if (strncmp(next, SETTINGS_TELEMETRY_PERIODS_KEY, next_len) == 0) {
        int res = read_cb(cb_arg, &entry->period_seconds, sizeof(entry->period_seconds));
        if (res < 0) {
            EDGEHOG_LOG_ERR("Unable to read telemetry entry period from settings: %d", res);
            return res;
        }
        return 0;
    }

    if (strncmp(next, SETTINGS_TELEMETRY_ENABLE_KEY, next_len) == 0) {
        int res = read_cb(cb_arg, &entry->enable, sizeof(entry->enable));
        if (res < 0) {
            EDGEHOG_LOG_ERR("Unable to read telemetry entry enable from settings: %d", res);
            return res;
        }
        return 0;
    }

    return -ENOENT;
}

/**
 * @brief Function invoked by each telemetry entry timer upon expiration.
 *
 * @details It will place a message in the message queue with the entry type.
 *
 * @param[in] timer Pointer to the timer triggering the call.
 */
static void entry_timer_expiry_fn(struct k_timer *timer)
{
    telemetry_entry_t *entry = CONTAINER_OF(timer, telemetry_entry_t, timer);
    struct k_msgq *msgq = (struct k_msgq *) k_timer_user_data_get(timer);

    k_msgq_put(msgq, &entry->type, K_NO_WAIT);
}

/**
 * @brief Entry point for the message message queue thread.
 *
 * @details This thread will wait for a new message from the queue. Each time it receives one it
 * will trigger the pubblish of a telemetry entry of the received type.
 *
 *
 * @param[in] device_ptr Pointer to a valid instance of an edgehog device.
 * @param[in] queue_ptr Pointer to the message queue instance.
 * @param[in] unused Unused parameter.
 */
static void msgq_thread_entry_point(void *device_ptr, void *queue_ptr, void *unused)
{
    ARG_UNUSED(unused);
    edgehog_device_handle_t device = (edgehog_device_handle_t) device_ptr;
    struct k_msgq *msgq = (struct k_msgq *) queue_ptr;
    edgehog_telemetry_type_t type = EDGEHOG_TELEMETRY_INVALID;

    while (atomic_test_bit(&device->telemetry->msgq_thread_state, MSGQ_THREAD_RUNNING_BIT)) {
        if (k_msgq_get(msgq, &type, K_MSEC(MSGQ_GET_TIMEOUT)) == 0) {
            edgehog_device_publish_telemetry(device, type);
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
    load_entries_from_settings(telemetry->entries);
    load_entries_from_config(configs, configs_len, telemetry->entries);

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

    if (atomic_test_and_set_bit(&telemetry->msgq_thread_state, MSGQ_THREAD_RUNNING_BIT)) {
        EDGEHOG_LOG_ERR("Failed starting telemetry service as it's already running");
        return EDGEHOG_RESULT_TELEMETRY_START_FAIL;
    }

    k_msgq_init(&telemetry->msgq, telemetry->msgq_buffer, sizeof(edgehog_telemetry_type_t),
        EDGEHOG_TELEMETRY_LEN);

    k_tid_t thread_id = k_thread_create(&telemetry->msgq_thread, message_queue_stack_area,
        MSGQ_THREAD_STACK_SIZE, msgq_thread_entry_point, (void *) device, (void *) &telemetry->msgq,
        NULL, MSGQ_THREAD_PRIORITY, 0, K_NO_WAIT);

    if (!thread_id) {
        EDGEHOG_LOG_ERR("Unable to start telemetry message thread");
        atomic_clear_bit(&telemetry->msgq_thread_state, MSGQ_THREAD_RUNNING_BIT);
        return EDGEHOG_RESULT_TELEMETRY_START_FAIL;
    }

    for (int i = 0; i < EDGEHOG_TELEMETRY_LEN; i++) {
        telemetry_entry_t *entry = telemetry->entries[i];
        if (entry && entry->enable) {
            telemetry_schedule_entry(telemetry, entry);
        }
    }

    return EDGEHOG_RESULT_OK;
}

edgehog_result_t edgehog_telemetry_config_set_event(
    edgehog_telemetry_t *telemetry, astarte_device_property_set_event_t *event)
{
    char *endpoint = NULL;
    edgehog_telemetry_type_t type = EDGEHOG_TELEMETRY_INVALID;
    edgehog_result_t res = parse_config_event(&event->data_event, &type, &endpoint);

    if (res != EDGEHOG_RESULT_OK) {
        return res;
    }

    if (!endpoint) {
        EDGEHOG_LOG_ERR("Unable to handle config telemetry update, endpoint empty");
        return EDGEHOG_RESULT_ASTARTE_ERROR;
    }

    telemetry_entry_t *entry = get_telemetry_entry(type, telemetry->entries);

    if (!entry) {
        entry = telemetry_entry_new(type, TELEMETRY_UPDATE_DEFAULT, false);

        if (!entry) {
            return EDGEHOG_RESULT_TELEMETRY_START_FAIL;
        }

        set_telemetry_entry(entry, telemetry->entries);
    }

    astarte_individual_t value = event->individual;
    if (strcmp(endpoint, "enable") == 0) {
        if (value.tag == ASTARTE_MAPPING_TYPE_BOOLEAN) {
            entry->enable = value.data.boolean;
        }
    } else if (strcmp(endpoint, "periodSeconds") == 0) {
        if (value.tag == ASTARTE_MAPPING_TYPE_LONGINTEGER) {
            entry->period_seconds = value.data.longinteger;
        }
    }

    if (entry->enable) {
        return telemetry_schedule_entry(telemetry, entry);
    }
    return telemetry_unschedule_entry(entry);
}

edgehog_result_t edgehog_telemetry_config_unset_event(
    edgehog_telemetry_t *telemetry, astarte_device_data_event_t *event)
{
    char *endpoint = NULL;
    edgehog_telemetry_type_t type = EDGEHOG_TELEMETRY_INVALID;
    edgehog_result_t res = parse_config_event(event, &type, &endpoint);

    if (res != EDGEHOG_RESULT_OK) {
        return res;
    }

    if (!endpoint) {
        EDGEHOG_LOG_ERR("Unable to handle config telemetry update, endpoint empty");
        return EDGEHOG_RESULT_ASTARTE_ERROR;
    }

    telemetry_entry_t *entry = get_telemetry_entry(type, telemetry->entries);

    if (!entry) {
        return EDGEHOG_RESULT_TELEMETRY_START_FAIL;
    }

    if (strcmp("enable", endpoint) == 0) {
        entry->enable = telemetry_type_is_present_in_config(entry->type, telemetry);
    } else if (strcmp("periodSeconds", endpoint) == 0) {
        entry->period_seconds = get_telemetry_entry_period_from_config(entry->type, telemetry);
    }

    if (entry->enable) {
        return telemetry_schedule_entry(telemetry, entry);
    }
    return telemetry_unschedule_entry(entry);
}

edgehog_result_t edgehog_telemetry_stop(edgehog_telemetry_t *telemetry, k_timeout_t timeout)
{
    // Request the thread to self exit
    atomic_clear_bit(&telemetry->msgq_thread_state, MSGQ_THREAD_RUNNING_BIT);
    // Wait for the thread to self exit
    int res = k_thread_join(&telemetry->msgq_thread, timeout);
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

/************************************************
 *         Static functions definitions         *
 ***********************************************/

static edgehog_result_t load_entries_from_settings(telemetry_entry_t **entries)
{
    return edgehog_settings_load(SETTINGS_TELEMETRY_KEY, settings_entry_loader, (void *) entries);
}

static void load_entries_from_config(
    edgehog_telemetry_config_t *configs, size_t configs_len, telemetry_entry_t **entries)
{
    for (int i = 0; i < configs_len; i++) {
        edgehog_telemetry_config_t config_entry = configs[i];
        if (!telemetry_entry_exist(config_entry.type, entries)) {
            telemetry_entry_t *entry
                = telemetry_entry_new(config_entry.type, config_entry.period_seconds, true);
            if (entry) {
                set_telemetry_entry(entry, entries);
            }
        }
    }
}

static telemetry_entry_t *telemetry_entry_new(
    edgehog_telemetry_type_t type, int64_t period_seconds, bool enable)
{
    telemetry_entry_t *entry = (telemetry_entry_t *) calloc(1, sizeof(telemetry_entry_t));

    if (!entry) {
        EDGEHOG_LOG_ERR("Out of memory %s: %d", __FILE__, __LINE__);
        return NULL;
    }

    entry->type = type;
    entry->period_seconds = period_seconds;
    entry->enable = enable;
    k_timer_init(&entry->timer, entry_timer_expiry_fn, NULL);

    return entry;
}

static edgehog_telemetry_type_t get_telemetry_entry_type(const char *interface_name)
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

static int get_telemetry_entry_index(edgehog_telemetry_type_t type)
{
    if (type > EDGEHOG_TELEMETRY_INVALID && type < EDGEHOG_TELEMETRY_LEN) {
        return (int) type - 1;
    }
    return -1;
}

static bool telemetry_entry_exist(edgehog_telemetry_type_t type, telemetry_entry_t **entries)
{
    int entry_idx = get_telemetry_entry_index(type);

    if (entry_idx < 0) {
        EDGEHOG_LOG_ERR("Invalid telemetry index %d", type);
        return false;
    }

    telemetry_entry_t *entry = entries[entry_idx];
    if (entry) {
        return true;
    }

    return false;
}

static telemetry_entry_t *get_telemetry_entry(
    edgehog_telemetry_type_t type, telemetry_entry_t **entries)
{
    int entry_idx = get_telemetry_entry_index(type);

    if (entry_idx < 0) {
        EDGEHOG_LOG_ERR("Invalid telemetry index %d", type);
        return NULL;
    }

    return entries[entry_idx];
}

static int64_t get_telemetry_entry_period_from_config(
    edgehog_telemetry_type_t type, edgehog_telemetry_t *telemetry)
{
    for (int i = 0; i < telemetry->configs_len; i++) {
        if (type == telemetry->configs[i].type) {
            return telemetry->configs[i].period_seconds;
        }
    }
    return -1;
}

static void set_telemetry_entry(telemetry_entry_t *new_entry, telemetry_entry_t **entries)
{
    int entry_idx = get_telemetry_entry_index(new_entry->type);

    if (entry_idx < 0) {
        EDGEHOG_LOG_ERR("Invalid telemetry index %d", new_entry->type);
        return;
    }

    telemetry_entry_t *current_entry = entries[entry_idx];
    free(current_entry);
    entries[entry_idx] = new_entry;
}

static void remove_telemetry_entry(edgehog_telemetry_type_t type, telemetry_entry_t **entries)
{
    int index = get_telemetry_entry_index(type);

    if (index < 0) {
        EDGEHOG_LOG_ERR("Invalid telemetry entry type, %d", type);
        return;
    }

    telemetry_entry_t *entry = entries[index];
    free(entry);
    entries[index] = NULL;
}

static edgehog_result_t store_telemetry_entry(telemetry_entry_t *entry)
{
    size_t entry_type_key_len = SETTINGS_TELEMETRY_KEY_LEN + 2;
    char entry_type_key[entry_type_key_len + 1];

    int snprintf_rc = snprintf(
        entry_type_key, entry_type_key_len + 1, SETTINGS_TELEMETRY_KEY "/%d", entry->type);

    if (snprintf_rc != entry_type_key_len) {
        EDGEHOG_LOG_ERR("Failure in formatting the Telemetry type key.");
        return EDGEHOG_RESULT_TELEMETRY_STORE_FAIL;
    }

    edgehog_result_t res = edgehog_settings_save(entry_type_key, SETTINGS_TELEMETRY_PERIODS_KEY,
        &entry->period_seconds, sizeof(entry->period_seconds));

    if (res != EDGEHOG_RESULT_OK) {
        return res;
    }

    return edgehog_settings_save(
        entry_type_key, SETTINGS_TELEMETRY_ENABLE_KEY, &entry->enable, sizeof(entry->enable));
}

static bool telemetry_type_is_present_in_config(
    edgehog_telemetry_type_t type, edgehog_telemetry_t *telemetry)
{
    for (int i = 0; i < telemetry->configs_len; i++) {
        if (type == telemetry->configs[i].type) {
            return true;
        }
    }
    return false;
}

static edgehog_result_t parse_config_event(
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

    edgehog_telemetry_type_t type_tmp = get_telemetry_entry_type(interface_name);
    if (type_tmp == EDGEHOG_TELEMETRY_INVALID) {
        EDGEHOG_LOG_ERR("Unable to handle config telemetry update, telemetry type %s not supported",
            interface_name);
        return EDGEHOG_RESULT_ASTARTE_ERROR;
    }

    *type = type_tmp;

    return EDGEHOG_RESULT_OK;
}

static edgehog_result_t telemetry_schedule_entry(
    edgehog_telemetry_t *telemetry, telemetry_entry_t *entry)
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

    store_telemetry_entry(entry);

    if (!atomic_test_bit(&telemetry->msgq_thread_state, MSGQ_THREAD_RUNNING_BIT)) {
        return EDGEHOG_RESULT_OK;
    }

    if (k_timer_remaining_get(&entry->timer) == 0) {

        k_timer_user_data_set(&entry->timer, &telemetry->msgq);
        k_timer_start(
            &entry->timer, K_SECONDS(entry->period_seconds), K_SECONDS(entry->period_seconds));

        if (k_timer_remaining_get(&entry->timer) == 0) {
            EDGEHOG_LOG_WRN("The timer %d could not be set into the Active state", entry->type);
            k_timer_stop(&entry->timer);
            remove_telemetry_entry(entry->type, telemetry->entries);
            goto error;
        };

    } else {
        k_timer_stop(&entry->timer);
        k_timer_start(
            &entry->timer, K_SECONDS(entry->period_seconds), K_SECONDS(entry->period_seconds));
    }

    return EDGEHOG_RESULT_OK;

error:
    EDGEHOG_LOG_ERR("Unable to schedule new telemetry");
    return EDGEHOG_RESULT_TELEMETRY_START_FAIL;
}

static edgehog_result_t telemetry_unschedule_entry(telemetry_entry_t *entry)
{
    if (!entry) {
        EDGEHOG_LOG_ERR("Telemetry undefined");
        return EDGEHOG_RESULT_TELEMETRY_STOP_FAIL;
    }

    if (entry->type <= EDGEHOG_TELEMETRY_INVALID || entry->type >= EDGEHOG_TELEMETRY_LEN) {
        EDGEHOG_LOG_ERR("Telemetry type invalid %d", entry->type);
        return EDGEHOG_RESULT_TELEMETRY_STOP_FAIL;
    }

    store_telemetry_entry(entry);

    if (k_timer_remaining_get(&entry->timer) != 0) {
        k_timer_stop(&entry->timer);
    }

    return EDGEHOG_RESULT_OK;
}
