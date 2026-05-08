/*
 * (C) Copyright 2026, SECO Mind Srl
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef FILE_TRANSFER_UTILS_H
#define FILE_TRANSFER_UTILS_H

/**
 * @file file_transfer/utils.h
 * @brief Utility APIs for file transfer.
 */

#include "edgehog_device/device.h"

#include <psa/crypto.h>

#include "file_transfer/compression.h"
#include "file_transfer/core.h"
#include "file_transfer/decompression.h"

/** @brief HTTP request timeout duration in milliseconds. */
#define EDGEHOG_FT_HTTP_REQ_TIMEOUT_MS (60 * 1000)

/** @brief Size for the output buffer used to temporarely store the compressed chunk. */
#define EDGEHOG_FT_COMPRESSED_OUT_BUFFER_SIZE 1024

/** @brief Unified user data for HTTP callbacks. */
typedef struct
{
    /** @brief Edgehog device handle. */
    edgehog_device_handle_t edgehog_device;
    /** @brief Transfer ID for the file. */
    char *id;
    /** @brief Flag to enable the progress reporting of the transfer. */
    bool progress;
    /** @brief The type of the file transfer, indicating which callbacks to use. */
    edgehog_ft_type_t type;
    /** @brief Generic file callbacks, used to manage file operations */
    const void *file_cbks;
    /** @brief File callbacks context */
    void *file_cbks_ctx;
    /** @brief posix error number */
    int posix_errno;
    /** @brief Message to send to Astarte in case of success or error of a FT request */
    char *message;
    /** @brief Worker to handle progress updates */
    struct k_work progress_work;
    /** @brief The number of received bytes to be transmitted as progress */
    size_t transferred_bytes;
    /** @brief The total expected bytes to be processed */
    size_t total_bytes;
    /** @brief The number of bytes at the last progress report */
    atomic_t last_reported_bytes;
    /** @brief The expected digest string for verification (e.g., "sha256:...") */
    const char *expected_digest;
    /** @brief The hash operation context for streaming digest calculation */
    psa_hash_operation_t hash_operation;
    /** @brief Optional encoding for the file transfer payload. */
    enum edgehog_ft_encoding encoding;
#ifdef CONFIG_EDGEHOG_DEVICE_FILE_TRANSFER_COMPRESSION
    /** @brief Decompression context for incoming downloaded files */
    file_transfer_decompression_ctx_t decomp_ctx;
    /** @brief Compression context for outgoing uploaded files */
    file_transfer_compression_ctx_t comp_ctx;
    /** @brief Buffer used to hold the compressed output chunk during uploads */
    uint8_t comp_out_buf[EDGEHOG_FT_COMPRESSED_OUT_BUFFER_SIZE];
    /** @brief Track if the underlying file is fully read */
    bool file_exhausted;
    /** @brief Track if the LZ4 footer has been successfully written */
    bool comp_footer_written;
#endif
} edgehog_ft_http_cbk_data_t;

/**
 * @brief Parses an array of Astarte object entries and initializes a file transfer message.
 *
 * @param rx_values Array of incoming Astarte object entries to parse.
 * @param rx_values_size The number of elements in the rx_values array.
 * @param type The type of file transfer operation (download or upload).
 * @param msg Pointer to the file transfer message struct to initialize.
 * @return EDGEHOG_RESULT_OK on successful initialization, otherwise an error code.
 */
edgehog_result_t edgehog_ft_msg_init(astarte_object_entry_t *rx_values, size_t rx_values_size,
    edgehog_ft_type_t type, edgehog_ft_msg_t *msg);

/**
 * @brief Destroys a file transfer message and frees all dynamically allocated memory.
 *
 * @param msg Pointer to the file transfer message to destroy.
 */
void edgehog_ft_msg_destroy(edgehog_ft_msg_t *msg);

/**
 * @brief Allocates and initializes a new HTTP callback data structure.
 * @details This function allocates a new context for HTTP callbacks, initializing it with the
 * provided device handle, file transfer message, and callback parameters.
 *
 * @param edgehog_device The Edgehog device handle.
 * @param msg Pointer to the file transfer message containing initial transfer data.
 * @param file_cbks Pointer to the generic file operations callbacks.
 * @param file_cbks_ctx Pointer to the context to be passed to the file callbacks.
 * @return A pointer to the newly allocated edgehog_ft_http_cbk_data_t structure,
 * or NULL if memory allocation fails.
 */
edgehog_ft_http_cbk_data_t *edgehog_ft_http_cbk_data_new(edgehog_device_handle_t edgehog_device,
    edgehog_ft_msg_t *msg, const void *file_cbks, void *file_cbks_ctx);

/**
 * @brief Destroys the HTTP callback data structure.
 * @details Safely cleans up the callback data by canceling any pending progress
 * reporting work queues synchronously and freeing the allocated memory.
 *
 * @param data Pointer to the callback data structure to destroy.
 */
void edgehog_ft_http_cbk_data_destroy(edgehog_ft_http_cbk_data_t *data);

/**
 * @brief Updates the file transfer progress and triggers a report if necessary.
 * @details Increments the transferred bytes count and evaluates whether a progress report should
 * be sent to the server. A report is submitted to the work queue if the transferred amount exceeds
 * predefined percentage/byte thresholds, or if it is the final data chunk.
 *
 * @param data Pointer to the HTTP callback data structure managing the transfer.
 * @param chunk_size The size in bytes of the most recently processed chunk.
 * @param last_chunk Boolean indicating whether this is the final chunk of the transfer.
 */
void edgehog_ft_update_progress(
    edgehog_ft_http_cbk_data_t *data, size_t chunk_size, bool last_chunk);

/**
 * @brief Send the final response or error result for a file transfer operation.
 *
 * @param device A valid Edgehog device handle.
 * @param identifier The unique identifier for the file transfer.
 * @param type The type of transfer.
 * @param in_errno Underlying system error code (e.g., errno), if applicable.
 * @param in_msg Human-readable error or success message.
 * @param eres The edgehog result code to use as fallback if no errno is available.
 */
void edgehog_ft_send_response(edgehog_device_handle_t device, const char *identifier,
    edgehog_ft_type_t type, int in_errno, const char *in_msg, edgehog_result_t eres);

#endif // FILE_TRANSFER_UTILS_H
