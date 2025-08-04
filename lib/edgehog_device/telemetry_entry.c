/*
 * (C) Copyright 2024-2025, SECO Mind Srl
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "telemetry_entry.h"

#include <stdlib.h>

#include "settings.h"

#include "log.h"
EDGEHOG_LOG_MODULE_DECLARE(telemetry, CONFIG_EDGEHOG_DEVICE_TELEMETRY_LOG_LEVEL);

/************************************************
 *        Defines, constants and typedef        *
 ***********************************************/

#define SETTINGS_TELEMETRY_KEY "telemetry"
#define SETTINGS_TELEMETRY_KEY_LEN strlen(SETTINGS_TELEMETRY_KEY)
#define SETTINGS_TELEMETRY_PERIODS_KEY "periods"
#define SETTINGS_TELEMETRY_ENABLE_KEY "enable"
#define SETTINGS_TELEMETRY_TYPE_KEY_SIZE 1

/************************************************
 *         Static functions declaration         *
 ***********************************************/

/**
 * @brief Get the telemetry entry array index for specific telemetry type.
 *
 * @param[in] type The type of telemetry.
 *
 * @return The index of array, -1 otherwise.
 */
static int get_telemetry_entry_index(edgehog_telemetry_type_t type);

/**
 * @brief Check the existence of telemetry type in the entries array.
 *
 * @param[in] type The type of telemtry to check.
 * @param[in] entries The array of entries.
 *
 * @return True if the telemetry exists, false otherwise.
 */
static bool telemetry_entry_exist(
    edgehog_telemetry_type_t type, edgehog_telemetry_entry_t **entries);

/************************************************
 *       Callbacks declaration/definition       *
 ***********************************************/

/**
 * @brief Function invoked by each telemetry entry timer upon expiration.
 *
 * @details It will place a message in the message queue with the entry type.
 *
 * @param[in] timer Pointer to the timer triggering the call.
 */
static void entry_timer_expiry_fn(struct k_timer *timer)
{
    edgehog_telemetry_entry_t *entry = CONTAINER_OF(timer, edgehog_telemetry_entry_t, timer);
    struct k_msgq *msgq = (struct k_msgq *) k_timer_user_data_get(timer);

    k_msgq_put(msgq, &entry->type, K_NO_WAIT);
}

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
    edgehog_telemetry_entry_t **entries = (edgehog_telemetry_entry_t **) param;

    size_t next_len = settings_name_next(key, &next);

    if (next_len == 0 || !next) {
        return -ENOENT;
    }

    const int base = 10;
    edgehog_telemetry_type_t type = (edgehog_telemetry_type_t) strtol(&key[0], NULL, base);

    if (type <= EDGEHOG_TELEMETRY_INVALID || type >= EDGEHOG_TELEMETRY_LEN) {
        return -ENOENT;
    }

    edgehog_telemetry_entry_t *entry = telemetry_entry_get(type, entries);
    if (!entry) {
        entry = telemetry_entry_new(type, TELEMETRY_UPDATE_DEFAULT, false);

        if (!entry) {
            return -ENOENT;
        }

        telemetry_entry_set(entry, entries);
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

/************************************************
 *         Global functions definitions         *
 ***********************************************/

edgehog_telemetry_entry_t *telemetry_entry_new(
    edgehog_telemetry_type_t type, int64_t period_seconds, bool enable)
{
    edgehog_telemetry_entry_t *entry = calloc(1, sizeof(edgehog_telemetry_entry_t));

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

edgehog_result_t telemetry_entries_load_from_settings(edgehog_telemetry_entry_t **entries)
{
    return edgehog_settings_load(SETTINGS_TELEMETRY_KEY, settings_entry_loader, (void *) entries);
}

void telemetry_entries_load_from_config(
    edgehog_telemetry_config_t *configs, size_t configs_len, edgehog_telemetry_entry_t **entries)
{
    for (int i = 0; i < configs_len; i++) {
        edgehog_telemetry_config_t config_entry = configs[i];
        if (!telemetry_entry_exist(config_entry.type, entries)) {
            edgehog_telemetry_entry_t *entry
                = telemetry_entry_new(config_entry.type, config_entry.period_seconds, true);
            if (entry) {
                telemetry_entry_set(entry, entries);
            }
        }
    }
}

edgehog_result_t telemetry_entry_store(edgehog_telemetry_entry_t *entry)
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

void telemetry_entry_remove(edgehog_telemetry_type_t type, edgehog_telemetry_entry_t **entries)
{
    int index = get_telemetry_entry_index(type);

    if (index < 0) {
        EDGEHOG_LOG_ERR("Invalid telemetry entry type, %d", type);
        return;
    }

    edgehog_telemetry_entry_t *entry = entries[index];
    free(entry);
    entries[index] = NULL;
}

edgehog_telemetry_entry_t *telemetry_entry_get(
    edgehog_telemetry_type_t type, edgehog_telemetry_entry_t **entries)
{
    int entry_idx = get_telemetry_entry_index(type);

    if (entry_idx < 0) {
        EDGEHOG_LOG_ERR("Invalid telemetry index %d", type);
        return NULL;
    }

    return entries[entry_idx];
}

void telemetry_entry_set(edgehog_telemetry_entry_t *new_entry, edgehog_telemetry_entry_t **entries)
{
    int entry_idx = get_telemetry_entry_index(new_entry->type);

    if (entry_idx < 0) {
        EDGEHOG_LOG_ERR("Invalid telemetry index %d", new_entry->type);
        return;
    }

    edgehog_telemetry_entry_t *current_entry = entries[entry_idx];
    free(current_entry);
    entries[entry_idx] = new_entry;
}

/************************************************
 *         Static functions definitions         *
 ***********************************************/

static int get_telemetry_entry_index(edgehog_telemetry_type_t type)
{
    if (type > EDGEHOG_TELEMETRY_INVALID && type < EDGEHOG_TELEMETRY_LEN) {
        return (int) type - 1;
    }
    return -1;
}

static bool telemetry_entry_exist(
    edgehog_telemetry_type_t type, edgehog_telemetry_entry_t **entries)
{
    int entry_idx = get_telemetry_entry_index(type);

    if (entry_idx < 0) {
        EDGEHOG_LOG_ERR("Invalid telemetry index %d", type);
        return false;
    }

    edgehog_telemetry_entry_t *entry = entries[entry_idx];
    if (entry) {
        return true;
    }

    return false;
}
