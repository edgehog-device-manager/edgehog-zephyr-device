/*
 * (C) Copyright 2024, SECO Mind Srl
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef EDGEHOG_DEVICE_DEVICE_H
#define EDGEHOG_DEVICE_DEVICE_H

/**
 * @file device.h
 */

/**
 * @defgroup device Device management
 * @brief API for device management.
 * @ingroup edgehog_device
 * @{
 */

#include <zephyr/sys_clock.h>

#include "edgehog_device/result.h"
#include "edgehog_device/telemetry.h"

#include <astarte_device_sdk/device.h>

/** @brief Major version number */
#define EDGEHOG_DEVICE_MAJOR 0
/** @brief Minor version number */
#define EDGEHOG_DEVICE_MINOR 7
/** @brief Patch version number */
#define EDGEHOG_DEVICE_PATCH 0

/**
 * @brief Handle for an instance of an Edgehog device.
 *
 * @details Each handle is a pointer to an opaque internally allocated data struct containing
 * all the data for the Edgehog device.
 */
typedef struct edgehog_device *edgehog_device_handle_t;

/**
 * @brief Edgehog device configuration struct.
 *
 * @details This struct is used to collect all the data needed by the #edgehog_device_new function.
 * The values provided within this struct are not copied, do not free() them before calling
 * #edgehog_device_destroy.
 */
typedef struct
{
    /**
     * @brief Configuration struct for the Astarte device.
     * @details This struct will be used to initialize the Astarte device that the Edgehog device
     * will use for communication. The Edgehog device will maintain ownership of the Astarte
     * device and will take care of connecting/disconnecting it, terminating its execution and
     * freeing its resources.
     */
    astarte_device_config_t astarte_device_config;
    /**
     * @brief The telemetries configured by the user.
     * @details See #edgehog_telemetry_config_t for more information.
     */
    edgehog_telemetry_config_t *telemetry_config;
    /** @brief The len of telemetries. */
    size_t telemetry_config_len;
} edgehog_device_config_t;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Create an Edgehog device instance.
 *
 * @details This function creates an Edgehog device instance. It must be called before any other
 * function.
 *
 * Usage example:
 *  ```C
 *  edgehog_device_config_t edgehog_conf = {
 *      .astarte_device_config = astarte_conf,
 *  };
 *
 *  edgehog_device_handle_t edgehog_device = NULL;
 *  edgehog_result_t edgehog_result = edgehog_device_new(&edgehog_conf, &edgehog_device);
 *  ```
 *
 * @param[in] config The configuration for the Edgehog instance to create.
 * @param[out] edgehog_handle Handle to the created device instance. If the function returns an
 * error this parameter is left unchanges, a call to #edgehog_device_destroy is not required.
 * @return #EDGEHOG_RESULT_OK if successful, otherwise an error code.
 */
edgehog_result_t edgehog_device_new(
    edgehog_device_config_t *config, edgehog_device_handle_t *edgehog_handle);

/**
 * @brief Destroy an Edgehog device.
 *
 * @details This function destroys the device, freeing all its resources.
 * @note The internal Astarte device will become invalid after a call to this function.
 *
 * @param[in] edgehog_device A valid Edgehog device handle.
 */
void edgehog_device_destroy(edgehog_device_handle_t edgehog_device);

/**
 * @brief Start an Edgehog device.
 *
 * @details This function starts the device, creating a connection with the cloud instance
 * through Astarte.
 *
 * @param[inout] edgehog_device A valid Edgehog device handle.
 * @return #EDGEHOG_RESULT_OK if successful, otherwise an error code.
 */
edgehog_result_t edgehog_device_start(edgehog_device_handle_t edgehog_device);

/**
 * @brief Poll the Edgehog device.
 *
 * @details This function should be periodically called to poll the Edgehog device.
 *
 * @param[inout] edgehog_device A valid Edgehog device handle.
 * @return #EDGEHOG_RESULT_OK on success, an error code otherwise.
 */
edgehog_result_t edgehog_device_poll(edgehog_device_handle_t edgehog_device);

/**
 * @brief Stop the Edgehog device.
 *
 * @note When this function timeouts it is not ensured that the telemetry service won't be running
 * for some additional time after the function returned.
 *
 * @param[inout] edgehog_device A valid Edgehog device handle.
 * @param[in] timeout A timeout to use for stopping the telemetry services and disconnecting the
 * Astarte device.
 * @return #EDGEHOG_RESULT_OK if successful, #EDGEHOG_RESULT_TELEMETRY_STOP_TIMEOUT if the timeout
 * has expired before the telemetry service stopped, otherwise an error code.
 */
edgehog_result_t edgehog_device_stop(edgehog_device_handle_t edgehog_device, k_timeout_t timeout);

/**
 * @brief Get a reference to the Astarte device that Edgehog uses for communication.
 *
 * @note The Astarte device returned will remain owned by the Edgehog device and should only
 * be used to interact with user defined interfaces. Stopping the Edgehog device will automatically
 * disconnect the Astarte device. While destroying it will free its resources.
 *
 * @param[inout] edgehog_device A valid Edgehog device handle.
 * @return The Astarte device for the Edgehog instance.
 */
astarte_device_handle_t edgehog_device_get_astarte_device(edgehog_device_handle_t edgehog_device);

/**
 * @brief Get the last returned Astarte error.
 *
 * @note Should be used when an Edgehog function returns #EDGEHOG_RESULT_ASTARTE_ERROR to fetch the
 * last returned error from the internal Astarte device.
 *
 * @param[inout] edgehog_device A valid Edgehog device handle.
 * @return The last Astarte error returned by the internal Astarte device.
 */
astarte_result_t edgehog_device_get_astarte_error(edgehog_device_handle_t edgehog_device);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif // EDGEHOG_DEVICE_DEVICE_H
