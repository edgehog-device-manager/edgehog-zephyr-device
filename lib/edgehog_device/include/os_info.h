/*
 * (C) Copyright 2024, SECO Mind Srl
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef OS_INFO_H
#define OS_INFO_H

/**
 * @file os_info.h
 * @brief Operating System info API
 */

#include "edgehog_device/device.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Publish a OS informations.
 *
 * @param[in] edgehog_device A valid Edgehog device handle.
 */
void publish_os_info(edgehog_device_handle_t edgehog_device);

#ifdef __cplusplus
}
#endif

#endif // OS_INFO_H
