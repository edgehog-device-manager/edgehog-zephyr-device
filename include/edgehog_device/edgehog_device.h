/*
 * (C) Copyright 2024, SECO Mind Srl
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file edgehog.h
 * @brief ...
 */

#ifndef EDGEHOG_DEVICE_EDGEHOG_DEVICE_H
#define EDGEHOG_DEVICE_EDGEHOG_DEVICE_H

typedef struct edgehog_device_t *edgehog_device_handle_t;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief create Edgehog device handle.
 *
 * @details This function creates an Edgehog device handle. It must be called before anything else.
 *
 * Example:
 *  edgehog_device_handle_t edgehog_device = edgehog_device_new();
 *
 * @return The handle to the device, NULL if an error occurred.
 */
edgehog_device_handle_t edgehog_device_new();

/**
 * @brief destroy Edgehog device.
 *
 * @details This function destroys the device, freeing all its resources.
 * @param edgehog_device A valid Edgehog device handle.
 */
void edgehog_device_destroy(edgehog_device_handle_t edgehog_device);

#ifdef __cplusplus
}
#endif

#endif // EDGEHOG_DEVICE_EDGEHOG_DEVICE_H
