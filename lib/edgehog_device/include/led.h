/*
 * (C) Copyright 2024, SECO Mind Srl
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LED_H
#define LED_H

/**
 * @file led.h
 * @brief LED managing functions
 */

#include "edgehog_device/device.h"
#include "edgehog_device/result.h"

#include <zephyr/kernel.h>

/** @brief The devicetree node identifier for the Edgehog LED alias. */
#define EDGEHOG_LED_NODE DT_ALIAS(edgehog_led)

/** @brief Data struct for a led thread instance. */
typedef struct
{
    /** @brief LED thread running state. */
    atomic_t led_run_state;
    /** @brief LED k_thread_handle. */
    struct k_thread led_thread_handle;
    /** @brief LED stop timer. */
    struct k_timer led_blink_timer;
} led_thread_t;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Handle received Edgehog device LED event.
 *
 * @details This function handles an LED event request from Astarte.
 *
 * @param edgehog_dev A valid Edgehog device handle.
 * @param event A valid Astarte datastream individual event.
 *
 * @return EDGEHOG_RESULT_OK if the LED event is handled successfully, an edgehog_result_t
 * otherwise.
 */
edgehog_result_t edgehog_led_event(
    edgehog_device_handle_t edgehog_dev, astarte_device_datastream_individual_event_t *event);

#ifdef __cplusplus
}
#endif

#endif /* LED_H */
