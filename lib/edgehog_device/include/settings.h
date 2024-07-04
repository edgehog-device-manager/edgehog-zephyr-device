/*
 * (C) Copyright 2024, SECO Mind Srl
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SETTINGS_H
#define SETTINGS_H

/**
 * @file settings.h
 * @brief Edgehog device settings API.
 *
 * @details The Edgehog settings subsystem gives a way to store persistent
 * configuration and runtime state. Settings items are stored as key-value pair strings.
 * The keys are organized by package ID `edgehog_device` and subtree, for instance the key
 * `edgehog_device/ota` would define the ota state for edgehog device.
 */

#include "edgehog_device/device.h"
#include "edgehog_device/result.h"

#include <zephyr/settings/settings.h>

#define EGDEHOG_ID "edgehog_device"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialization of Edgehog settings.
 *
 * @return EDGEHOG_RESULT_OK if successful, otherwise an error code.
 */
edgehog_result_t edgehog_settings_init();

/**
 * @brief Load set of serialized items using given callback.
 *
 * @details This function loads the settings for a specific subtree, all the values found are
 * passed to the given callback.
 *
 * @param subtree[in] subtree name of the subtree to be loaded.
 * @param load_cb[in] pointer to the callback function..
 * @param param[inout] parameter to be passed when callback function is called..
 *
 * @return EDGEHOG_RESULT_OK if successful, otherwise an error code.
 */
edgehog_result_t edgehog_settings_load(
    const char *subtree, settings_load_direct_cb load_cb, void *param);

/**
 * @brief Store a single value to Edegehog settings
 *
 * @details This function write a single serialized value to persisted storage.
 *
 *  @param subtree[in] subtree name of the subtree to be stored.
 *  @param key[in] key of the settings item.
 *  @param value[in] Pointer to the value of the settings item.
 *  @param value_len[in] Pointer to the value of the settings item.
 *
 * @return EDGEHOG_RESULT_OK if successful, otherwise an error code.
 */
edgehog_result_t edgehog_settings_save(
    const char *subtree, const char *key, const void *value, size_t value_len);

/**
 * @brief Delete a single serialized in Edegehog persisted storage.
 *
 * @details This function delete a single serialized value from the persisted storage.
 *
 *  @param[in] subtree[in] subtree name of the subtree to be stored.
 *  @param[in] key[in] key of the settings item.
 *
 * @return EDGEHOG_RESULT_OK if successful, otherwise an error code.
 */
edgehog_result_t edgehog_settings_delete(const char *subtree, const char *key);

#ifdef __cplusplus
}
#endif

#endif // SETTINGS_H
