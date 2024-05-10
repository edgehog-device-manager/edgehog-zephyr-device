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
    /** @brief The operation caused an out of memory error. */
    EDGEHOG_RESULT_OUT_OF_MEMORY = 4,
    /** @brief A generic network error occurred. */
    EDGEHOG_RESULT_NETWORK_ERROR = 5,
    /** @brief A generic error occurred when dealing with NVS. */
    EDGEHOG_RESULT_NVS_ERROR = 6,
    /** @brief Invalid OTA update request received. */
    EDGEHOG_RESULT_OTA_INVALID_REQUEST = 7,
    /** @brief An HTTP request could not be processed. */
    EDGEHOG_RESULT_HTTP_REQUEST_ERROR = 8,
    /** @brief Attempted to perform OTA operation while another is still active. */
    EDGEHOG_RESULT_OTA_ALREADY_IN_PROGRESS = 9,
    /** @brief Invalid OTA image received. */
    EDGEHOG_RESULT_OTA_INVALID_IMAGE = 10,
    /** @brief An error occurred during erase second slot procedure. */
    EDGEHOG_RESULT_OTA_ERASE_SECOND_SLOT_ERROR = 11,
    /** @brief An error occurred during flash area write procedure. */
    EDGEHOG_RESULT_OTA_INIT_FLASH_ERROR = 12,
    /** @brief An error occurred during flash area write procedure. */
    EDGEHOG_RESULT_OTA_WRITE_FLASH_ERROR = 13,
    /** @brief The OTA procedure booted from the wrong partition. */
    EDGEHOG_RESULT_OTA_SYSTEM_ROLLBACK = 14,
    /** @brief OTA update aborted by Edgehog half way during the procedure. */
    EDGEHOG_RESULT_OTA_CANCELED = 15,
    /** @brief An error occurred during OTA procedure. */
    EDGEHOG_RESULT_OTA_INTERNAL_ERROR = 16,
    /** @brief Unable to swap the contents to the slot 1. */
    EDGEHOG_RESULT_OTA_SWAP_FAIL = 17,
    /** @brief Tried to perform an operation on a Device in a non-ready or initialized state. */
    EDGEHOG_RESULT_DEVICE_NOT_READY = 18,
    /** @brief k_thread_create was unable to spawn a new task. */
    EDGEHOG_RESULT_THREAD_CREATE_ERROR = 19,
    /** @brief Invalid Command request received. */
    EDGEHOG_RESULT_COMMAND_INVALID_REQUEST = 20,
} edgehog_result_t;

#endif // EDGEHOG_DEVICE_RESULT_H
