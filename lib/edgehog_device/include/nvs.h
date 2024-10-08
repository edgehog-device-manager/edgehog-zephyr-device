/*
 * (C) Copyright 2024, SECO Mind Srl
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef NVS_H
#define NVS_H

/**
 * @file nvs.h
 * @brief Edgehog NVS private APIs
 */

#include "edgehog_device/device.h"
#include "edgehog_device/result.h"

#include <zephyr/fs/nvs.h>

#ifdef CONFIG_EDGEHOG_DEVICE_USE_EDGEHOG_PARTITION
/** @brief The devicetree partition name for the NVS. */
#define NVS_PARTITION edgehog_partition
#else
/** @brief The devicetree partition name for the NVS. */
#define NVS_PARTITION storage_partition
#endif

#if DT_NODE_HAS_PROP(DT_NODELABEL(NVS_PARTITION), label)
/** @brief The devicetree partition label for the NVS. */
#define NVS_PARTITION_LABEL DT_PROP(DT_NODELABEL(NVS_PARTITION), label)
#else
/** @brief The devicetree partition label for the NVS. */
#define NVS_PARTITION_LABEL "storage"
#endif

/** @brief The devicetree partition size for the NVS. */
#define NVS_PARTITION_SIZE DT_REG_SIZE(DT_NODELABEL(NVS_PARTITION))

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief open the Edgehog non-volatile storage.
 *
 * @details This function open an Edgehog non-volatile storage.
 *
 * @param[inout] out_fs If successful (return code is zero), handle will be
 *                        returned in this argument.
 * @return EDGEHOG_RESULT_OK if storage handle was opened successfully, an edgehog_result_t
 * otherwise.
 */
edgehog_result_t edgehog_nvs_open(struct nvs_fs *out_fs);

/**
 * @brief get the Edgehog non-volatile storage free space.
 *
 * @details This function calculate the free space available to Edgehog non-volatile storage.
 *
 * @param[out] free_space If successful (return code is zero), the number of bytes free will be
 *                         returned in this argument.
 * @return EDGEHOG_RESULT_OK if storage handle was opened successfully, an edgehog_result_t
 * otherwise.
 */
edgehog_result_t edgehog_nvs_get_free_space(size_t *free_space);

#ifdef __cplusplus
}
#endif

#endif // NVS_H
