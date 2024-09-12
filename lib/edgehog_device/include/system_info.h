/*
 * (C) Copyright 2024, SECO Mind Srl
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SYSTEM_INFO_H
#define SYSTEM_INFO_H

/**
 * @file system_info.h
 * @brief Operating System info API
 */

#include "edgehog_device/device.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Publish a system informations.
 *
 * @param[in] edgehog_device A valid Edgehog device handle.
 */
void publish_system_info(edgehog_device_handle_t edgehog_device);

#ifdef __cplusplus
}
#endif

#endif // SYSTEM_INFO_H
