/*
 * (C) Copyright 2026, SECO Mind Srl
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "file_transfer/storage.h"
#include "log.h"

#include <stddef.h>

#include <zephyr/fs/fs.h>

EDGEHOG_LOG_MODULE_REGISTER(file_transfer_storage, CONFIG_EDGEHOG_DEVICE_FILE_TRANSFER_LOG_LEVEL);

#define MAX_FILE_NAME_LEN 255
#define FILE_PERCENTAGE 100
#define MAX_FT_FILE_SIZE (64 * 1024 * 1024)
#define PARTITION_NODE DT_NODELABEL(lfs1)

#if DT_NODE_EXISTS(PARTITION_NODE)
// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
FS_FSTAB_DECLARE_ENTRY(PARTITION_NODE);
#else /* PARTITION_NODE */
#error "Could not find the littlefs partition!"
#endif /* PARTITION_NODE */

static struct fs_mount_t *const mountpoint = &FS_FSTAB_ENTRY(PARTITION_NODE); // NOLINT

/**
 * @brief Context structure for file transfer storage.
 */
typedef struct
{
    /** The file handle. */
    struct fs_file_t file;
    /** Pointer to the file name. */
    char *file_name;
    /** Length of the file name. */
    size_t file_name_size;
    /** The total expected size of the file being transferred. */
    size_t expected_file_size;
    /** The amount of data written to the file so far. */
    size_t current_file_size;
} storage_ctx_t;

/**
 * @brief check if there is enough space in the storage.
 * @param required_bytes the size of the file to store.
 * @return true if there is enough space, false otherwise
 */
bool has_enough_space(size_t required_bytes)
{
    struct fs_statvfs stat;

    int err = fs_statvfs(mountpoint->mnt_point, &stat);
    if (err < 0) {
        EDGEHOG_LOG_ERR(
            "FT couldn't retrieve file system stats for %s: %d", mountpoint->mnt_point, err);
        return false;
    }

    uint32_t free_space_bytes = stat.f_bfree * stat.f_frsize;

    return free_space_bytes >= required_bytes;
}

static edgehog_result_t write_file_init(void **ctx, edgehog_ft_cbks_t * /*cbks*/, char *identifier,
    char * /*url*/, size_t expected_file_size, char * /*destination*/)
{
    if (!ctx) {
        return EDGEHOG_RESULT_FILE_TRANSFER_START_FAIL;
    }

    if (*ctx) {
        EDGEHOG_LOG_DBG("FT storage context already initialized");
        return EDGEHOG_RESULT_OK;
    }

    if (!has_enough_space(expected_file_size)) {
        EDGEHOG_LOG_ERR("FT not enough space left in storage.");
        return EDGEHOG_RESULT_OUT_OF_MEMORY;
    }

    *ctx = k_malloc(sizeof(storage_ctx_t));
    if (!*ctx) {
        EDGEHOG_LOG_ERR("FT failed to initialize memory for storage ctx");
        return EDGEHOG_RESULT_OUT_OF_MEMORY;
    }

    edgehog_result_t eres = EDGEHOG_RESULT_OUT_OF_MEMORY;

    storage_ctx_t *storage_ctx = (storage_ctx_t *) *ctx;

    struct fs_file_t file;
    fs_file_t_init(&file);

    storage_ctx->file = file;
    storage_ctx->expected_file_size = expected_file_size;
    storage_ctx->current_file_size = 0;
    storage_ctx->file_name_size = mountpoint->mountp_len + sizeof("/\0") + strlen(identifier);
    storage_ctx->file_name = k_malloc(storage_ctx->file_name_size);
    if (!storage_ctx->file_name) {
        EDGEHOG_LOG_ERR("FT failed to allocate memory for the file name");
        goto error;
    }
    int ret = snprintf(storage_ctx->file_name, storage_ctx->file_name_size, "%s/%s",
        mountpoint->mnt_point, identifier);
    if (ret < 0 || ret >= storage_ctx->file_name_size) {
        EDGEHOG_LOG_ERR("FT failed to create filename: %d", ret); // NOLINT
        eres = EDGEHOG_RESULT_FILE_SYSTEM_ERROR;
        goto error;
    }

    return EDGEHOG_RESULT_OK;

error:

    k_free(storage_ctx->file_name);
    k_free(*ctx);

    return eres;
}

