/*
 * (C) Copyright 2024, SECO Mind Srl
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef EDGEHOG_DEVICE_NVS_H
#define EDGEHOG_DEVICE_NVS_H

/**
 * @file nvs.h
 * @brief Edgehog NVS private APIs
 */

#include "edgehog_device/device.h"
#include "edgehog_device/result.h"

#include <zephyr/fs/nvs.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief open the Edgehog non-volatile storage.
 *
 * @details This function open an Edgehog non-volatile storage.
 *
 * @param[in/out] out_fs If successful (return code is zero), handle will be
 *                        returned in this argument.
 * @return EDGEHOG_RESULT_OK if storage handle was opened successfully, an edgehog_result_t otherwise.
 */
edgehog_result_t edgehog_nvs_open(struct nvs_fs *out_fs);

#ifdef __cplusplus
}
#endif

#endif // EDGEHOG_DEVICE_NVS_H
