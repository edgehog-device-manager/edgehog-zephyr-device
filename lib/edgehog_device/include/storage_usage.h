/*
 * (C) Copyright 2024, SECO Mind Srl
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef STORAGE_USAGE_H
#define STORAGE_USAGE_H

/**
 * @file storage_usage.h
 * @brief Storage usageS API
 */

#include "edgehog_device/device.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Publish a storage usage informations.
 *
 * @param[in] edgehog_device A valid Edgehog device handle.
 */
void publish_storage_usage(edgehog_device_handle_t edgehog_device);

#ifdef __cplusplus
}
#endif

#endif // STORAGE_USAGE_H
