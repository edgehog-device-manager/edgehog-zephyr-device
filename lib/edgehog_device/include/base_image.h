/*
 * (C) Copyright 2024, SECO Mind Srl
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef BASE_IMAGE_H
#define BASE_IMAGE_H

/**
 * @file base_image.h
 * @brief Base Edgehog image information API
 */

#include "edgehog_device/device.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Publish the base image information.
 *
 * @param[in] edgehog_device A valid Edgehog device handle.
 */
void publish_base_image(edgehog_device_handle_t edgehog_device);

#ifdef __cplusplus
}
#endif

#endif // BASE_IMAGE_H
