/*
 * (C) Copyright 2024, SECO Mind Srl
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef EDGEHOG_PRIVATE_H
#define EDGEHOG_PRIVATE_H

/**
 * @file edgehog_private.h
 * @brief Private Edgehog Device APIs and fields
 */

#include "led.h"
#include "ota.h"
#include "telemetry_private.h"
#include "uuid.h"

#include <astarte_device_sdk/device.h>

/** @brief Possible states for the Edgehog device. */
enum device_states
{
    /** @brief The device is not operational. */
    EDGEHOG_DEVICE_STOPPED = 0U,
    /** @brief The device is has been started, but does not yet have connectivity. */
    EDGEHOG_DEVICE_STARTED,
    /** @brief The device is has been started, and has been connected to Astarte. */
    EDGEHOG_DEVICE_CONNECTED,
};

/**
 * @brief Internal struct for an instance of an Edgehog device.
 *
 * @warning Users should not modify the content of this struct directly.
 */
struct edgehog_device_t
{
    /** @brief Edgehog device state. */
    enum device_states state;
    /** @brief This flag marks if the initial publish has been performed. */
    bool initial_publish;
    /** @brief Handle of an Astarte device. */
    astarte_device_handle_t astarte_device;
    /** @brief The last returned error from Astarte. */
    astarte_result_t astarte_error;
    /** @brief Original connection callback provided by the user, might be NULL. */
    astarte_device_connection_cbk_t user_connection_cbk;
    /** @brief Original disconnection callback provided by the user, might be NULL. */
    astarte_device_disconnection_cbk_t user_disconnection_cbk;
    /** @brief Original datastream individual callback provided by the user, might be NULL. */
    astarte_device_datastream_individual_cbk_t user_datastream_individual_cbk;
    /** @brief Original datastream object callback provided by the user, might be NULL. */
    astarte_device_datastream_object_cbk_t user_datastream_object_cbk;
    /** @brief Original property set callback provided by the user, might be NULL. */
    astarte_device_property_set_cbk_t user_property_set_cbk;
    /** @brief Original property unset callback provided by the user, might be NULL. */
    astarte_device_property_unset_cbk_t user_property_unset_cbk;
    /** @brief Original user data for the callbacks provided by the user, might be NULL. */
    void *user_cbk_user_data;
    /** @brief UUID representing the Boot Id. */
    char boot_id[UUID_STR_LEN + 1];
    /** @brief OTA thread data used during the OTA Update operation. */
    ota_thread_t ota_thread;
    /** @brief LED thread data used during the LED blink. */
    led_thread_t led_thread;
    /** @brief Telemetry data. */
    edgehog_telemetry_t *telemetry;
};

/**
 * @brief Publish a telemetry.
 *
 * @details This function publishes a telemetry based on @p type parameter.
 *
 * @param device A valid edgehog device handle.
 * @param type The type of telemetry to publish.
 *
 */
void edgehog_device_publish_telemetry(
    edgehog_device_handle_t device, edgehog_telemetry_type_t type);

#endif // EDGEHOG_PRIVATE_H
