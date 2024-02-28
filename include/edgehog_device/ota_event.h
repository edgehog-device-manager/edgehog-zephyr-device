/*
 * (C) Copyright 2024, SECO Mind Srl
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef EDGEHOG_DEVICE_OTA_EVENT_H
#define EDGEHOG_DEVICE_OTA_EVENT_H

/**
 * @file ota_event.h
 * @brief Edgehog OTA event.
 */

#ifdef CONFIG_EDGEHOG_DEVICE_ZBUS_OTA_EVENT

#include <zephyr/zbus/zbus.h>

/**
 * @brief Edgehog event codes.
 */
typedef enum
{
    EDGEHOG_OTA_INVALID_EVENT = 0, /**< An invalid event. */
    EDGEHOG_OTA_INIT_EVENT, /**< Edgehog OTA routine init. */
    EDGEHOG_OTA_FAILED_EVENT, /**< Edgehog OTA routine failed. */
    EDGEHOG_OTA_SUCCESS_EVENT /**< Edgehog OTA routine successful. */
} edgehog_ota_event_t;

/**
 * @brief OTA Event.
 *
 * @details Defines an event occured during the OTA procedure, used as a type of the zbus
 * `edgehog_ota_chan`.
 *
 * @note Since the zbus channel definition accepts only struct or union,
 * we defined this struct with single field containg the OTA event occured.
 */
typedef struct
{
    /** @brief Edgehog OTA event. */
    edgehog_ota_event_t event;
} edgehog_ota_chan_event_t;

#endif

#endif // EDGEHOG_DEVICE_OTA_EVENT_H
