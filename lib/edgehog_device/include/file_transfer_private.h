#ifndef FILE_TRANSFER_PRIVATE_H
#define FILE_TRANSFER_PRIVATE_H

/**
 * @file file_transfer_private.h
 * @brief Private File Transfer fields.
 */

#include "edgehog_device/device.h"

#include <zephyr/kernel.h>

#define EDGEHOG_FILE_TRANSFER_LEN 10

typedef struct
{
    char *id;
    char *url;
    char *http_header_key;
    char *http_header_value;
    int64_t file_size_bytes;
    bool progress;
} ft_server_to_device_data_t;

typedef struct
{
    /** @brief File transfer entries list. */
    ft_server_to_device_data_t *entries[EDGEHOG_FILE_TRANSFER_LEN];
    /** @brief Message queue. */
    struct k_msgq msgq;
    /** @brief Ring buffer that holds queued messages. */
    // TODO: at the moment we are only sending int32 values for testing
    char msgq_buffer[EDGEHOG_FILE_TRANSFER_LEN * sizeof(ft_server_to_device_data_t)];
    /** @brief Telemetry service thread, peeks the msgq and transmits eventual messages. */
    struct k_thread thread;
    /** @brief Run state for the telemetry service thread. */
    atomic_t thread_state;
} edgehog_file_transfer_t;

edgehog_file_transfer_t *edgehog_file_transfer_new();

edgehog_result_t edgehog_file_transfer_start(edgehog_device_handle_t device);

/**
 * @brief receive Edgehog device File Transfer (Server to Device) event.
 *
 * @details This function receives an FT event request from Astarte and forward it to the
 * thread handling it through a message queue.
 *
 * @param edgehog_dev A valid Edgehog device handle.
 * @param object_event A valid Astarte datastream object event.
 *
 * @return EDGEHOG_RESULT_OK if the FT event is handled successfully, an edgehog_result_t
 * otherwise.
 */
edgehog_result_t edgehog_file_transfer_event(
    edgehog_device_handle_t device, astarte_device_datastream_object_event_t *object_event);

edgehog_result_t edgehog_file_transfer_stop(
    edgehog_file_transfer_t *file_transfer, k_timeout_t timeout);

void edgehog_file_transfer_destroy(edgehog_file_transfer_t *ft);

bool edgehog_file_transfer_is_running(edgehog_file_transfer_t *file_transfer);

#endif // FILE_TRANSFER_PRIVATE_H
