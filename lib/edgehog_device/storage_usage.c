/*
 * (C) Copyright 2024, SECO Mind Srl
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "storage_usage.h"

#include <zephyr/devicetree.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/fs/fs.h>
#include <zephyr/settings/settings.h>
#include <zephyr/storage/flash_map.h>
#include <zephyr/version.h>

#if KERNEL_VERSION_NUMBER >= ZEPHYR_VERSION(4, 4, 0)
#ifdef CONFIG_SETTINGS_NVS
#include <zephyr/kvss/nvs.h>
#elif defined(CONFIG_SETTINGS_ZMS)
#include <zephyr/kvss/zms.h>
#endif
#else
#ifdef CONFIG_SETTINGS_NVS
#include <zephyr/fs/nvs.h>
#elif defined(CONFIG_SETTINGS_ZMS)
#include <zephyr/fs/zms.h>
#endif
#endif

#include <astarte_device_sdk/device.h>
#include <astarte_device_sdk/result.h>

#include "edgehog_private.h"
#include "generated_interfaces.h"
#include "system_time.h"

#include "log.h"
EDGEHOG_LOG_MODULE_REGISTER(storage_usage, CONFIG_EDGEHOG_DEVICE_STORAGE_USAGE_LOG_LEVEL);

/************************************************
 *        Defines, constants and typedef        *
 ***********************************************/

/* Only demand a settings partition from DT if we are using a flash-based backend */
#if defined(CONFIG_SETTINGS_NVS) || defined(CONFIG_SETTINGS_ZMS)
#if DT_HAS_CHOSEN(zephyr_settings_partition)
/* Get the node identifier for the chosen settings partition */
#define SETTINGS_PARTITION_NODE DT_CHOSEN(zephyr_settings_partition)

#if DT_NODE_HAS_PROP(SETTINGS_PARTITION_NODE, label)
/* Gets the string label ("edgehog_storage") */
#define SETTINGS_PARTITION_LABEL DT_PROP(SETTINGS_PARTITION_NODE, label)
#else
/* Fallback in case the user did not set an explicit label. Use the partition name. */
#define SETTINGS_PARTITION_LABEL DT_NODE_FULL_NAME(SETTINGS_PARTITION_NODE)
#endif
#else
/* Fallback: If not explicitly chosen, Zephyr defaults to the 'storage_partition' node.
   We provide a safe generic string here to prevent build failures. */
#define SETTINGS_PARTITION_LABEL "storage"
#endif
#endif

/************************************************
 *         Static functions declarations        *
 ***********************************************/

static void publish_user_storage_usage(edgehog_device_handle_t edgehog_device);
static void publish_edgehog_storage_usage(edgehog_device_handle_t edgehog_device);
#ifdef CONFIG_SETTINGS_NVS
edgehog_result_t get_nvs_space(size_t *total_space, size_t *free_space);
#endif // defined(CONFIG_SETTINGS_NVS)
#ifdef CONFIG_SETTINGS_ZMS
edgehog_result_t get_zms_space(size_t *total_space, size_t *free_space);
#endif // defined(CONFIG_SETTINGS_ZMS)
static void send_storage_telemetry(
    edgehog_device_handle_t device, const char *path, size_t total_space, size_t free_space);

/************************************************
 *         Global functions definition          *
 ***********************************************/

void publish_storage_usage(edgehog_device_handle_t edgehog_device)
{
    if (edgehog_device == NULL) {
        EDGEHOG_LOG_ERR("Publish storage usage called with NULL device handle");
        return;
    }

    EDGEHOG_LOG_DBG("Publishing storage usage");
    if ((edgehog_device->storage_partitions_len > 0) && edgehog_device->storage_partitions) {
        publish_user_storage_usage(edgehog_device);
    }
    publish_edgehog_storage_usage(edgehog_device);
}

/************************************************
 *         Static functions definition          *
 ***********************************************/

static void publish_user_storage_usage(edgehog_device_handle_t edgehog_device)
{
    for (size_t i = 0; i < edgehog_device->storage_partitions_len; i++) {
        const char *path = edgehog_device->storage_partitions[i].path;
        size_t total_space = 0;
        size_t free_space = 0;

        switch (edgehog_device->storage_partitions[i].type) {
            case EDGEHOG_STORAGE_PARTITION_TYPE_FS: {
#ifdef CONFIG_FILE_SYSTEM
                struct fs_statvfs stats = { 0 };
                int res = fs_statvfs(path, &stats);
                if (res != 0) {
                    EDGEHOG_LOG_ERR("Unable to calculate FS free space: %d", res);
                    continue;
                }
                total_space = (size_t) stats.f_frsize * stats.f_blocks;
                free_space = (size_t) stats.f_frsize * stats.f_bfree;
                break;
#else
                EDGEHOG_LOG_ERR(
                    "File system storage partition type is not supported in this build");
                continue;
#endif // defined(CONFIG_FILE_SYSTEM)
            }
            case EDGEHOG_STORAGE_PARTITION_TYPE_NVS: {
#ifdef CONFIG_NVS
                ssize_t free_space_res = nvs_calc_free_space(
                    edgehog_device->storage_partitions[i].nvs_partition.nvs_fs);
                if (free_space_res < 0) {
                    EDGEHOG_LOG_ERR("Unable to calculate NVS free space: %zd", free_space_res);
                    continue;
                }
                total_space = edgehog_device->storage_partitions[i].nvs_partition.total_space;
                free_space = (size_t) free_space_res;
                break;
#else
                EDGEHOG_LOG_ERR("NVS storage partition type is not supported in this build");
                continue;
#endif // defined(CONFIG_NVS)
            }
            case EDGEHOG_STORAGE_PARTITION_TYPE_ZMS: {
#ifdef CONFIG_ZMS
                ssize_t free_space_res = zms_calc_free_space(
                    edgehog_device->storage_partitions[i].zms_partition.zms_fs);
                if (free_space_res < 0) {
                    EDGEHOG_LOG_ERR("Unable to calculate ZMS free space: %zd", free_space_res);
                    continue;
                }
                total_space = edgehog_device->storage_partitions[i].zms_partition.total_space;
                free_space = (size_t) free_space_res;
                break;
#else
                EDGEHOG_LOG_ERR("ZMS storage partition type is not supported in this build");
                continue;
#endif // defined(CONFIG_ZMS)
            }
            default:
                EDGEHOG_LOG_ERR("Unknown storage partition type for %s", path);
                continue;
        }

        send_storage_telemetry(edgehog_device, path, total_space, free_space);
    }
}

