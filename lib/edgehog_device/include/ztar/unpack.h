/*
 * (C) Copyright 2026, SECO Mind Srl
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZTAR_UNPACK_H
#define ZTAR_UNPACK_H

/**
 * @file ztar/unpack.h
 * @brief TAR extraction APIs.
 * @details This parser only supports the USTAR format and is designed to work with streaming data,
 * making it suitable for processing large tar archives without needing to load the entire archive
 * into memory. It provides callbacks for file start, file data, and file end events, allowing users
 * to handle extracted files as they are processed from the stream.
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "ztar/core.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Callbacks for ztar stream parsing events. */
typedef struct
{
    /**
     * @brief Callback invoked when a new file starts in the archive.
     *
     * @param[in] header Pointer to the parsed TAR header.
     * @param[in,out] user_data User specified data.
     * @return 0 if successful, -1 on error. The stream parser will abort if an error is returned.
     */
    int (*on_file_start)(const ztar_header_t *header, void *user_data);

    /**
     * @brief Callback invoked when file data is read from the stream.
     *
     * @param[in] header Pointer to the parsed TAR header.
     * @param[in] data Pointer to the file data chunk.
     * @param[in] size Size of the file data chunk.
     * @param[in,out] user_data User specified data.
     * @return 0 if successful, -1 on error. The stream parser will abort if an error is returned.
     */
    int (*on_file_data)(
        const ztar_header_t *header, const uint8_t *data, size_t size, void *user_data);

    /**
     * @brief Callback invoked when a file ends in the archive.
     *
     * @param[in] header Pointer to the parsed TAR header.
     * @param[in,out] user_data User specified data.
     * @return 0 if successful, -1 on error. The stream parser will abort if an error is returned.
     */
    int (*on_file_end)(const ztar_header_t *header, void *user_data);
} ztar_unpack_callbacks_t;

/** @brief Internal states of the TAR stream parser. */
typedef enum
{
    /** @brief Parser is currently reading a header block. */
    ZTAR_UNPACK_STATE_HEADER = 0,
    /** @brief Parser is currently reading file data. */
    ZTAR_UNPACK_STATE_DATA,
    /** @brief Parser is currently reading file padding blocks. */
    ZTAR_UNPACK_STATE_PADDING,
    /** @brief Parser is reading the archive end trailer blocks. */
    ZTAR_UNPACK_STATE_TRAILER
} ztar_unpack_state_t;

/** @brief Data struct for a ztar stream parser instance. */
typedef struct
{
    /** @brief Indicates if the stream parser has been initialized. */
    bool initialized;
    /** @brief Current internal state of the stream parser. */
    ztar_unpack_state_t state;
    /** @brief The TAR header currently being processed. */
    ztar_header_t current_header;
    /** @brief Number of bytes processed in the current header. */
    size_t bytes_processed_in_header;
    /** @brief Number of bytes processed in the current file data. */
    size_t bytes_processed_in_file;
    /** @brief Number of bytes processed in the current file's block padding. */
    size_t bytes_processed_in_padding;
    /** @brief Number of bytes processed in the archive trailer. */
    size_t bytes_processed_in_trailer;
    /** @brief Registered callbacks for parsing events. */
    ztar_unpack_callbacks_t callbacks;
    /** @brief User data passed to callback functions. */
    void *user_data;
} ztar_unpack_t;

/**
 * @brief Initialize the ztar stream parser.
 *
 * @param[in,out] stream Pointer to the ztar stream to initialize.
 * @param[in] callbacks Struct containing parser event callbacks.
 * @param[in] user_data User specified data to pass to the callbacks.
 * @return ZTAR_RESULT_OK if successful, otherwise a ztar_result_t error code.
 */
ztar_result_t ztar_unpack_init(
    ztar_unpack_t *stream, ztar_unpack_callbacks_t callbacks, void *user_data);

/**
 * @brief Check if the ztar stream parser is initialized.
 *
 * @param[in] stream Pointer to the ztar stream.
 * @return True if initialized, false if not.
 */
bool ztar_unpack_is_initialized(const ztar_unpack_t *stream);

/**
 * @brief Process a chunk of data through the TAR stream parser.
 *
 * @param[in,out] stream Pointer to the initialized ztar stream.
 * @param[in] data Pointer to the chunk of TAR archive data.
 * @param[in] size Size of the TAR archive data chunk.
 * @return ZTAR_RESULT_OK if successful, otherwise a ztar_result_t error code.
 */
ztar_result_t ztar_unpack_process(ztar_unpack_t *stream, const uint8_t *data, size_t size);

/**
 * @brief Extract the full file name from a parsed TAR header.
 * @details In USTAR, if the prefix is populated, the full path is prefix + '/' + name.
 *
 * @param[in] header Pointer to the parsed TAR header.
 * @param[out] buffer Output buffer for the file name (must be at least 257 bytes).
 * @return ZTAR_RESULT_OK if successful, otherwise a ztar_result_t error code.
 */
ztar_result_t ztar_unpack_get_file_name(
    const ztar_header_t *header, char buffer[static ZTAR_FILE_NAME_BUFF_SIZE]);

/**
 * @brief Extract the file size from a parsed TAR header.
 *
 * @param[in] header Pointer to the parsed TAR header.
 * @param[out] file_size Output pointer for the extracted file size.
 * @return ZTAR_RESULT_OK if successful, otherwise a ztar_result_t error code.
 */
ztar_result_t ztar_unpack_get_file_size(const ztar_header_t *header, size_t *file_size);

/**
 * @brief Extract the file type from a parsed TAR header.
 * @details Restricts output strictly to regular files and directories, all other types are
 * considered unsupported.
 *
 * @param[in] header Pointer to the parsed TAR header.
 * @param[out] file_type Output pointer for the extracted file type.
 * @return ZTAR_RESULT_OK if successful, otherwise a ztar_result_t error code.
 */
ztar_result_t ztar_unpack_get_file_type(const ztar_header_t *header, ztar_filetype_t *file_type);

#ifdef __cplusplus
}
#endif

#endif // ZTAR_UNPACK_H
