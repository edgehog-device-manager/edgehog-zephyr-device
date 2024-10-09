/*
 * (C) Copyright 2024, SECO Mind Srl
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef TELEMETRY_PRIVATE_H
#define TELEMETRY_PRIVATE_H

/**
 * @file telemetry_private.h
 * @brief Private Telemetry fields.
 */

#include "edgehog_device/device.h"
#include "edgehog_device/telemetry.h"

/**
 * @brief Internal struct for an instance of an Edgehog telemetry.
 */
typedef struct edgehog_telemetry_data edgehog_telemetry_t;

#ifdef __cplusplus
extern "C" {
#endif
/**
 * @brief Create an Edgehog telemetry service.
 *
 * @param configs An array of user configuration entries.
 * @param configs_len Number of configuration elements.
 * @return A pointer to Edgehog telemetry or a NULL if an error occurred.
 */
edgehog_telemetry_t *edgehog_telemetry_new(edgehog_telemetry_config_t *configs, size_t configs_len);

/**
 * @brief Start an Edgehog telemetry service.
 *
 * @param device A valid Edgehog device handle.
 *
 * @return EDGEHOG_RESULT_OK if the telemetry routine is started successfully, an edgehog_result_t
 * otherwise.
 */
edgehog_result_t edgehog_telemetry_start(edgehog_device_handle_t device);

/**
 * @brief Stop an Edgehog telemetry service.
 *
 * @param telemetry A valid Edgehog telemetry pointer.
 * @param timeout A timeout for the stop operation.
 *
 * @return EDGEHOG_RESULT_OK if the telemetry service is stopped successfully within the timeout,
 * EDGEHOG_RESULT_TELEMETRY_STOP_TIMEOUT otherwise.
 */
edgehog_result_t edgehog_telemetry_stop(edgehog_telemetry_t *telemetry, k_timeout_t timeout);

/**
 * @brief Destroy an Edgehog telemetry service.
 *
 * @param telemetry A valid Edgehog telemetry handle.
 */
void edgehog_telemetry_destroy(edgehog_telemetry_t *telemetry);

/**
 * @brief Handle received Edgehog telemetry config set event.
 *
 * @details This function handles a config telemetry set event request from Astarte.
 *
 * @param telemetry A valid Edgehog telemetry handle.
 * @param event A valid Astarte property set event data.
 *
 * @return EDGEHOG_RESULT_OK if successful, an edgehog_result_t otherwise.
 */
edgehog_result_t edgehog_telemetry_config_set_event(
    edgehog_telemetry_t *telemetry, astarte_device_property_set_event_t *event);

/**
 * @brief Handle received Edgehog telemetry config unset event.
 *
 * @details This function handles a config telemetry unset event request from Astarte.
 *
 * @param telemetry A valid Edgehog telemetry handle.
 * @param event A valid Astarte device event data.
 *
 * @return EDGEHOG_RESULT_OK if successful, an edgehog_result_t otherwise.
 */
edgehog_result_t edgehog_telemetry_config_unset_event(
    edgehog_telemetry_t *telemetry, astarte_device_data_event_t *event);

#ifdef __cplusplus
}
#endif

#endif // TELEMETRY_PRIVATE_H
