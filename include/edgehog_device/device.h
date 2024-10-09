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

/**
 * @defgroup device Device management
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
#define EDGEHOG_DEVICE_MINOR 6
/** @brief Patch version number */
#define EDGEHOG_DEVICE_PATCH 0

/**
 * @brief Handle for an instance of an Edgehog device.
 *
 * @details Each handle is a pointer to an opaque internally allocated data struct containing
 * all the data for the Edgehog device.
 */
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
    /** @brief The telemetries configured by the user. */
    edgehog_device_telemetry_config_t *telemetry_config;
    /** @brief The len of telemetries. */
    size_t telemetry_config_len;
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
 * @param[out] edgehog_handle Device instance initialized.
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
 * @param edgehog_device A valid Edgehog device handle.
 * @return EDGEHOG_RESULT_OK if the device was successfully started, another edgehog_err_t
 * otherwise.
 */
edgehog_result_t edgehog_device_start(edgehog_device_handle_t edgehog_device);

/**
 * @brief Stop the Edgehog device.
 *
 * @note When this function timeouts it is not ensured that the telemetry service  won't be running
 * for some additional time after the function returned.
 *
 * @param edgehog_device A valid Edgehog device handle.
 * @param timeout A timeout to use for stopping the telemetry services.
 * @return EDGEHOG_RESULT_OK if successful, EDGEHOG_RESULT_TELEMETRY_STOP_TIMEOUT if the timeout
 * has expired before the telemetry service stopped, otherwise an error code.
 */
edgehog_result_t edgehog_device_stop(edgehog_device_handle_t edgehog_device, k_timeout_t timeout);

/**
 * @brief Handler for astarte datastream object event.
 *
 * @details This function must be called when an Astarte datastream object event is received.
 *
 * @param edgehog_device A valid Edgehog device handle.
 * @param event Astarte device datastream object event pointer.
 */
void edgehog_device_datastream_object_events_handler(
    edgehog_device_handle_t edgehog_device, astarte_device_datastream_object_event_t event);

/**
 * @brief Handler for astarte datastream individual event.
 *
 * @details This function must be called when an Astarte datastream individual event is received.
 *
 * @param edgehog_device A valid Edgehog device handle.
 * @param event Astarte device datastream individual event pointer.
 */
void edgehog_device_datastream_individual_events_handler(
    edgehog_device_handle_t edgehog_device, astarte_device_datastream_individual_event_t event);

/**
 * @brief Handler for astarte set property event.
 *
 * @details This function must be called when an Astarte property set event is received.
 *
 * @param edgehog_device A valid Edgehog device handle.
 * @param event Astarte device data event pointer.
 */
void edgehog_device_property_set_events_handler(
    edgehog_device_handle_t edgehog_device, astarte_device_property_set_event_t event);

/**
 * @brief Handler for astarte unset property event.
 *
 * @details This function must be called when an Astarte property unset event is received.
 *
 * @param edgehog_device A valid Edgehog device handle.
 * @param event  Astarte device data event pointer.
 */
void edgehog_device_property_unset_events_handler(
    edgehog_device_handle_t edgehog_device, astarte_device_data_event_t event);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif // EDGEHOG_DEVICE_DEVICE_H
