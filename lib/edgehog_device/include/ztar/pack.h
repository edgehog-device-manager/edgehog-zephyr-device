/*
 * (C) Copyright 2026, SECO Mind Srl
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZTAR_PACK_H
#define ZTAR_PACK_H

/**
 * @file ztar/pack.h
 * @brief TAR packing APIs.
 * @details This serializer only supports the USTAR format and is designed to work with streaming
 * data, making it suitable for processing large TAR archives without needing to load the entire
 * archive into memory. It provides callbacks for getting files to pack into a TAR archive.
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "ztar/core.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Internal states of the TAR stream packer. */
typedef enum
{
    /** @brief Ready to process the next file. */
    ZTAR_PACK_STATE_NEXT_FILE = 0,
    /** @brief Writing the TAR header for the current file. */
    ZTAR_PACK_STATE_HEADER,
    /** @brief Writing the file data. */
    ZTAR_PACK_STATE_DATA,
    /** @brief Writing the padding after the file data. */
    ZTAR_PACK_STATE_PADDING,
    /** @brief Writing the TAR archive trailer (end of archive). */
    ZTAR_PACK_STATE_TRAILER,
    /** @brief Packing is complete. */
    ZTAR_PACK_STATE_DONE
} ztar_pack_state_t;

/** @brief Callbacks for ztar stream packing events. */
typedef struct
{
    /**
     * @brief Called when the packer needs the next file's metadata.
     * @param[out] has_next Pointer to a boolean indicating if there are more files.
     * @param[out] name Buffer to copy the file name into.
     * @param[out] size Pointer to write the file size into.
     * @param[in] user_data User specified data.
     * @return 0 on success, -1 on error.
     */
    int (*get_next_file)(
        bool *has_next, char name[static ZTAR_FILE_NAME_BUFF_SIZE], size_t *size, void *user_data);

    /**
     * @brief Called when the packer needs raw file data.
     * @param[out] buffer Destination buffer for the file data.
     * @param[in] max_size Maximum number of bytes to read.
     * @param[in] user_data User specified data.
     * @param[out] bytes_read Pointer to write the actual number of bytes read into.
     * @return 0 on success, -1 on error.
     */
    int (*read_file_data)(uint8_t *buffer, size_t max_size, void *user_data, size_t *bytes_read);
} ztar_pack_callbacks_t;

/** @brief Data struct for a ztar stream packer instance. */
typedef struct
{
    /** @brief Indicates if the stream packer has been initialized. */
    bool initialized;
    /** @brief Current state of the packer. */
    ztar_pack_state_t state;
    /** @brief Name of the file currently being processed. */
    char current_file_name[ZTAR_FILE_NAME_BUFF_SIZE];
    /** @brief Expected size of the file currently being packed. */
    size_t expected_file_size;
    /** @brief Number of bytes written for the current file data so far. */
    size_t bytes_written_for_file;
    /** @brief Number of padding bytes processed so far. */
    size_t bytes_processed_in_padding;
    /** @brief Number of trailer bytes processed so far. */
    size_t bytes_processed_in_trailer;
    /** @brief User-provided callbacks for file metadata and data. */
    ztar_pack_callbacks_t callbacks;
    /** @brief User-provided context data. */
    void *user_data;
} ztar_pack_t;

/**
 * @brief Initializes the TAR stream packer.
 * @param[out] stream The stream instance to initialize.
 * @param[in] callbacks The callbacks to use for retrieving file data.
 * @param[in] user_data User context data passed to callbacks.
 * @return ZTAR_SUCCESS on success, or an appropriate ztar_result_t error code.
 */
ztar_result_t ztar_pack_init(ztar_pack_t *stream, ztar_pack_callbacks_t callbacks, void *user_data);

/**
 * @brief Checks if the TAR packer stream has been initialized.
 * @param[in] stream The stream instance to check.
 * @return true if initialized, false otherwise.
 */
bool ztar_pack_is_initialized(const ztar_pack_t *stream);

/**
 * @brief Reads a chunk of the packed TAR stream into a buffer.
 * @param[in,out] stream The stream instance.
 * @param[out] out_buffer The buffer to write the packed TAR data into.
 * @param[in] out_size The maximum size of the output buffer.
 * @param[out] bytes_written Pointer to store the actual number of bytes written to the buffer.
 * @return ZTAR_SUCCESS on success, or an appropriate ztar_result_t error code.
 */
ztar_result_t ztar_pack_read_stream(
    ztar_pack_t *stream, uint8_t *out_buffer, size_t out_size, size_t *bytes_written);

#ifdef __cplusplus
}
#endif

#endif // ZTAR_PACK_H
