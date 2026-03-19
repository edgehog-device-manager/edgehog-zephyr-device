/*
 * (C) Copyright 2026, SECO Mind Srl
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef FILE_TRANSFER_PRIVATE_H
#define FILE_TRANSFER_PRIVATE_H

/**
 * @file file_transfer_private.h
 * @brief Private File Transfer fields.
 */

#include "edgehog_device/device.h"

#include <zephyr/kernel.h>

/** @brief Maximum number of file transfer messages in the queue. */
#define EDGEHOG_FILE_TRANSFER_LEN 10

/**
 * @brief Data payload for a Server-to-Device file transfer request.
 */
typedef struct
{
    /** @brief Transfer ID for the file. */
    char *id;
    /** @brief The URL to get the file from. */
    char *url;
    /** @brief Keys for the HTTP headers, must be in the order of the values. */
    char *http_header_key;
    /** @brief Values for the HTTP headers, must be in the order of the keys. */
    char *http_header_value;
    /** @brief Total decompressed file size in bytes. */
    int64_t file_size_bytes;
    /** @brief Flag to enable the progress reporting of the download. */
    bool progress;
    // TODO: add other fields from the io.edgehog.devicemanager.fileTransfer.posix.ServerToDevice
    // interface
} ft_server_to_device_data_t;

/**
 * @brief Data payload for a Device-to-Server file transfer request.
 */
typedef struct
{
    /** @brief Transfer ID for the file. */
    char *id;
    /** @brief The URL to upload the file to. */
    char *url;
    /** @brief Keys for the HTTP headers, must be in the order of the values. */
    char *http_header_key;
    /** @brief Values for the HTTP headers, must be in the order of the keys. */
    char *http_header_value;
    /** @brief Flag to enable the progress reporting of the upload. */
    bool progress;
    /** @brief Source from which the file should be read from (storage, streaming, filesystem). */
    char *source_type;
    /** @brief Source-specific information on what to upload to the server for the source. */
    char *source;
    // TODO: add other fields from the io.edgehog.devicemanager.fileTransfer.DeviceToServer
    // interface
} ft_device_to_server_data_t;

/**
 * @brief Identifies the direction of the file transfer message.
 */
typedef enum
{
    /** @brief indicates a file download from the server to the device. */
    FT_MSG_SERVER_TO_DEVICE = 0,
    /** @brief indicates a file upload from the device to the server. */
    FT_MSG_DEVICE_TO_SERVER = 1,
} ft_msg_type_t;

/**
 * @brief Wrapper for file transfer messages sent through the message queue.
 */
typedef struct
{
    /** @brief The type of the message, indicating which payload to read. */
    ft_msg_type_t type;

    /** @brief Union containing the specific file transfer request data. */
    union
    {
        /** @brief Payload for a server-to-device download request. */
        ft_server_to_device_data_t server_to_device;
        /** @brief Payload for a device-to-server upload request. */
        ft_device_to_server_data_t device_to_server;
    } payload;

} ft_msgq_data_t;

/**
 * @brief Context structure managing the file transfer service state and thread.
 */
typedef struct
{
    /** @brief File transfer entries list. */
    ft_msgq_data_t *entries[EDGEHOG_FILE_TRANSFER_LEN];
    /** @brief Message queue. */
    struct k_msgq msgq;
    /** @brief Ring buffer that holds queued messages. */
    char msgq_buffer[EDGEHOG_FILE_TRANSFER_LEN * sizeof(ft_msgq_data_t)];
    /** @brief Telemetry service thread, peeks the msgq and transmits eventual messages. */
    struct k_thread thread;
    /** @brief Run state for the telemetry service thread. */
    atomic_t thread_state;
} edgehog_file_transfer_t;

/**
 * @brief Allocates and initializes a new file transfer context.
 * @return A pointer to the newly created context, or NULL on failure.
 */
edgehog_file_transfer_t *edgehog_file_transfer_new();

/**
 * @brief Starts the file transfer background service/thread.
 * @param device A valid Edgehog device handle.
 * @return EDGEHOG_RESULT_OK on success, otherwise an error code.
 */
edgehog_result_t edgehog_ft_start(edgehog_device_handle_t device);

/**
 * @brief Receive Edgehog device File Transfer Server-to-Device event.
 *
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
 * @brief Receive Edgehog device File Transfer Device-to-Server event.
 *
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
 * @brief Signals the file transfer service to stop processing and wait for termination.
 * @param file_transfer The file transfer context to stop.
 * @param timeout Maximum time to wait for the thread to terminate.
 * @return EDGEHOG_RESULT_OK on successful stop, otherwise an error code.
 */
edgehog_result_t edgehog_ft_stop(edgehog_file_transfer_t *file_transfer, k_timeout_t timeout);

/**
 * @brief Frees all resources associated with a file transfer context.
 * * @param file_transfer The file transfer context to destroy.
 */
void edgehog_ft_destroy(edgehog_file_transfer_t *file_transfer);

/**
 * @brief Checks if the file transfer service thread is currently running.
 * * @param file_transfer The file transfer context to check.
 * @return true if running, false otherwise.
 */
bool edgehog_ft_is_running(edgehog_file_transfer_t *file_transfer);

#endif // FILE_TRANSFER_PRIVATE_H
