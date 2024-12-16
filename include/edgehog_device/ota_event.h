/*
 * (C) Copyright 2024, SECO Mind Srl
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef EDGEHOG_DEVICE_OTA_EVENT_H
#define EDGEHOG_DEVICE_OTA_EVENT_H

#ifdef CONFIG_EDGEHOG_DEVICE_ZBUS_OTA_EVENT

/**
 * @file ota_event.h
 */

/**
 * @defgroup ota OTA event
 * @brief API for an OTA update event.
 * @ingroup edgehog_device
 * @{
 */

#include <zephyr/zbus/zbus.h>

/** @brief Edgehog ota event codes. */
typedef enum
{
    /** @brief An invalid event. */
    EDGEHOG_OTA_INVALID_EVENT = 0,
    /** @brief Edgehog OTA routine init. */
    EDGEHOG_OTA_INIT_EVENT,
    /** @brief Edgehog OTA routine pending reboot. */
    EDGEHOG_OTA_PENDING_REBOOT_EVENT,
    /** @brief Edgehog OTA routine reboot confirmation. */
    EDGEHOG_OTA_CONFIRM_REBOOT_EVENT,
    /** @brief Edgehog OTA routine failed. */
    EDGEHOG_OTA_FAILED_EVENT,
    /** @brief Edgehog OTA routine successful. */
    EDGEHOG_OTA_SUCCESS_EVENT
} edgehog_ota_event_t;

/**
 * @brief OTA Event.
 *
 * @details Defines an event occured during the OTA procedure, used as a type of the zbus Edgehog
 * OTA channel.
 * @note Since the zbus channel definition accepts only struct or union, we defined this struct
 * with a single field containg the OTA event occured.
 */
typedef struct
{
    /** @brief Edgehog OTA event. */
    edgehog_ota_event_t event;
} edgehog_ota_chan_event_t;

/**
 * @brief Declaration for the zbus OTA channel.
 *
 * @details The variable @p edgehog_ota_chan is defined within the Edgehog device. It should be used
 * to add observers, for example using ZBUS_CHAN_ADD_OBS.
 */
ZBUS_CHAN_DECLARE(edgehog_ota_chan);

/**
 * @}
 */

#endif

#endif // EDGEHOG_DEVICE_OTA_EVENT_H
