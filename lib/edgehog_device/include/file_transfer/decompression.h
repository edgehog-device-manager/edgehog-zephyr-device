/*
 * (C) Copyright 2026, SECO Mind Srl
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef FILE_TRANSFER_DECOMPRESSION_H
#define FILE_TRANSFER_DECOMPRESSION_H

#ifdef CONFIG_EDGEHOG_DEVICE_FILE_TRANSFER_COMPRESSION

/**
 * @file file_transfer/decompression.h
 * @brief LZ4 Decompression context and processing functions
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <lz4frame.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @typedef file_transfer_decompression_write_data_cbk_t
 * @brief Callback used when a chunk of decompressed data is ready to be written.
 *
 * @param[in] data Pointer to the decompressed data chunk.
 * @param[in] size Size of the decompressed data chunk.
 * @param[in,out] user_data User specified data passed during initialization.
 *
 * @return 0 if successful, otherwise a negative error code.
 */
typedef int (*file_transfer_decompression_write_data_cbk_t)(
    const uint8_t *data, size_t size, void *user_data);

/** @brief Data struct for a decompression context instance. */
typedef struct
{
    /** @brief LZ4 decompression context. */
    LZ4F_dctx *lz4_dctx;
    /** @brief Buffer used for intermediate decompressed data. */
    uint8_t *decomp_buf;
    /** @brief Callback for writing decompressed data. */
    file_transfer_decompression_write_data_cbk_t write_data_cbk;
    /** @brief User data passed to write_data_cbk callback function. */
    void *user_data;
} file_transfer_decompression_ctx_t;

/**
 * @brief Initialize the decompression context.
 * @note Calling this function on an already initialized context will return an error.
 *
 * @param[in,out] ctx Pointer to the decompression context.
 * @param[in] write_data_cbk Callback to execute when data is decompressed.
 * @param[in] user_data User specified data to pass to the callback.
 * @return 0 on success, negative value on error.
 */
int file_transfer_decompression_init(file_transfer_decompression_ctx_t *ctx,
    file_transfer_decompression_write_data_cbk_t write_data_cbk, void *user_data);

/**
 * @brief Check if the decompression context is initialized.
 *
 * @param[in] ctx Pointer to the decompression context.
 * @return True if initialized, false if not.
 */
bool file_transfer_decompression_is_initialized(const file_transfer_decompression_ctx_t *ctx);

/**
 * @brief Decompress a chunk of data and pass it to the write callback.
 *
 * @param[in,out] ctx Pointer to the decompression context.
 * @param[in] src Pointer to the compressed source data.
 * @param[in] src_size Size of the compressed source data.
 * @return 0 on success, negative value on error.
 */
int file_transfer_decompression_process_chunk(
    file_transfer_decompression_ctx_t *ctx, const uint8_t *src, size_t src_size);

/**
 * @brief Free the decompression context.
 *
 * @param[in,out] ctx Pointer to the decompression context.
 */
void file_transfer_decompression_free(file_transfer_decompression_ctx_t *ctx);

#ifdef __cplusplus
}
#endif

#endif

#endif /* FILE_TRANSFER_DECOMPRESSION_H */
