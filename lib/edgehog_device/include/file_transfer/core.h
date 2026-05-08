/*
 * (C) Copyright 2026, SECO Mind Srl
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef FILE_TRANSFER_PRIVATE_H
#define FILE_TRANSFER_PRIVATE_H

/**
 * @file file_transfer/core.h
 * @brief Core file transfer APIs.
 */

#include "edgehog_device/device.h"
#include "edgehog_device/file_transfer.h"

#include <zephyr/kernel.h>

/**
 * @brief Encodings and compressions for file transfers.
 */
enum edgehog_ft_encoding
{
    /** @brief No encoding applied. */
    EDGEHOG_FT_ENCODING_NONE = 0,
    /** @brief LZ4 compression encoding. */
    EDGEHOG_FT_ENCODING_LZ4,
    /** @brief Tar archive encoding. */
    EDGEHOG_FT_ENCODING_TAR,
    /** @brief Tar archive with LZ4 compression. */
    EDGEHOG_FT_ENCODING_TAR_LZ4,
    /** @brief Unsupported encoding. */
    EDGEHOG_FT_ENCODING_UNSUPPORTED,
};

/** @brief Wrapper for file transfer messages sent through the message queue. */
typedef struct
{
    /** @brief Transfer ID for the file. */
    char *id;
    /** @brief The URL to get the file from. */
    char *url;
    /** @brief HTTP headers, as a NULL terminated list. */
    char **http_headers;
    /** @brief Optional encoding for the file transfer payload. */
    enum edgehog_ft_encoding encoding;
    /** @brief Flag to enable the progress reporting of the transfer. */
    bool progress;
    /** @brief A SHA-256 hash for the file to transfer, prefixed by "sha256:". */
    char *digest;
    /** @brief Location for the file (streaming, filesystem). */
    char *location_type;
    /** @brief Location-specific information on how to save the file. */
    char *location;
    /** @brief The type of the message, indicating which payload to read. */
    edgehog_ft_type_t type;
    /** @brief Total expected decompressed file size in bytes (server-to-device transfers). */
    int64_t file_size_bytes;
} edgehog_ft_msg_t;

/** @brief Data structure for the file transfer service. */
typedef struct
{
    /** @brief Message queue used to pass transfer requests to the service thread. */
    struct k_msgq msgq;
    /** @brief File transfer service thread, peeks the msgq and performs transfers if present. */
    struct k_thread thread;
    /** @brief Run state for the file transfer service thread. */
    atomic_t thread_state;
    /** @brief File transfer callbacks registered by the user. */
    edgehog_ft_cbks_t cbks;
    /** @brief The file system partitions explicitly allowed for file transfers. */
    edgehog_ft_filesystem_partition_t *partitions;
    /** @brief The length of the partitions array. */
    size_t partitions_len;
} edgehog_ft_t;

/**
 * @brief Publishes the file transfer capabilities for the Edgehog device.
 * @param edgehog_device A valid Edgehog device handle.
 */
void edgeghog_ft_publish_capabilities(edgehog_device_handle_t edgehog_device);

/**
 * @brief Creates a new Edgehog file transfer instance.
 *
 * @param cbks File transfer callbacks registered by the user.
 * @param partitions Pointer to the file system partitions explicitly allowed for file transfers.
 * @param partitions_len The number of partitions in the array.
 * @return A pointer to the newly allocated edgehog_ft_t instance, or NULL if allocation fails.
 */
edgehog_ft_t *edgehog_ft_new(
    edgehog_ft_cbks_t cbks, edgehog_ft_filesystem_partition_t *partitions, size_t partitions_len);

/**
 * @brief Frees all resources associated with a file transfer context.
 * @param file_transfer The file transfer context to destroy.
 */
void edgehog_ft_destroy(edgehog_ft_t *file_transfer);

/**
 * @brief Starts the file transfer background service/thread.
 * @param device A valid Edgehog device handle.
 * @return EDGEHOG_RESULT_OK on success, otherwise an error code.
 */
edgehog_result_t edgehog_ft_start(edgehog_device_handle_t device);

/**
 * @brief Signals the file transfer service to stop processing and wait for termination.
 * @param file_transfer The file transfer context to stop.
 * @param timeout Maximum time to wait for the thread to terminate.
 * @return EDGEHOG_RESULT_OK on successful stop, otherwise an error code.
 */
edgehog_result_t edgehog_ft_stop(edgehog_ft_t *file_transfer, k_timeout_t timeout);

/**
 * @brief Checks if the file transfer service thread is currently running.
 * @param file_transfer The file transfer context to check.
 * @return true if running, false otherwise.
 */
bool edgehog_ft_is_running(edgehog_ft_t *file_transfer);

/**
 * @brief Processes a file transfer event and enqueues it for the handler thread.
 *
 * @param device A valid Edgehog device handle.
 * @param object_event A valid Astarte datastream object event containing the transfer parameters.
 * @param type The type of file transfer operation (server-to-device or device-to-server).
 * @return EDGEHOG_RESULT_OK if successfully processed and enqueued, otherwise an error code.
 */
edgehog_result_t edgehog_ft_process_event(edgehog_device_handle_t device,
    astarte_device_datastream_object_event_t *object_event, edgehog_ft_type_t type);

#endif // FILE_TRANSFER_PRIVATE_H
