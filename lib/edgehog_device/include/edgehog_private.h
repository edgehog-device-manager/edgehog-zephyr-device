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

/**
 * @brief Internal struct for an instance of an Edgehog device.
 *
 * @warning Users should not modify the content of this struct directly
 */
struct edgehog_device_t
{
    /** @brief Handle of an Astarte device. */
    astarte_device_handle_t astarte_device;
    /** @brief UUID representing the Boot Id. */
    char boot_id[UUID_STR_LEN + 1];
    /** @brief OTA thread data used during the OTA Update operation. */
    ota_thread_t ota_thread;
    /** @brief LED thread data used during the LED blink. */
    led_thread_t led_thread;
    /** @brief Telemetry data ... */
    edgehog_telemetry_t *telemetry;
};

/**
 * @brief Publish a telemetry.
 *
 * @details This function publishs a telemetry based on telemetry_type parameter.
 *
 * @param device A valid edgehog device handle.
 * @param type One of edgehog_telemetry_type_t values.
 *
 */
void edgehog_device_publish_telemetry(
    edgehog_device_handle_t device, edgehog_telemetry_type_t type);

#endif // EDGEHOG_PRIVATE_H
