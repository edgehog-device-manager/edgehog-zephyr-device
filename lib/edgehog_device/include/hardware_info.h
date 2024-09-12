/*
 * (C) Copyright 2024, SECO Mind Srl
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef HARDWARE_INFO_H
#define HARDWARE_INFO_H

/**
 * @file hardware_info.h
 * @brief Hardware info API
 */

#include "edgehog_device/device.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Publish a hardware informations.
 *
 * @param[in] edgehog_device A valid Edgehog device handle.
 */
void publish_hardware_info(edgehog_device_handle_t edgehog_device);

/**
 * @brief get memory size.
 *
 * @param memory_size A valid size_t handle.
 *
 * @return true if the memory is found successfully in the device-tree, false otherwise.
 */
bool hardware_info_get_memory_size(size_t *memory_size);

#ifdef __cplusplus
}
#endif

#endif // HARDWARE_INFO_H