static void publish_edgehog_storage_usage(edgehog_device_handle_t edgehog_device)
{
#ifdef CONFIG_SETTINGS_NVS
    const char *path = "/" SETTINGS_PARTITION_LABEL;
    size_t total_space = 0;
    size_t free_space = 0;
    if (get_nvs_space(&total_space, &free_space) != EDGEHOG_RESULT_OK) {
        return;
    }
#elif defined(CONFIG_SETTINGS_ZMS)
    const char *path = "/" SETTINGS_PARTITION_LABEL;
    size_t total_space = 0;
    size_t free_space = 0;
    if (get_zms_space(&total_space, &free_space) != EDGEHOG_RESULT_OK) {
        return;
    }
#elif defined(CONFIG_SETTINGS_FILE)
    struct fs_file_t file;
    fs_file_t_init(&file);
    if (fs_open(&file, CONFIG_SETTINGS_FILE_PATH, 0) != 0) {
        return;
    }
    const char *path = file.mp->mnt_point;
    fs_close(&file);

    struct fs_statvfs stats;
    if (fs_statvfs(path, &stats) != 0) {
        return;
    }
    size_t total_space = (size_t) stats.f_frsize * stats.f_blocks;
    size_t free_space = (size_t) stats.f_frsize * stats.f_bfree;
#else
    return; // Unsupported storage backend, skip telemetry
#endif

    send_storage_telemetry(edgehog_device, path, total_space, free_space);
}

#ifdef CONFIG_SETTINGS_NVS

edgehog_result_t get_nvs_space(size_t *total_space, size_t *free_space)
{
    void *storage_ptr = NULL;

    int ret = settings_storage_get(&storage_ptr);
    if (ret != 0 || storage_ptr == NULL) {
        EDGEHOG_LOG_ERR("Failed to get settings storage context: %d", ret);
        return EDGEHOG_RESULT_NVS_ERROR;
    }

    struct nvs_fs *nvs_fs = (struct nvs_fs *) storage_ptr;

    // Dynamically calculate total space from the actual NVS instance
    *total_space = (size_t) (nvs_fs->sector_size * nvs_fs->sector_count);

    ssize_t free_space_res = nvs_calc_free_space(nvs_fs);
    if (free_space_res < 0) {
        EDGEHOG_LOG_ERR("Unable to calculate NVS free space: %zd", free_space_res);
        return EDGEHOG_RESULT_NVS_ERROR;
    }

    *free_space = (size_t) free_space_res;
    return EDGEHOG_RESULT_OK;
}
#endif // defined (CONFIG_SETTINGS_NVS)

#ifdef CONFIG_SETTINGS_ZMS
edgehog_result_t get_zms_space(size_t *total_space, size_t *free_space)
{
    void *storage_ptr = NULL;

    int ret = settings_storage_get(&storage_ptr);
    if (ret != 0 || storage_ptr == NULL) {
        EDGEHOG_LOG_ERR("Failed to get settings storage context: %d", ret);
        return EDGEHOG_RESULT_ZMS_ERROR;
    }

    struct zms_fs *zms_fs = (struct zms_fs *) storage_ptr;

    // Dynamically calculate total space from the actual ZMS instance
    *total_space = (size_t) (zms_fs->sector_size * zms_fs->sector_count);

    ssize_t free_space_res = zms_calc_free_space(zms_fs);
    if (free_space_res < 0) {
        EDGEHOG_LOG_ERR("Unable to calculate ZMS free space: %zd", free_space_res);
        return EDGEHOG_RESULT_ZMS_ERROR;
    }

    *free_space = (size_t) free_space_res;
    return EDGEHOG_RESULT_OK;
}
#endif // defined (CONFIG_SETTINGS_ZMS)

static void send_storage_telemetry(
    edgehog_device_handle_t device, const char *path, size_t total_space, size_t free_space)
{
    astarte_object_entry_t object_entries[]
        = { { .path = "totalBytes", .data = astarte_data_from_longinteger(total_space) },
              { .path = "freeBytes", .data = astarte_data_from_longinteger(free_space) } };

    int64_t timestamp_ms = 0;
    system_time_current_ms(&timestamp_ms);

    astarte_result_t res = astarte_device_send_object(device->astarte_device,
        io_edgehog_devicemanager_StorageUsage.name, path, object_entries,
        ARRAY_SIZE(object_entries), &timestamp_ms);

    if (res != ASTARTE_RESULT_OK) {
        EDGEHOG_LOG_ERR("Unable to send storage usage for %s", path);
    }
}
