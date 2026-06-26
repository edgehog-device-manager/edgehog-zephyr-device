/*
 * (C) Copyright 2026, SECO Mind Srl
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef EDGEHOG_DEVICE_STORAGE_H
#define EDGEHOG_DEVICE_STORAGE_H

/**
 * @file storage.h
 */

/**
 * @defgroup storage Storage management
 * @brief API for storage management.
 * @ingroup edgehog_device
 * @{
 */

#include <zephyr/version.h>

#if KERNEL_VERSION_NUMBER >= ZEPHYR_VERSION(4, 4, 0)
#ifdef CONFIG_NVS
#include <zephyr/kvss/nvs.h>
#elif defined(CONFIG_ZMS)
#include <zephyr/kvss/zms.h>
#endif
#else
#ifdef CONFIG_NVS
#include <zephyr/fs/nvs.h>
#elif defined(CONFIG_ZMS)
#include <zephyr/fs/zms.h>
#endif
#endif

/**
 * @brief Storage partition type.
 * @details Represents the type of a storage partition to be monitored by telemetry.
 */
typedef enum
{
    /** @brief File system partition. */
    EDGEHOG_STORAGE_PARTITION_TYPE_FS = 0,
    /** @brief Non Volatile Storage partition. */
    EDGEHOG_STORAGE_PARTITION_TYPE_NVS,
    /** @brief Zephyr Managed Storage partition. */
    EDGEHOG_STORAGE_PARTITION_TYPE_ZMS,
} edgehog_storage_partition_type_t;

#ifdef CONFIG_NVS
typedef struct
{
    /** @brief The mount point or path of the partition. */
    struct nvs_fs *nvs_fs;
    /** @brief The total space available in the partition. */
    size_t total_space;
} edgehog_storage_partition_nvs_t;
#endif // defined(CONFIG_NVS)

#ifdef CONFIG_ZMS
typedef struct
{
    /** @brief The mount point or path of the partition. */
    struct zms_fs *zms_fs;
    /** @brief The total space available in the partition. */
    size_t total_space;
} edgehog_storage_partition_zms_t;
#endif // defined(CONFIG_ZMS)

/**
 * @brief Edgehog storage partition struct.
 * @details Represents a storage partition to be monitored by telemetry.
 */
typedef struct
{
    /** @brief The type of the partition. */
    edgehog_storage_partition_type_t type;
    /** @brief The mount point or path of the partition. */
    const char *path;
#ifdef CONFIG_NVS
    edgehog_storage_partition_nvs_t nvs_partition;
#endif // defined(CONFIG_NVS)
#ifdef CONFIG_ZMS
    edgehog_storage_partition_zms_t zms_partition;
#endif // defined(CONFIG_ZMS)
} edgehog_storage_partition_t;

/**
 * @}
 */

#endif // EDGEHOG_DEVICE_STORAGE_H
