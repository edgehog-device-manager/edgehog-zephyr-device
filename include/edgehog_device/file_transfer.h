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
#define EDGEHOG_FT_STREAM_EOF_EVENT_FLAG (1U << 0U)
/** @brief Event flag indicating an acknowledgement in the file transfer stream. */
#define EDGEHOG_FT_STREAM_ACK_EVENT_FLAG (1U << 1U)
/** @brief Event flag indicating an error in the file transfer stream. */
#define EDGEHOG_FT_STREAM_ERROR_EVENT_FLAG (1U << 2U)

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

/** @brief File transfer file system permissions for a given partition. */
typedef enum
{
    /** @brief Allow reading files from this partition. */
    EDGEHOG_FT_FILESYSTEM_PERM_READ = (1U << 0U),
    /** @brief Allow writing files to this partition. */
    EDGEHOG_FT_FILESYSTEM_PERM_WRITE = (1U << 1U),
    /** @brief Allow both reading and writing. */
    EDGEHOG_FT_FILESYSTEM_PERM_RW
    = (EDGEHOG_FT_FILESYSTEM_PERM_READ | EDGEHOG_FT_FILESYSTEM_PERM_WRITE)
} edgehog_ft_filesystem_permission_t;

/** @brief Configuration for an allowed filesystem partition. */
typedef struct
{
    /**
     * @brief The mount point of the formatted partition.
     * @details E.g., "/lfs1". Edgehog will verify if the file system is mounted using Zephyr's
     * fs_stat() before operating on it.
     */
    const char *mount_point;
    /** @brief Allowed transfer operations on this partition. */
    edgehog_ft_filesystem_permission_t permissions;
} edgehog_ft_filesystem_partition_t;

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
    /**
     * @brief Callback invoked when a filesystem transfer has been performed.
     * @details This function notifies the application that a file transfer has been completed.
     *
     * @param[in] type The direction of the transfer.
     * @param[in] file_path Path of the transferred file.
     */
    void (*on_filesystem_transfer_done)(edgehog_ft_type_t type, const char *file_path);
} edgehog_ft_cbks_t;

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif // EDGEHOG_DEVICE_FILE_TRANSFER_H
