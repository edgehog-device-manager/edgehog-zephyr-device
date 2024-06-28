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

#include <astarte_device_sdk/device.h>
#include <astarte_device_sdk/uuid.h>

struct edgehog_device_t
{
    /** @brief Handle of an Astarte device. */
    astarte_device_handle_t astarte_device;
    /** @brief UUID representing the Boot Id. */
    char boot_id[ASTARTE_UUID_STR_LEN];
    /** @brief OTA thread data used during the OTA Update operation. */
    ota_thread_t ota_thread;
    /** @brief LED thread data used during the LED blink. */
    led_thread_t led_thread;
};

#endif // EDGEHOG_PRIVATE_H
