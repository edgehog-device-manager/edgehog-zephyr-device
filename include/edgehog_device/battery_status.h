/*
 * (C) Copyright 2024, SECO Mind Srl
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef EDGEHOG_DEVICE_BATTERY_STATUS_H
#define EDGEHOG_DEVICE_BATTERY_STATUS_H

/**
 * @file battery_status.h
 */

/**
 * @defgroup battery Battery status
 * @brief API for device battery status.
 * @ingroup edgehog_device
 * @{
 */

#include "edgehog_device/device.h"
#include "edgehog_device/result.h"

/** @brief Edgehog battery state codes. */
typedef enum
{
    /** @brief The battery state for the device is invalid. */
    BATTERY_INVALID = 0,
    /** @brief The device is plugged into power and the battery is 100% charged. */
    BATTERY_IDLE,
    /** @brief The device is plugged into power and the battery is less than 100% charged. */
    BATTERY_CHARGING,
    /** @brief The device is not plugged into power; the battery is discharging. */
    BATTERY_DISCHARGING,
    /** @brief The battery state for the device cannot be distinguished between "Idle" and
       "Charging". */
    BATTERY_IDLE_OR_CHARGING,
    /** @brief A generic failure occurred. */
    BATTERY_FAILURE,
    /** @brief Battery removed from the device */
    BATTERY_REMOVED,
    /** @brief The battery state for the device cannot be determined. */
    BATTERY_UNKNOWN
} edgehog_battery_state_t;

/** @brief Battery status struct. */
typedef struct
{
    /** @brief Battery slot name. */
    const char *battery_slot;
    /** @brief Charge level in [0.0%-100.0%] range, such as 89.0%. */
    double level_percentage;
    /** @brief The level measurement absolute error in [0.0-100.0] range. */
    double level_absolute_error;
    /** @brief Any value of edgehog_battery_state such as #BATTERY_CHARGING. */
    edgehog_battery_state_t battery_state;
} edgehog_battery_status_t;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Publish battery status info.
 *
 * @details This function publishes to Edgehog all available battery status updates.
 *
 * @param edgehog_device A valid Edgehog device handle.
 * @param battery_status A battery status structure that contains the current battery status. It can
 * be safely allocated on the stack, a copy of it is created internally.
 *
 * @return #EDGEHOG_RESULT_OK if publish has been successful, an error code otherwise.
 */
edgehog_result_t edgehog_battery_status_publish(
    edgehog_device_handle_t edgehog_device, const edgehog_battery_status_t *battery_status);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif // EDGEHOG_DEVICE_BATTERY_STATUS_H
