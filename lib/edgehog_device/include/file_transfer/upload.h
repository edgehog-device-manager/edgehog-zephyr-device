/*
 * (C) Copyright 2026, SECO Mind Srl
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef FILE_TRANSFER_UPLOAD_H
#define FILE_TRANSFER_UPLOAD_H

/**
 * @file file_transfer/upload.h
 * @brief Upload APIs for file transfer.
 */

#include "file_transfer/core.h"

/** @brief Callbacks functions for read from file operations. */
typedef struct
{
    /** @brief Initializes the storage backend and returns a context. */
    edgehog_result_t (*file_init)(
        void **ctx, edgehog_ft_cbks_t *cbks, char *identifier, char *source);
    /** @brief Reads a chunk of data from the storage backend. */
    edgehog_result_t (*file_read_chunk)(
        void *ctx, size_t max_length, uint8_t **chunk_data, size_t *chunk_size, bool *last_chunk);
    /** @brief Finalizes and closes the file transfer successfully. */
    edgehog_result_t (*file_complete)(void *ctx);
    /** @brief Aborts the transfer and cleans up resources. */
    void (*file_abort)(void *ctx);
} edgehog_ft_file_read_cbks_t;

/**
 * @brief Receive Edgehog device File Transfer Device-to-Server event.
 * @details This function receives an FT event request from Astarte and forwards it to the
 * thread handling it through a message queue.
 *
 * @param device A valid Edgehog device handle.
 * @param object_event A valid Astarte datastream object event.
 *
 * @return EDGEHOG_RESULT_OK if the FT event is handled successfully, an edgehog_result_t
 * otherwise.
 */
edgehog_result_t edgehog_ft_device_to_server_event(
    edgehog_device_handle_t device, astarte_device_datastream_object_event_t *object_event);

/**
 * @brief Handle a device-to-server file transfer operation.
 *
 * @param edgehog_device A valid Edgehog device handle.
 * @param msg Pointer to the device-to-server message payload.
 */
void edgehog_ft_handle_device_to_server(
    edgehog_device_handle_t edgehog_device, edgehog_ft_msg_t *msg);

#endif // FILE_TRANSFER_UPLOAD_H
