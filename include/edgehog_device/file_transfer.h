/*
 * (C) Copyright 2026, SECO Mind Srl
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef EDGEHOG_DEVICE_FILE_TRANSFER_H
#define EDGEHOG_DEVICE_FILE_TRANSFER_H

#include <stdbool.h>
#include <zephyr/kernel.h>

/**
 * @file file_transfer.h
 */

/**
 * @defgroup file_transfer File transfer
 * @brief API for file transfer.
 * @ingroup edgehog_device
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Event flag indicating the end of a file transfer stream. */
#define EDGEHOG_FT_EOF_EVENT_FLAG (1U << 0U)
/** @brief Event flag indicating an acknowledgement in the file transfer stream. */
#define EDGEHOG_FT_ACK_EVENT_FLAG (1U << 1U)

/** @brief Direction of the file transfer */
typedef enum
{
    /** @brief Transfer originates from the server and goes to the device. */
    EDGEHOG_FT_TYPE_SERVER_TO_DEVICE,
    /** @brief Transfer originates from the device and goes to the server. */
    EDGEHOG_FT_TYPE_DEVICE_TO_SERVER
} edgehog_ft_type_t;

/** @brief Stream resources provided to the application by the file transfer */
typedef struct
{
    /** @brief Pointer to the Zephyr pipe used for reading/writing the data stream. */
    struct k_pipe *pipe;
    /** @brief Pointer to the Zephyr event used for signaling and transfer synchronization. */
    struct k_event *event;
} edgehog_ft_stream_t;

/** @brief Callbacks for an Edgehog file transfer. */
typedef struct
{
    /**
     * @brief Callback invoked when a stream transfer is requested.
     * @details This function notifies the application of an incoming/outgoing transfer
     * and provides the associated dynamically allocated pipe and event.
     *
     * @param[in] name The path/name of the requested stream.
     * @param[in] type The direction of the transfer.
     * @param[in] expected_size The size of the file (0 if unknown).
     * @param[in] stream Pointer to a struct where the library provides the allocated pipe and
     * event.
     * @return true if the application accepts the transfer, false to reject it.
     */
    bool (*on_stream_transfer_start)(const char *name, edgehog_ft_type_t type, size_t expected_size,
        edgehog_ft_stream_t *stream);
} edgehog_ft_cbks_t;

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif // EDGEHOG_DEVICE_FILE_TRANSFER_H
