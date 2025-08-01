/*
 * (C) Copyright 2024-2025, SECO Mind Srl
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef TELEMETRY_ENTRY_H
#define TELEMETRY_ENTRY_H

/**
 * @file telemetry_entry.h
 * @brief Telemetry entry fields.
 */

#include "edgehog_device/device.h"
#include "edgehog_device/telemetry.h"

#include <zephyr/kernel.h>

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
} edgehog_telemetry_entry_t;

#define TELEMETRY_UPDATE_DEFAULT 0

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Create a new instance of the a telemetry entry.
 *
 * @param[in] type The type of the telemetry entry to create.
 * @param[in] period_seconds The interval in seconds for the telemetry entry.
 * @param[in] enable The enable flag.
 *
 * @return The telemetry entry if the creation was successfully, NULL otherwise.
 */
edgehog_telemetry_entry_t *telemetry_entry_new(
    edgehog_telemetry_type_t type, int64_t period_seconds, bool enable);

/**
 * @brief Load telemetry entries from settings.
 *
 * @param[out] entries Array of telemetry entries extracted from settings.
 *
 * @return EDGEHOG_RESULT_OK if successful, an edgehog_result_t otherwise.
 */
edgehog_result_t telemetry_entries_load_from_settings(edgehog_telemetry_entry_t **entries);

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
void telemetry_entries_load_from_config(
    edgehog_telemetry_config_t *configs, size_t configs_len, edgehog_telemetry_entry_t **entries);

/**
 * @brief Store a telemetry entry in the settings.
 *
 * @param[in] entry The telemtry entry to store.
 *
 * @return EDGEHOG_RESULT_OK if successful, an edgehog_result_t otherwise.
 */
edgehog_result_t telemetry_entry_store(edgehog_telemetry_entry_t *entry);

/**
 * @brief Remove an telemetry entry from the entries array.
 *
 * @param[in] type The type for the entry to remove.
 * @param[inout] entries The array of entries.
 */
void telemetry_entry_remove(edgehog_telemetry_type_t type, edgehog_telemetry_entry_t **entries);

/**
 * @brief Get a telemetry entry from the entries array.
 *
 * @param[in] type The type of telemetry entry to get.
 * @param[in] entries The array of telemetry entries.
 *
 * @return The entry if it exists, NULL otherwise.
 */
edgehog_telemetry_entry_t *telemetry_entry_get(
    edgehog_telemetry_type_t type, edgehog_telemetry_entry_t **entries);

/**
 * @brief Set a telemetry entry in the telemetry entries array.
 *
 * @details If the entry to be set is already existing, the previous will be freed.
 *
 * @param[in] new_entry A valid telemetry entry to set.
 * @param[inout] entries The array in which to set the entry.
 */
void telemetry_entry_set(edgehog_telemetry_entry_t *new_entry, edgehog_telemetry_entry_t **entries);

#ifdef __cplusplus
}
#endif

#endif // TELEMETRY_ENTRY_H
