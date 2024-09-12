/*
 * (C) Copyright 2024, SECO Mind Srl
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef OTA_H
#define OTA_H

/**
 * @file ota.h
 * @brief OTA APIs for Edgehog devices
 */

#include "edgehog_device/device.h"
#include "edgehog_device/result.h"

#include <zephyr/dfu/flash_img.h>
#include <zephyr/kernel.h>

/**
 * @brief OTA Request data.
 *
 * @details Defines an OTA request data received from server.
 */
typedef struct
{
    /** @brief OTA request UUID. */
    char *uuid;
    /** @brief OTA download url. */
    char *download_url;
} ota_request_t;

/**
 * @brief OTA Thread data.
 *
 * @details Defines the data used by OTA thread.
 */
typedef struct
{
    /** @brief OTA request data configured during OTA Event. */
    ota_request_t ota_request;
    /** @brief An Image flash context used for writing the image to the flash. */
    struct flash_img_context flash_ctx;
    /** @brief Size of the total OTA data downloaded. */
    size_t download_size;
    /** @brief Size of the OTA image. */
    size_t image_size;
    /** @brief Last download percentage sent to the server. */
    uint8_t last_perc_sent;
    /** @brief OTA thread running state. */
    atomic_t ota_run_state;
} ota_thread_data_t;

/** @brief Data struct for a ota thread instance. */
typedef struct
{
    /** @brief OTA thread data. */
    ota_thread_data_t ota_thread_data;
    /** @brief Thread handle. */
    struct k_thread ota_thread_handle;
} ota_thread_t;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief initialize Edgehog device OTA.
 *
 * @details This function initializes the OTA procedure and
 * if there is any pending OTA it completes it.
 *
 * @param edgehog_dev A valid Edgehog device handle.
 */
void edgehog_ota_init(edgehog_device_handle_t edgehog_dev);

/**
 * @brief receive Edgehog device OTA.
 *
 * @details This function receives an OTA event request from Astarte. This function may spawn a
 * new task to perform the OTA update.
 *
 * @param edgehog_dev A valid Edgehog device handle.
 * @param object_event A valid Astarte datastream object event.
 *
 * @return EDGEHOG_RESULT_OK if the OTA event is handled successfully, an edgehog_result_t
 * otherwise.
 */
edgehog_result_t edgehog_ota_event(
    edgehog_device_handle_t edgehog_dev, astarte_device_datastream_object_event_t *object_event);

#ifdef __cplusplus
}
#endif

#endif // OTA_H
