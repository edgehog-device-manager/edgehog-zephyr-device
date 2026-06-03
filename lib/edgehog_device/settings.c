/*
 * (C) Copyright 2024, SECO Mind Srl
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "settings.h"

#include <stdio.h>

#include "log.h"
EDGEHOG_LOG_MODULE_REGISTER(edgehog_settings, CONFIG_EDGEHOG_DEVICE_SETTINGS_LOG_LEVEL);

/************************************************
 *        Defines, constants and typedef        *
 ***********************************************/

#define EGDEHOG_ID "edgehog_device"

/************************************************
 *         Global functions definition          *
 ***********************************************/

edgehog_result_t edgehog_settings_init()
{
    EDGEHOG_LOG_DBG("Initializing Edgehog settings subsystem...");
    int res = settings_subsys_init();
    if (res != 0) {
        EDGEHOG_LOG_ERR("Unable to init edgehog settings, error code: %d", res);
        return EDGEHOG_RESULT_SETTINGS_INIT_FAIL;
    }

    EDGEHOG_LOG_DBG("Edgehog settings subsystem initialized successfully.");
    return EDGEHOG_RESULT_OK;
}

edgehog_result_t edgehog_settings_load(
    const char *subtree, settings_load_direct_cb load_cb, void *param)
{
    size_t edgehog_subtree_len = strlen(EGDEHOG_ID) + 1 + strlen(subtree);
    char edgehog_subtree[edgehog_subtree_len + 1];

    int snprintf_rc = snprintf(edgehog_subtree, edgehog_subtree_len + 1, "%s%c%s", EGDEHOG_ID,
        SETTINGS_NAME_SEPARATOR, subtree);
    if (snprintf_rc != edgehog_subtree_len) {
        EDGEHOG_LOG_ERR("Failure in formatting the Edgehog subtree settings for [%s]", subtree);
        return EDGEHOG_RESULT_SETTINGS_LOAD_FAIL;
    }

    EDGEHOG_LOG_DBG("Loading settings from subtree: [%s]", edgehog_subtree);
    int res = settings_load_subtree_direct(edgehog_subtree, load_cb, param);
    if (res != 0) {
        EDGEHOG_LOG_ERR("Unable to load items from the Edgehog setting [%s], error code: %d",
            edgehog_subtree, res);
        return EDGEHOG_RESULT_SETTINGS_LOAD_FAIL;
    }

    EDGEHOG_LOG_DBG("Successfully loaded settings from subtree: [%s]", edgehog_subtree);
    return EDGEHOG_RESULT_OK;
}

edgehog_result_t edgehog_settings_save(
    const char *subtree, const char *key, const void *value, size_t value_len)
{
    size_t edgehog_subtree_path_len = strlen(EGDEHOG_ID) + 1 + strlen(subtree) + 1 + strlen(key);
    char edgehog_subtree_path[edgehog_subtree_path_len + 1];

    int snprintf_rc = snprintf(edgehog_subtree_path, edgehog_subtree_path_len + 1, "%s%c%s%c%s",
        EGDEHOG_ID, SETTINGS_NAME_SEPARATOR, subtree, SETTINGS_NAME_SEPARATOR, key);

    if (snprintf_rc != edgehog_subtree_path_len) {
        EDGEHOG_LOG_ERR(
            "Failure in formatting the Edgehog subtree settings path for [%s/%s]", subtree, key);
        return EDGEHOG_RESULT_SETTINGS_SAVE_FAIL;
    }

    EDGEHOG_LOG_DBG(
        "Saving setting to path: [%s] (size: %zu bytes)", edgehog_subtree_path, value_len);
    int res = settings_save_one(edgehog_subtree_path, value, value_len);
    if (res != 0) {
        EDGEHOG_LOG_ERR("Unable to save item { %s } to the Edgehog setting, error code: %d",
            edgehog_subtree_path, res);
        return EDGEHOG_RESULT_SETTINGS_SAVE_FAIL;
    }

    EDGEHOG_LOG_DBG("Successfully saved setting to path: [%s]", edgehog_subtree_path);
    return EDGEHOG_RESULT_OK;
}

edgehog_result_t edgehog_settings_delete(const char *subtree, const char *key)
{
    size_t edgehog_subtree_path_len = strlen(EGDEHOG_ID) + 1 + strlen(subtree) + 1 + strlen(key);
    char edgehog_subtree_path[edgehog_subtree_path_len + 1];

    int snprintf_rc = snprintf(edgehog_subtree_path, edgehog_subtree_path_len + 1, "%s%c%s%c%s",
        EGDEHOG_ID, SETTINGS_NAME_SEPARATOR, subtree, SETTINGS_NAME_SEPARATOR, key);
    if (snprintf_rc != edgehog_subtree_path_len) {
        EDGEHOG_LOG_ERR(
            "Failure in formatting the Edgehog subtree settings path for deletion [%s/%s]", subtree,
            key);
        return EDGEHOG_RESULT_SETTINGS_SAVE_FAIL;
    }

    EDGEHOG_LOG_DBG("Deleting setting at path: [%s]", edgehog_subtree_path);
    int res = settings_delete(edgehog_subtree_path);

    if (res != 0) {
        EDGEHOG_LOG_ERR("Unable to delete item at { %s } from the Edgehog setting, error code: %d",
            edgehog_subtree_path, res);
        return EDGEHOG_RESULT_SETTINGS_SAVE_FAIL;
    }

    EDGEHOG_LOG_DBG("Successfully deleted setting at path: [%s]", edgehog_subtree_path);
    return EDGEHOG_RESULT_OK;
}
