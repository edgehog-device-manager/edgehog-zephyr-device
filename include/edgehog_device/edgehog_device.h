/*
 * (C) Copyright 2024, SECO Mind Srl
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef EDGEHOG_DEVICE_EDGEHOG_DEVICE_H
#define EDGEHOG_DEVICE_EDGEHOG_DEVICE_H

/**
 * @file edgehog_device.h
 * @brief Edgehog device SDK API.
 */

#include <astarte_device_sdk/device.h>

typedef struct edgehog_device_t *edgehog_device_handle_t;

/**
 * @brief Edgehog device configuration struct
 *
 * @details This struct is used to collect all the data needed by the edgehog_device_new function.
 * Pay attention that astarte_device is required and must not be null.
 * The values provided with this struct are not copied, do not free() them before calling
 * edgehog_device_destroy.
 */
typedef struct
{
    astarte_device_handle_t astarte_device;
} edgehog_device_config_t;


#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief create Edgehog device handle.
 *
 * @details This function creates an Edgehog device handle. It must be called before anything else.
 *
 * Example:
 *  astarte_device_handle_t astarte_device = NULL;
    astarte_err = astarte_device_new(&device_config, &device);
    if (astarte_err != ASTARTE_OK) {
        return -1;
    }
 *
 *  edgehog_device_config_t edgehog_conf = {
 *      .astarte_device = astarte_device,
 *  };
 *
 *  edgehog_device_handle_t edgehog_device = edgehog_device_new(&edgehog_conf);
 *
 * @param config An edgehog_device_config_t struct.
 * @return The handle to the device, NULL if an error occurred.
 */

edgehog_device_handle_t edgehog_device_new(edgehog_device_config_t *config);

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
