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

typedef struct edgehog_telemetry_data edgehog_telemetry_t;

#ifdef __cplusplus
extern "C" {
#endif
/**
 * @brief create Edgehog telemetry.
 *
 * @details This function creates an Edgehog telemetry.
 *
 * @param telemetry_config An edgehog_device_telemetry_config_t struct.
 * @param telemetry_config_len Number of telemetry config elements.
 * @return A pointer to Edgehog telemetry or a NULL if an error occurred.
 */
edgehog_telemetry_t *edgehog_telemetry_new(
    edgehog_device_telemetry_config_t *telemetry_config, size_t telemetry_config_len);

/**
 * @brief Start Edgehog telemetry.
 *
 * @details This function starts an Edgehog telemetry.
 *
 * @param edgehog_device A valid Edgehog device handle.
 * @param edgehog_telemetry A valid Edgehog telemetry pointer.
 */
edgehog_result_t edgehog_telemetry_start(
    edgehog_device_handle_t edgehog_device, edgehog_telemetry_t *edgehog_telemetry);

/**
 * @brief Destroy Edgehog telemetry.
 *
 * @details This function destroy an Edgehog telemetry.
 *
 * @param edgehog_telemetry A valid Edgehog telemetry handle.
 */
edgehog_result_t edgehog_telemetry_destroy(edgehog_telemetry_t *edgehog_telemetry);

/**
 * @brief Handle received Edgehog telemetry config set event.
 *
 * @details This function handles a config telemetry set event request from Astarte.
 *
 * @param edgehog_dev A valid Edgehog device handle.
 * @param event A valid Astarte property set event.
 *
 * @return EDGEHOG_RESULT_OK if the LED event is handled successfully, an edgehog_result_t
 * otherwise.
 */
edgehog_result_t edgehog_telemetry_config_set_event(
    edgehog_device_handle_t edgehog_dev, astarte_device_property_set_event_t *event);

/**
 * @brief Handle received Edgehog telemetry config unset event.
 *
 * @details This function handles a config telemetry unset event request from Astarte.
 *
 * @param edgehog_dev A valid Edgehog device handle.
 * @param event A valid Astarte datastream individual event.
 *
 * @return EDGEHOG_RESULT_OK if the LED event is handled successfully, an edgehog_result_t
 * otherwise.
 */
edgehog_result_t edgehog_telemetry_config_unset_event(
    edgehog_device_handle_t edgehog_dev, astarte_device_data_event_t *event);

#ifdef __cplusplus
}
#endif

#endif // TELEMETRY_PRIVATE_H
