/*
 * (C) Copyright 2024, SECO Mind Srl
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef EDGEHOG_DEVICE_TELEMETRY_H
#define EDGEHOG_DEVICE_TELEMETRY_H

/**
 * @file telemetry.h
 */

/**
 * @defgroup telemetry Telemetry
 * @brief API for the telemetry service.
 * @details
 * The telemetry service periodically transmit information from the device to Edgehog.
 * The #edgehog_telemetry_type_t enum defines the types of information that can be transmitted
 * by this device.
 * Each telemetry type can be scheduled for transmission independently setting the telemetry
 * configuration in the #edgehog_device_config_t struct.
 * @ingroup edgehog_device
 * @{
 */

/**
 * @brief Edgehog telemetry types.
 *
 * @details This is a selection of the telemetry types that the Edgehog device currently
 * supports. The types in this enum can be used to configure the telemetry in service with the
 * #edgehog_telemetry_config_t struct.
 */
typedef enum
{
    /** @brief Invalid telemetry entry. */
    EDGEHOG_TELEMETRY_INVALID = 0,
    /** @brief Hardware info telemetry type. */
    EDGEHOG_TELEMETRY_HW_INFO,
    /** @brief WiFi scan telemetry type. */
    EDGEHOG_TELEMETRY_WIFI_SCAN,
    /** @brief System status telemetry type. */
    EDGEHOG_TELEMETRY_SYSTEM_STATUS,
    /** @brief Storage usage telemetry type. */
    EDGEHOG_TELEMETRY_STORAGE_USAGE,
    /** @brief Don't use it, It is a placeholder for the enum len. */
    EDGEHOG_TELEMETRY_LEN
} edgehog_telemetry_type_t;

/**
 * @brief Edgehog device telemetry configuration struct.
 *
 * Example:
 * ```C
 *     edgehog_telemetry_config_t telemetry_config = {
 *         .type = EDGEHOG_TELEMETRY_WIFI_SCAN,
 *         .period_seconds = 5
 *     };
 * ```
 */
typedef struct
{
    /** @brief Type of telemetry. */
    edgehog_telemetry_type_t type;
    /** @brief Interval of transmission in seconds. */
    long period_seconds;
} edgehog_telemetry_config_t;

/**
 * @}
 */

#endif // EDGEHOG_DEVICE_TELEMETRY_H
