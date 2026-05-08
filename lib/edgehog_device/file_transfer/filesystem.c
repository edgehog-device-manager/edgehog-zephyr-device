/*
 * (C) Copyright 2026, SECO Mind Srl
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "file_transfer/filesystem.h"

#include "edgehog_device/file_transfer.h"

#include <stdlib.h>
#include <string.h>
#include <zephyr/fs/fs.h>
#include <zephyr/kernel.h>

#include "file_transfer/core.h"
#include "log.h"

EDGEHOG_LOG_MODULE_REGISTER(
    file_transfer_filesystem, CONFIG_EDGEHOG_DEVICE_FILE_TRANSFER_LOG_LEVEL);

/************************************************
 *        Defines, constants and typedef        *
 ***********************************************/

#define FS_READ_BUFFER_SIZE 4096

/** @brief Context structure for write operations. */
typedef struct
{
    /** @brief Zephyr filesystem file object used for writing. */
    struct fs_file_t file;
    /** @brief Path to the destination file on the filesystem. */
    const char *path;
    /** @brief Pointer to the file transfer callback structure. */
    edgehog_ft_cbks_t *cbks;
} write_ctx_t;

/** @brief Context structure for read operations. */
typedef struct
{
    /** @brief Zephyr filesystem file object used for reading. */
    struct fs_file_t file;
    /** @brief Path to the source file on the filesystem. */
    const char *path;
    /** @brief Pointer to the file transfer callback structure. */
    edgehog_ft_cbks_t *cbks;
    /** @brief Buffer used to hold the data chunks read from the file. */
    uint8_t buffer[FS_READ_BUFFER_SIZE];
} read_ctx_t;

/************************************************
 *         Static functions declarations        *
 ***********************************************/

static edgehog_result_t write_init(void **ctx, edgehog_ft_cbks_t *cbks, char *identifier, char *url,
    size_t expected_file_size, char *destination);
static edgehog_result_t write_append(void *ctx, const uint8_t *chunk_data, size_t chunk_size);
static edgehog_result_t write_complete(void *ctx);
static void write_abort(void *ctx);

static edgehog_result_t read_init(
    void **ctx, edgehog_ft_cbks_t *cbks, char *identifier, char *source);
static edgehog_result_t read_chunk(
    void *ctx, size_t max_length, uint8_t **chunk_data, size_t *chunk_size, bool *last_chunk);
static edgehog_result_t read_complete(void *ctx);
static void read_abort(void *ctx);

static bool is_valid_file_path(const char *path);
static bool is_valid_partition(edgehog_ft_cbks_t *cbks, const char *path,
    edgehog_ft_filesystem_permission_t req_perm, size_t expected_file_size);

/************************************************
 *         Global variables definitions         *
 ***********************************************/

const edgehog_ft_file_write_cbks_t edgehog_ft_filesystem_write_cbks = { .file_init = write_init,
    .file_append_chunk = write_append,
    .file_complete = write_complete,
    .file_abort = write_abort };
const edgehog_ft_file_read_cbks_t edgehog_ft_filesystem_read_cbks = { .file_init = read_init,
    .file_read_chunk = read_chunk,
    .file_complete = read_complete,
    .file_abort = read_abort };

/************************************************
 *         Static functions definitions         *
 ***********************************************/

static edgehog_result_t write_init(void **ctx, edgehog_ft_cbks_t *cbks, char * /*identifier*/,
    char * /*url*/, size_t expected_file_size, char *destination)
{
    edgehog_result_t eres = EDGEHOG_RESULT_OK;
    write_ctx_t *wctx = NULL;

    if ((destination == NULL) || (strlen(destination) == 0)) {
        EDGEHOG_LOG_ERR("Missing filesystem path in destination.");
        eres = EDGEHOG_RESULT_INVALID_CONFIGURATION;
        goto error;
    }

    if (!is_valid_partition(
            cbks, destination, EDGEHOG_FT_FILESYSTEM_PERM_WRITE, expected_file_size)) {
        EDGEHOG_LOG_ERR("Permission denied or partition unavailable for write to %s", destination);
        eres = EDGEHOG_RESULT_INTERNAL_ERROR;
        goto error;
    }

    wctx = k_malloc(sizeof(write_ctx_t));
    if (!wctx) {
        EDGEHOG_LOG_ERR("Out of memory %s: %d", __FILE__, __LINE__);
        eres = EDGEHOG_RESULT_OUT_OF_MEMORY;
        goto error;
    }

    fs_file_t_init(&wctx->file);
    // NOLINTNEXTLINE (hicpp-signed-bitwise)
    int res = fs_open(&wctx->file, destination, FS_O_CREATE | FS_O_WRITE);
    if (res != 0) {
        EDGEHOG_LOG_ERR("Failed to open file for writing %s, err %d", destination, res);
        eres = EDGEHOG_RESULT_INTERNAL_ERROR;
        goto error;
    }
    wctx->cbks = cbks;
    wctx->path = destination;

    *ctx = wctx;
    return EDGEHOG_RESULT_OK;

error:
    k_free(wctx);
    return eres;
}

