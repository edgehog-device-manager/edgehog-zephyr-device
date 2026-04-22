/*
 * (C) Copyright 2026, SECO Mind Srl
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef FILE_TRANSFER_DOWNLOAD_H
#define FILE_TRANSFER_DOWNLOAD_H

/**
 * @file file_transfer/download.h
 * @brief Download APIs for file transfer.
 */

#include "file_transfer/core.h"

/** @brief Callbacks functions for write on file operations. */
typedef struct
{
    /** @brief Initializes the storage backend and returns a context. */
    edgehog_result_t (*file_init)(void **ctx, edgehog_ft_cbks_t *cbks, char *identifier, char *url,
        size_t expected_file_size, char *destination);
    /** @brief Appends a chunk of data to the storage backend. */
    edgehog_result_t (*file_append_chunk)(void *ctx, const uint8_t *chunk_data, size_t chunk_size);
    /** @brief Finalizes and closes the file transfer successfully. */
    edgehog_result_t (*file_complete)(void *ctx);
    /** @brief Aborts the transfer and cleans up resources (e.g., deletes partial file). */
    void (*file_abort)(void *ctx);
} edgehog_ft_file_write_cbks_t;

/**
 * @brief Receive Edgehog device File Transfer Server-to-Device event.
 * @details This function receives an FT event request from Astarte and forwards it to the
 * thread handling it through a message queue.
 *
 * @param device A valid Edgehog device handle.
 * @param object_event A valid Astarte datastream object event.
 *
 * @return EDGEHOG_RESULT_OK if the FT event is handled successfully, an edgehog_result_t
 * otherwise.
 */
edgehog_result_t edgehog_ft_server_to_device_event(
    edgehog_device_handle_t device, astarte_device_datastream_object_event_t *object_event);

/**
 * @brief Handle a server-to-device file transfer operation.
 *
 * @param edgehog_device A valid Edgehog device handle.
 * @param msg Pointer to the server-to-device message payload.
 */
void edgehog_ft_handle_server_to_device(
    edgehog_device_handle_t edgehog_device, edgehog_ft_msg_t *msg);

#endif // FILE_TRANSFER_DOWNLOAD_H
