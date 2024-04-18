/*
 * (C) Copyright 2024, SECO Mind Srl
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef EDGEHOG_DEVICE_DEVICE_H
#define EDGEHOG_DEVICE_DEVICE_H

/**
 * @file device.h
 * @brief Device management
 */

#include "edgehog_device/result.h"

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
    /**
     * @brief Instance of the astarte device.
     *
     * @details This handle **won't** be freed by the Edgehog device,
     * his ownership belongs to the caller.
     */
    astarte_device_handle_t astarte_device;
} edgehog_device_config_t;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief create Edgehog device handle.
 *
 * @details This function creates an Edgehog device handle. It must be called before anything else.
 * If an error code is returned the astarte_device_free function must not be called.
 *
 * Example:
 *  astarte_device_handle_t astarte_device = NULL;
    astarte_err = astarte_device_new(&device_config, &astarte_device);
    if (astarte_err != ASTARTE_OK) {
        return -1;
    }
 *
 *  edgehog_device_config_t edgehog_conf = {
 *      .astarte_device = astarte_device,
 *  };
 *
 *  edgehog_device_handle_t edgehog_device = NULL;
 *  edgehog_result_t edgehog_result = edgehog_device_new(&edgehog_conf, &edgehog_device);
 *
 * @param[in] config An edgehog_device_config_t struct.
 * @param[out] handle Device instance initialized.
 * @return EDGEHOG_RESULT_OK if successful, otherwise an error code.
 */

edgehog_result_t edgehog_device_new(
    edgehog_device_config_t *config, edgehog_device_handle_t *edgehog_handle);

/**
 * @brief destroy Edgehog device.
 *
 * @details This function destroys the device, freeing all its resources.
 * @param edgehog_device A valid Edgehog device handle.
 */
void edgehog_device_destroy(edgehog_device_handle_t edgehog_device);

/**
 * @brief start Edgehog device.
 *
 * @details This function starts the device, enabling the telemetry update if configured.
 * @param device A valid Edgehog device handle.
 * @return EDGEHOG_RESULT_OK if the device was successfully started, another edgehog_err_t
 * otherwise.
 */
edgehog_result_t edgehog_device_start(edgehog_device_handle_t edgehog_device);

#ifdef __cplusplus
}
#endif

#endif // EDGEHOG_DEVICE_DEVICE_H
