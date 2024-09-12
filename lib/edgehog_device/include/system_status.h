/*
 * (C) Copyright 2024, SECO Mind Srl
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SYSTEM_STATUS_H
#define SYSTEM_STATUS_H

/**
 * @file system_status.h
 * @brief System status
 */

#include "edgehog_device/device.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Publish a system status.
 *
 * @param[in] edgehog_device A valid Edgehog device handle.
 */
void publish_system_status(edgehog_device_handle_t edgehog_device);

#ifdef __cplusplus
}
#endif

#endif // SYSTEM_STATUS_H
