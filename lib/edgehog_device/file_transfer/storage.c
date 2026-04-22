/*
 * (C) Copyright 2026, SECO Mind Srl
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "file_transfer/storage.h"
#include "log.h"

#include <stddef.h>

EDGEHOG_LOG_MODULE_REGISTER(file_transfer_storage, CONFIG_EDGEHOG_DEVICE_FILE_TRANSFER_LOG_LEVEL);

#define DUMMY_PROGRESS 50

static edgehog_result_t dummy_write_file_init(void **ctx, edgehog_ft_cbks_t * /*cbks*/,
    char *identifier, char *url, size_t expected_file_size, char *destination)
{
    EDGEHOG_LOG_INF("FT Dummy Init - ID: %s, URL: %s, Size: %zu, Dest: %s",
        identifier ? identifier : "N/A", url ? url : "N/A", expected_file_size,
        destination ? destination : "N/A");

    // We don't have a real context to maintain, so we just set it to NULL
    if (ctx) {
        *ctx = NULL;
    }

    return EDGEHOG_RESULT_OK;
}

static edgehog_result_t dummy_write_file_append_chunk(
    void * /*ctx*/, const uint8_t *chunk_data, size_t chunk_size)
{
    EDGEHOG_LOG_INF("FT Dummy Append - Received chunk of size: %zu bytes", chunk_size);

    if (chunk_data && chunk_size > 0) {
        EDGEHOG_LOG_INF("Chunk data as string: %.*s", (int) chunk_size, (const char *) chunk_data);
    }

    return EDGEHOG_RESULT_OK;
}

static edgehog_result_t dummy_write_file_complete(void * /*ctx*/)
{
    EDGEHOG_LOG_INF("FT Dummy Complete - File transfer finalized successfully");
    return EDGEHOG_RESULT_OK;
}

static void dummy_write_file_abort(void * /*ctx*/)
{
    EDGEHOG_LOG_INF("FT Dummy Abort - File transfer aborted, cleaning up");
}

const edgehog_ft_file_write_cbks_t file_transfer_storage_write_cbks
    = { .file_init = dummy_write_file_init,
          .file_append_chunk = dummy_write_file_append_chunk,
          .file_complete = dummy_write_file_complete,
          .file_abort = dummy_write_file_abort };

// Define a 100-character block of dummy text
#define DUMMY_BLOCK                                                                                \
    "This is a block of dummy text designed to make the file larger. It contains 100 characters "  \
    "exactly! "
#define DUMMY_BLOCK_X10                                                                            \
    DUMMY_BLOCK DUMMY_BLOCK DUMMY_BLOCK DUMMY_BLOCK DUMMY_BLOCK DUMMY_BLOCK DUMMY_BLOCK            \
        DUMMY_BLOCK DUMMY_BLOCK DUMMY_BLOCK
#define DUMMY_BLOCK_X40 DUMMY_BLOCK_X10 DUMMY_BLOCK_X10 DUMMY_BLOCK_X10 DUMMY_BLOCK_X10

static const char dummy_read_file_content[]
    = DUMMY_BLOCK_X40 "End of the 4000+ bytes dummy file transfer.\n";

#define MAX_CHUNK_SIZE 500
#define FULL_DOWNLOAD_PERCENTAGE 100

/** @brief Dummy context */
typedef struct
{
    /** @brief Dummy */
    size_t offset;
    /** @brief Dummy */
    size_t total_size;
} dummy_read_ctx_t;

static edgehog_result_t dummy_read_file_init(
    void **ctx, edgehog_ft_cbks_t * /*cbks*/, char *identifier, char *source)
{
    EDGEHOG_LOG_INF("FT Dummy Read Init - ID: %s, Source: %s", identifier ? identifier : "N/A",
        source ? source : "N/A");

    dummy_read_ctx_t *read_ctx = k_malloc(sizeof(dummy_read_ctx_t));
    if (!read_ctx) {
        EDGEHOG_LOG_ERR("Failed to allocate read context");
        return EDGEHOG_RESULT_INTERNAL_ERROR;
    }

    read_ctx->offset = 0;
    read_ctx->total_size = sizeof(dummy_read_file_content) - 1;

    if (ctx) {
        *ctx = read_ctx;
    }

    EDGEHOG_LOG_INF("FT Dummy Read - Total size: %zu bytes", read_ctx->total_size);
    return EDGEHOG_RESULT_OK;
}

static edgehog_result_t dummy_read_file_read_chunk(
    void *ctx, uint8_t **chunk_data, size_t *chunk_size, bool *last_chunk)
{
    if (!ctx || !chunk_data || !chunk_size) {
        return EDGEHOG_RESULT_INVALID_PARAM;
    }

    dummy_read_ctx_t *read_ctx = (dummy_read_ctx_t *) ctx;
    size_t remaining_bytes = read_ctx->total_size - read_ctx->offset;
    size_t current_chunk_size
        = (remaining_bytes > MAX_CHUNK_SIZE) ? MAX_CHUNK_SIZE : remaining_bytes;

    EDGEHOG_LOG_INF(
        "FT Dummy Read Chunk - Offset: %zu, Size: %zu bytes", read_ctx->offset, current_chunk_size);

    // Allocate a heap buffer instead of returning a pointer to read-only flash memory.
    // This prevents a possible crash when the HTTP/Network stack accesses the buffer.
    uint8_t *allocated_chunk = k_malloc(current_chunk_size);
    if (!allocated_chunk) {
        EDGEHOG_LOG_ERR("Failed to allocate memory for dummy read chunk");
        return EDGEHOG_RESULT_OUT_OF_MEMORY;
    }

    // Copy the data from the static constant string to the new mutable heap buffer
    memcpy(allocated_chunk, dummy_read_file_content + read_ctx->offset, current_chunk_size);

    *chunk_data = allocated_chunk;
    *chunk_size = current_chunk_size;

    read_ctx->offset += current_chunk_size;

    if (last_chunk) {
        *last_chunk = (read_ctx->offset >= read_ctx->total_size);
    }

    return EDGEHOG_RESULT_OK;
}

static edgehog_result_t dummy_read_file_complete(void *ctx)
{
    EDGEHOG_LOG_INF("FT Dummy Read Complete - Transfer finished, cleaning up context");
    if (ctx) {
        k_free(ctx);
    }
    return EDGEHOG_RESULT_OK;
}

static void dummy_read_file_abort(void *ctx)
{
    EDGEHOG_LOG_INF("FT Dummy Read Abort - Transfer failed/aborted, cleaning up context");
    if (ctx) {
        k_free(ctx);
    }
}

const edgehog_ft_file_read_cbks_t file_transfer_storage_read_cbks
    = { .file_init = dummy_read_file_init,
          .file_read_chunk = dummy_read_file_read_chunk,
          .file_complete = dummy_read_file_complete,
          .file_abort = dummy_read_file_abort };
