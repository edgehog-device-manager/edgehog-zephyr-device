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

#include "file_transfer/core.h"

/** @brief HTTP request timeout duration in milliseconds. */
#define EDGEHOG_FT_HTTP_REQ_TIMEOUT_MS (60 * 1000)

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
 * @brief Work queue handler for reporting file transfer progress to the server.
 *
 * @param work Pointer to the zephyr work structure embedded in the context.
 */
void edgehog_ft_progress_work_handler(struct k_work *work);

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