static edgehog_result_t write_append(void *ctx, const uint8_t *chunk_data, size_t chunk_size)
{
    write_ctx_t *wctx = (write_ctx_t *) ctx;
    size_t total_written = 0;

    while (total_written < chunk_size) {
        ssize_t res = fs_write(&wctx->file, chunk_data + total_written, chunk_size - total_written);
        if (res < 0) {
            EDGEHOG_LOG_ERR("Failed to append chunk to file, err %zd", res);
            return EDGEHOG_RESULT_INTERNAL_ERROR;
        }
        if (res == 0) {
            EDGEHOG_LOG_ERR("Failed to append chunk to file: wrote 0 bytes");
            return EDGEHOG_RESULT_INTERNAL_ERROR;
        }
        total_written += (size_t) res;
    }

    return EDGEHOG_RESULT_OK;
}

static edgehog_result_t write_complete(void *ctx)
{
    write_ctx_t *wctx = (write_ctx_t *) ctx;
    fs_close(&wctx->file);

    EDGEHOG_LOG_DBG("File write has been completed.");

    if (wctx->cbks && wctx->cbks->on_filesystem_transfer_done) {
        wctx->cbks->on_filesystem_transfer_done(EDGEHOG_FT_TYPE_SERVER_TO_DEVICE, wctx->path);
    }

    k_free(ctx);
    return EDGEHOG_RESULT_OK;
}

static void write_abort(void *ctx)
{
    write_ctx_t *wctx = (write_ctx_t *) ctx;

    fs_close(&wctx->file);
    fs_unlink(wctx->path);

    EDGEHOG_LOG_DBG("File write has been aborted.");

    k_free(ctx);
}

static edgehog_result_t read_init(
    void **ctx, edgehog_ft_cbks_t *cbks, char * /*identifier*/, char *source)
{
    edgehog_result_t eres = EDGEHOG_RESULT_OK;
    read_ctx_t *rctx = NULL;

    if ((source == NULL) || (strlen(source) == 0)) {
        EDGEHOG_LOG_ERR("Missing filesystem path in source.");
        eres = EDGEHOG_RESULT_INVALID_CONFIGURATION;
        goto error;
    }

    if (!is_valid_partition(cbks, source, EDGEHOG_FT_FILESYSTEM_PERM_READ, 0)) {
        EDGEHOG_LOG_ERR("Permission denied or partition unavailable for read from %s", source);
        eres = EDGEHOG_RESULT_INTERNAL_ERROR;
        goto error;
    }

    rctx = k_malloc(sizeof(read_ctx_t));
    if (!rctx) {
        EDGEHOG_LOG_ERR("Out of memory %s: %d", __FILE__, __LINE__);
        eres = EDGEHOG_RESULT_OUT_OF_MEMORY;
        goto error;
    }

    fs_file_t_init(&rctx->file);
    int res = fs_open(&rctx->file, source, FS_O_READ);
    if (res != 0) {
        EDGEHOG_LOG_ERR("Failed to open file for reading %s, err %d", source, res);
        eres = EDGEHOG_RESULT_INTERNAL_ERROR;
        goto error;
    }
    rctx->cbks = cbks;
    rctx->path = source;

    *ctx = rctx;
    return EDGEHOG_RESULT_OK;

error:
    k_free(rctx);
    return eres;
}

