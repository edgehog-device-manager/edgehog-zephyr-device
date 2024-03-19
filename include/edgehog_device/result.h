/*
 * (C) Copyright 2024, SECO Mind Srl
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef EDGEHOG_DEVICE_RESULT_H
#define EDGEHOG_DEVICE_RESULT_H

/**
 * @file result.h
 * @brief Edgehog result types.
 */

/**
 * @brief Edgehog Device return codes.
 *
 * @details Edgehog Device return codes. EDGEHOG_RESULT_OK is always returned when no errors
 * occurred.
 */
typedef enum
{
    /** @brief No errors. */
    EDGEHOG_RESULT_OK = 0,
    /** @brief A generic error occurred. This is usually an internal Edgehog error. */
    EDGEHOG_RESULT_INTERNAL_ERROR = 1,
    /** @brief Invalid configuration for the required operation. */
    EDGEHOG_RESULT_INVALID_CONFIGURATION = 2,
    /** @brief An Astarte error occurred. This is usually propagating from the Astarte device SDK.
     */
    EDGEHOG_RESULT_ASTARTE_ERROR = 3,
    /** @brief The operation caused an out of memory error */
    EDGEHOG_RESULT_OUT_OF_MEMORY = 4,
} edgehog_result_t;

#endif // EDGEHOG_DEVICE_RESULT_H
