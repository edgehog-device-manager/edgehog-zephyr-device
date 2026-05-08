/*
 * (C) Copyright 2026, SECO Mind Srl
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "file_transfer/compression.h"

#include "log.h"

#include <stdlib.h>
#include <string.h>

EDGEHOG_LOG_MODULE_REGISTER(compression, CONFIG_EDGEHOG_DEVICE_FILE_TRANSFER_COMPRESSION_LOG_LEVEL);

/************************************************
 *        Defines, constants and typedef        *
 ***********************************************/

// Static preferences for the LZ4 compression
static const LZ4F_preferences_t lz4_prefs = {
    .autoFlush = 1,
};

/************************************************
 *         Global functions definition          *
 ***********************************************/

int file_transfer_compression_init(file_transfer_compression_ctx_t *ctx)
{
    if (!ctx || ctx->lz4_cctx) {
        return -1;
    }
    EDGEHOG_LOG_DBG("Initializing compression context");

    size_t ret = LZ4F_createCompressionContext(&ctx->lz4_cctx, LZ4F_VERSION);
    if (LZ4F_isError(ret)) {
        EDGEHOG_LOG_ERR("Failed to create LZ4 compression context: %s", LZ4F_getErrorName(ret));
        return -1;
    }

    return 0;
}

bool file_transfer_compression_is_initialized(const file_transfer_compression_ctx_t *ctx)
{
    return ctx && ctx->lz4_cctx != NULL;
}

int file_transfer_compression_begin(
    file_transfer_compression_ctx_t *ctx, uint8_t *out, size_t out_size, size_t *bytes_written)
{
    if (!ctx || !ctx->lz4_cctx) {
        return -1;
    }

    size_t ret = LZ4F_compressBegin(ctx->lz4_cctx, out, out_size, &lz4_prefs);
    if (LZ4F_isError(ret)) {
        EDGEHOG_LOG_ERR("LZ4 compress begin failed: %s", LZ4F_getErrorName(ret));
        return -1;
    }

    *bytes_written = ret;
    return 0;
}

int file_transfer_compression_update(file_transfer_compression_ctx_t *ctx, const uint8_t *input,
    size_t input_size, uint8_t *out, size_t out_size, size_t *bytes_written)
{
    if (!ctx || !ctx->lz4_cctx) {
        return -1;
    }

    size_t ret = LZ4F_compressUpdate(ctx->lz4_cctx, out, out_size, input, input_size, NULL);
    if (LZ4F_isError(ret)) {
        EDGEHOG_LOG_ERR("LZ4 compression failed: %s", LZ4F_getErrorName(ret));
        return -1;
    }

    *bytes_written = ret;
    return 0;
}

int file_transfer_compression_end(
    file_transfer_compression_ctx_t *ctx, uint8_t *out, size_t out_size, size_t *bytes_written)
{
    if (!ctx || !ctx->lz4_cctx) {
        return -1;
    }

    size_t ret = LZ4F_compressEnd(ctx->lz4_cctx, out, out_size, NULL);
    if (LZ4F_isError(ret)) {
        EDGEHOG_LOG_ERR("LZ4 footer write failed: %s", LZ4F_getErrorName(ret));
        return -1;
    }

    *bytes_written = ret;
    return 0;
}

void file_transfer_compression_free(file_transfer_compression_ctx_t *ctx)
{
    if (ctx && ctx->lz4_cctx) {
        EDGEHOG_LOG_DBG("Freeing compression context");
        LZ4F_freeCompressionContext(ctx->lz4_cctx);
        ctx->lz4_cctx = NULL;
    }
}
