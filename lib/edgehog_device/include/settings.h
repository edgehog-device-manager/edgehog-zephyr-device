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
 * @details The settings are organized in a tree format, and the key used to store/fetch them should
 * correspond to the path to the corresponding tree location. Any instance of the settings driver
 * will have a common first branch named `EDGEHOG_ID`. Each instance of this driver will be free to
 * define its branch(es) using the `subtree` and `key` parameters. The `EDGEHOG_ID`, `subtree`, and
 * `key` paths will be combined to obtain the full path to each setting.
 */

#include "edgehog_device/device.h"
#include "edgehog_device/result.h"

#include <zephyr/settings/settings.h>

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
 * @param[in] subtree subtree name of the subtree to be loaded.
 * @param[in] load_cb pointer to the callback function.
 * @param[inout] param parameter to be passed when callback function is called.
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
 *  @param[in] subtree subtree name of the subtree to be stored.
 *  @param[in] key key of the settings item.
 *  @param[in] value Pointer to the value of the settings item.
 *  @param[in] value_len Pointer to the value of the settings item.
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
 *  @param[in] subtree subtree name of the subtree to be stored.
 *  @param[in] key key of the settings item.
 *
 * @return EDGEHOG_RESULT_OK if successful, otherwise an error code.
 */
edgehog_result_t edgehog_settings_delete(const char *subtree, const char *key);

#ifdef __cplusplus
}
#endif

#endif // SETTINGS_H