static edgehog_result_t write_file_append_chunk(
    void *ctx, const uint8_t *chunk_data, size_t chunk_size)
{
    edgehog_result_t eres = EDGEHOG_RESULT_HTTP_REQUEST_ABORTED;

    storage_ctx_t *storage_ctx = (storage_ctx_t *) ctx;

    EDGEHOG_LOG_DBG("FT received chunk of size: %zu bytes", chunk_size);

    if (chunk_data && chunk_size > 0) {
        EDGEHOG_LOG_DBG("Chunk data as string: %.*s", (int) chunk_size, (const char *) chunk_data);

        if (storage_ctx->current_file_size + chunk_size > storage_ctx->expected_file_size) {
            EDGEHOG_LOG_WRN(
                "Potential buffer overflow, the received chunk exceed declared dimensions");
            return eres;
        }

        // NOLINTNEXTLINE(hicpp-signed-bitwise)
        fs_mode_t flags = FS_O_CREATE | FS_O_READ | FS_O_WRITE | FS_O_APPEND;
        int ret = fs_open(&storage_ctx->file, storage_ctx->file_name, flags);
        if (ret < 0) {
            EDGEHOG_LOG_ERR("FT failed to open file %s: %d", storage_ctx->file_name, ret);
            eres = EDGEHOG_RESULT_FILE_SYSTEM_ERROR;
            goto exit;
        }

        EDGEHOG_LOG_INF("FT file %s opened successfully", storage_ctx->file_name);

        ret = fs_write(&storage_ctx->file, chunk_data, chunk_size);
        if (ret < 0 || (ret == 0 && ret != chunk_size)) {
            EDGEHOG_LOG_ERR("FT failed to write to file %s: %d", storage_ctx->file_name, ret);
            eres = EDGEHOG_RESULT_FILE_SYSTEM_ERROR;
            goto exit;
        }

        EDGEHOG_LOG_DBG(
            "FT downloaded %d bytes and written to file %s", ret, storage_ctx->file_name);

        storage_ctx->current_file_size += chunk_size;
    }

    eres = EDGEHOG_RESULT_OK;

exit:

    int ret = fs_close(&storage_ctx->file);
    if (ret != 0) {
        EDGEHOG_LOG_ERR("FT failed to close file %s", storage_ctx->file_name);
    }

    return eres;
}

static void write_file_abort(void *ctx)
{
    EDGEHOG_LOG_DBG("FT aborted, cleaning up");

    if (ctx) {
        storage_ctx_t *storage_ctx = (storage_ctx_t *) ctx;
        k_free(storage_ctx->file_name);
        k_free(storage_ctx);
    }
}

static edgehog_result_t write_file_complete(void *ctx)
{
    storage_ctx_t *storage_ctx = (storage_ctx_t *) ctx;

    if (storage_ctx->expected_file_size != storage_ctx->current_file_size) {
        EDGEHOG_LOG_ERR("FT download data size differs from the expected one.");
        write_file_abort(ctx);
        return EDGEHOG_RESULT_HTTP_REQUEST_ABORTED;
    }

    EDGEHOG_LOG_DBG("Download completed successfully!");

    return EDGEHOG_RESULT_OK;
}

const edgehog_ft_file_write_cbks_t file_transfer_storage_write_cbks
    = { .file_init = write_file_init,
          .file_append_chunk = write_file_append_chunk,
          .file_complete = write_file_complete,
          .file_abort = write_file_abort };

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
