/*
 * (C) Copyright 2024, SECO Mind Srl
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef EDGEHOG_DEVICE_TELEMETRY_H
#define EDGEHOG_DEVICE_TELEMETRY_H

/**
 * @file telemetry.h
 * @brief Edgehog telemtry fields.
 */

/**
 * @defgroup telemetry Telemetry
 * @ingroup edgehog_device
 * @{
 */

/**
 * @brief Edgehog telemetry types.
 *
 * @details This enum is used for configuring the telemetry type in
 * `edgehog_device_telemetry_config_t` struct.
 */
typedef enum
{
    /** @brief The telemetry type is invalid. */
    EDGEHOG_TELEMETRY_INVALID = 0,
    /** @brief The hardware info telemetry type. */
    EDGEHOG_TELEMETRY_HW_INFO,
    /** @brief The wifi scan telemetry type. */
    EDGEHOG_TELEMETRY_WIFI_SCAN,
    /** @brief The system status telemetry type. */
    EDGEHOG_TELEMETRY_SYSTEM_STATUS,
    /** @brief The storage usage telemetry type. */
    EDGEHOG_TELEMETRY_STORAGE_USAGE,
    /** @brief Don't use it, It is a placeholder for the enum len. */
    EDGEHOG_TELEMETRY_LEN
} telemetry_type_t;

/**
 * @brief Edgehog device telemtry configuration struct
 *
 * Example:
 *  edgehog_device_telemetry_config_t telemetry_config =
 *  {
 *      .type = EDGEHOG_TELEMETRY_WIFI_SCAN,
 *      .period_seconds = 5
 *   };
 */
typedef struct
{
    /** @brief Type of telemetry. */
    telemetry_type_t type;
    /** @brief Interval of period in seconds. */
    long period_seconds;
} edgehog_device_telemetry_config_t;

/**
 * @}
 */

#endif // EDGEHOG_DEVICE_TELEMETRY_H
