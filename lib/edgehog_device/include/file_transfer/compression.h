/*
 * (C) Copyright 2026, SECO Mind Srl
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef FILE_TRANSFER_COMPRESSION_H
#define FILE_TRANSFER_COMPRESSION_H

#ifdef CONFIG_EDGEHOG_DEVICE_FILE_TRANSFER_COMPRESSION

/**
 * @file file_transfer/compression.h
 * @brief LZ4 Compression context and processing functions
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <lz4frame.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Data struct for a compression context instance. */
typedef struct
{
    /** @brief LZ4 compression context. */
    LZ4F_cctx *lz4_cctx;
} file_transfer_compression_ctx_t;

/**
 * @brief Initialize the compression context.
 *
 * @param[in,out] ctx Pointer to the compression context.
 * @return 0 on success, negative value on error.
 */
int file_transfer_compression_init(file_transfer_compression_ctx_t *ctx);

/**
 * @brief Check if the compression context is initialized.
 *
 * @param[in] ctx Pointer to the compression context.
 * @return True if initialized, false if not.
 */
bool file_transfer_compression_is_initialized(const file_transfer_compression_ctx_t *ctx);

/**
 * @brief Start the compression stream and write the frame header.
 *
 * @param[in,out] ctx Pointer to the compression context.
 * @param[out] out Buffer to write the header into.
 * @param[in] out_size Maximum capacity of the destination buffer.
 * @param[out] bytes_written Number of bytes written to the destination buffer.
 * @return 0 on success, negative value on error.
 */
int file_transfer_compression_begin(
    file_transfer_compression_ctx_t *ctx, uint8_t *out, size_t out_size, size_t *bytes_written);

/**
 * @brief Compress a chunk of data.
 *
 * @param[in,out] ctx Pointer to the compression context.
 * @param[in] input Pointer to the uncompressed source data.
 * @param[in] input_size Size of the uncompressed source data.
 * @param[out] out Buffer to write the compressed data into.
 * @param[in] out_size Maximum capacity of the destination buffer.
 * @param[out] bytes_written Number of bytes written to the destination buffer.
 * @return 0 on success, negative value on error.
 */
int file_transfer_compression_update(file_transfer_compression_ctx_t *ctx, const uint8_t *input,
    size_t input_size, uint8_t *out, size_t out_size, size_t *bytes_written);

/**
 * @brief Finalize the compression stream and write the frame footer.
 *
 * @param[in,out] ctx Pointer to the compression context.
 * @param[out] out Buffer to write the footer into.
 * @param[in] out_size Maximum capacity of the destination buffer.
 * @param[out] bytes_written Number of bytes written to the destination buffer.
 * @return 0 on success, negative value on error.
 */
int file_transfer_compression_end(
    file_transfer_compression_ctx_t *ctx, uint8_t *out, size_t out_size, size_t *bytes_written);

/**
 * @brief Free the compression context.
 *
 * @param[in,out] ctx Pointer to the compression context.
 */
void file_transfer_compression_free(file_transfer_compression_ctx_t *ctx);

#ifdef __cplusplus
}
#endif

#endif

#endif /* FILE_TRANSFER_COMPRESSION_H */