static edgehog_result_t read_chunk(
    void *ctx, size_t max_length, uint8_t **chunk_data, size_t *chunk_size, bool *last_chunk)
{
    read_ctx_t *rctx = (read_ctx_t *) ctx;

    size_t bytes_to_read = MIN(FS_READ_BUFFER_SIZE, max_length);
    ssize_t res = fs_read(&rctx->file, rctx->buffer, bytes_to_read);
    if (res < 0) {
        EDGEHOG_LOG_ERR("Failed to read chunk from file, err %zd", res);
        return EDGEHOG_RESULT_INTERNAL_ERROR;
    }

    *chunk_data = rctx->buffer;
    *chunk_size = (size_t) res;
    *last_chunk = (res < bytes_to_read);

    return EDGEHOG_RESULT_OK;
}

static edgehog_result_t read_complete(void *ctx)
{
    read_ctx_t *rctx = (read_ctx_t *) ctx;
    fs_close(&rctx->file);
    EDGEHOG_LOG_DBG("File read has been completed, freeing context.");

    if (rctx->cbks && rctx->cbks->on_filesystem_transfer_done) {
        rctx->cbks->on_filesystem_transfer_done(EDGEHOG_FT_TYPE_DEVICE_TO_SERVER, rctx->path);
    }

    k_free(ctx);
    return EDGEHOG_RESULT_OK;
}

static void read_abort(void *ctx)
{
    read_ctx_t *rctx = (read_ctx_t *) ctx;
    EDGEHOG_LOG_DBG("File read has been aborted.");

    fs_close(&rctx->file);

    k_free(ctx);
}

static bool is_valid_partition(edgehog_ft_cbks_t *cbks, const char *path,
    edgehog_ft_filesystem_permission_t req_perm, size_t expected_file_size)
{
    if (!cbks) {
        EDGEHOG_LOG_ERR("Missing file transfer callbacks.");
        return false;
    }

    if (!is_valid_file_path(path)) {
        EDGEHOG_LOG_ERR("Invalid file name or path structure: %s", path);
        return false;
    }

    edgehog_ft_t *file_transfer = CONTAINER_OF(cbks, edgehog_ft_t, cbks);
    if (!file_transfer->partitions || file_transfer->partitions_len == 0) {
        return false;
    }

    for (size_t i = 0; i < file_transfer->partitions_len; i++) {
        const char *mount_point = file_transfer->partitions[i].mount_point;
        size_t mount_point_len = strlen(mount_point);

        // Mount point should be shorter than path
        if (mount_point_len > strlen(path)) {
            continue;
        }
        // Check if the file path starts with the mount point
        if (strncmp(path, mount_point, mount_point_len) != 0) {
            continue;
        }
        // Check if the file path is not just a prefix match
        if (path[mount_point_len] != '/' && path[mount_point_len] != '\0') {
            continue;
        }
        // Check if the required permissions are granted for this partition
        if ((file_transfer->partitions[i].permissions & req_perm) != req_perm) {
            continue;
        }
        // Determine if mount point is actively mounted and valid
        struct fs_statvfs stat;
        if (fs_statvfs(mount_point, &stat) != 0) {
            continue;
        }
        // Check if the file fits in the partition
        if (expected_file_size > 0) {
            size_t free_space = stat.f_bfree * stat.f_frsize;
            if (free_space < expected_file_size) {
                EDGEHOG_LOG_ERR("Insufficient free space on partition for %s (need %zu, have %zu)",
                    path, expected_file_size, free_space);
                continue;
            }
        }
        return true;
    }

    return false;
}

static bool is_valid_file_path(const char *path)
{
    if (!path || path[0] == '\0') {
        return false;
    }

    size_t len = strlen(path);

    // Reject paths ending in a directory separator
    if (path[len - 1] == '/') {
        return false;
    }

    // Reject consecutive slashes
    if (strstr(path, "//") != NULL) {
        return false;
    }

    // Reject directory traversal attempts
    // Checks for "/../" in the middle, "../" at the start, or "/.." at the very end
    if (strstr(path, "/../") != NULL || strncmp(path, "../", 3) == 0
        || (len >= 3 && strcmp(path + len - 3, "/..") == 0)) {
        return false;
    }

    return true;
}
