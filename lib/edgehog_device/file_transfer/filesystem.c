/*
 * (C) Copyright 2026, SECO Mind Srl
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "file_transfer/filesystem.h"

#include "edgehog_device/file_transfer.h"
#include "file_transfer/filesystem_utils.h"

#include <limits.h>
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

/* Maximum directory depth for packing directories into TAR files */
#define MAX_TAR_DIR_DEPTH 10
/* Maximum path length, necessary to avoid large memory allocations from very long paths. */
/* Includes the NULL terminator */
#define MAX_PATH_SIZE 256
/* Buffer for reading chunks of a file */
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
    /** @brief Tracks if the destination is a TAR directory. */
    bool is_tar;
    /** @brief Tracks if a file is currently open inside the context. */
    bool file_open;
} write_ctx_t;

/** @brief Context structure for read operations. */
typedef struct
{
    /** @brief Zephyr filesystem file object used for reading. */
    struct fs_file_t file;
    /** @brief Tracks if the source is a directory. */
    bool is_dir;
    /** @brief List of directories, used to traverse directories. */
    struct fs_dir_t tar_dirs[MAX_TAR_DIR_DEPTH];
    /** @brief List of lengths of directories, used to traverse directories. */
    size_t tar_path_lens[MAX_TAR_DIR_DEPTH];
    /** @brief Current depth of the TAR directory traversal. */
    int tar_dir_depth;
    /** @brief Current path pointing to the deepest directory while traversing. */
    char tar_current_path[MAX_PATH_SIZE];
    /** @brief Tracks if a file is currently open inside the context. */
    bool file_open;
    /** @brief Path to the source file on the filesystem. */
    const char *path;
    /** @brief Pointer to the file transfer callback structure. */
    edgehog_ft_cbks_t *cbks;
    /** @brief Buffer used to hold the data chunks read from the file. */
    uint8_t buffer[FS_READ_BUFFER_SIZE];
} read_ctx_t;

typedef void (*fs_walk_cb_t)(const char *path, struct fs_dirent *entry, void *user_data);

/************************************************
 *         Static functions declarations        *
 ***********************************************/

static edgehog_result_t write_init(
    void **ctx, edgehog_ft_cbks_t *cbks, size_t expected_file_size, char *destination, bool is_tar);
static edgehog_result_t write_append_next_entry(void *ctx, const char *file_name);
static edgehog_result_t write_append(void *ctx, const uint8_t *chunk_data, size_t chunk_size);
static edgehog_result_t write_complete(void *ctx);
static void write_abort(void *ctx);

static edgehog_result_t read_init(
    void **ctx, edgehog_ft_cbks_t *cbks, char *source, size_t *out_file_size, bool is_tar);
static edgehog_result_t read_get_next_entry(
    void *ctx, char *file_name, size_t file_name_size, size_t *file_size, bool *has_next);
static edgehog_result_t read_chunk(
    void *ctx, size_t max_length, uint8_t **chunk_data, size_t *chunk_size, bool *last_chunk);
static edgehog_result_t read_complete(void *ctx);
static void read_abort(void *ctx);

static void tar_ascend_directory(read_ctx_t *rctx);
static edgehog_result_t tar_build_entry_path(
    read_ctx_t *rctx, const char *entry_name, size_t *out_base_len);
static edgehog_result_t tar_handle_dir_entry(read_ctx_t *rctx, size_t base_len);
static edgehog_result_t tar_handle_file_entry(
    read_ctx_t *rctx, char *file_name, size_t file_name_size, size_t base_len);

/************************************************
 *     Callbacks definition and declaration     *
 ***********************************************/

void delete_files_callback(const char *path, struct fs_dirent *entry, void *user_data)
{
    ARG_UNUSED(entry);
    ARG_UNUSED(user_data);
    fs_unlink(path);
}

/************************************************
 *         Global variables definitions         *
 ***********************************************/

const edgehog_ft_file_write_cbks_t edgehog_ft_filesystem_write_cbks = { .file_init = write_init,
    .file_append_next_entry = write_append_next_entry,
    .file_append_chunk = write_append,
    .file_complete = write_complete,
    .file_abort = write_abort };
const edgehog_ft_file_read_cbks_t edgehog_ft_filesystem_read_cbks = { .file_init = read_init,
    .file_get_next_entry = read_get_next_entry,
    .file_read_chunk = read_chunk,
    .file_complete = read_complete,
    .file_abort = read_abort };

/************************************************
 *         Static functions definitions         *
 ***********************************************/

static edgehog_result_t write_init(
    void **ctx, edgehog_ft_cbks_t *cbks, size_t expected_file_size, char *destination, bool is_tar)
{
    edgehog_result_t eres = EDGEHOG_RESULT_OK;
    write_ctx_t *wctx = NULL;

    if (!is_valid_destination(destination, is_tar)) {
        EDGEHOG_LOG_ERR("Invalid destination path: %s", destination);
        eres = EDGEHOG_RESULT_INVALID_PARAM;
        goto error;
    }

    if (!is_valid_partition(
            cbks, destination, EDGEHOG_FT_FILESYSTEM_PERM_WRITE, &expected_file_size)) {
        EDGEHOG_LOG_ERR("Permission denied or partition unavailable for write to %s", destination);
        eres = EDGEHOG_RESULT_INVALID_PARAM;
        goto error;
    }

    wctx = k_malloc(sizeof(write_ctx_t));
    if (!wctx) {
        EDGEHOG_LOG_ERR("Out of memory %s: %d", __FILE__, __LINE__);
        eres = EDGEHOG_RESULT_OUT_OF_MEMORY;
        goto error;
    }

    fs_file_t_init(&wctx->file);
    wctx->cbks = cbks;
    wctx->path = destination;
    wctx->is_tar = is_tar;
    wctx->file_open = false;

    // Only open the file immediately if we are NOT extracting a TAR.
    // If it is a TAR, the files will be opened in write_append_next_entry.
    if (!is_tar) {
        // NOLINTNEXTLINE (hicpp-signed-bitwise)
        int res = fs_open(&wctx->file, destination, FS_O_CREATE | FS_O_WRITE);
        if (res != 0) {
            EDGEHOG_LOG_ERR("Failed to open file for writing %s, err %d", destination, res);
            eres = EDGEHOG_RESULT_INTERNAL_ERROR;
            goto error;
        }
        wctx->file_open = true;
    }

    EDGEHOG_LOG_DBG("Initialized a file transfer file system write context.");
    EDGEHOG_LOG_DBG("Is tar: %d, destination: %s", is_tar, destination);

    *ctx = wctx;
    return EDGEHOG_RESULT_OK;

error:
    k_free(wctx);
    return eres;
}

static edgehog_result_t write_append_next_entry(void *ctx, const char *file_name)
{
    edgehog_result_t eres = EDGEHOG_RESULT_OK;
    char *full_path = NULL;
    write_ctx_t *wctx = (write_ctx_t *) ctx;

    if (!wctx->is_tar) {
        EDGEHOG_LOG_ERR("Attempted to append next file entry but not in TAR mode");
        eres = EDGEHOG_RESULT_INVALID_CONFIGURATION;
        goto exit;
    }

    // Close the previous file handled during the last TAR iteration
    if (wctx->file_open) {
        fs_close(&wctx->file);
        wctx->file_open = false;
    }

    size_t full_path_size = strlen(wctx->path) + strlen(file_name) + 2;
    if (full_path_size > MAX_PATH_SIZE) {
        EDGEHOG_LOG_ERR("Combined file path is too long.");
        eres = EDGEHOG_RESULT_INTERNAL_ERROR;
        goto exit;
    }

    full_path = k_calloc(full_path_size, sizeof(char));
    if (!full_path) {
        EDGEHOG_LOG_ERR("Out of memory %s: %d", __FILE__, __LINE__);
        eres = EDGEHOG_RESULT_OUT_OF_MEMORY;
        goto exit;
    }

    int ret = snprintf(full_path, full_path_size, "%s/%s", wctx->path, file_name);
    if (ret < 0 || ret >= full_path_size) {
        EDGEHOG_LOG_ERR("Failed to make full path for file %s", file_name);
        eres = EDGEHOG_RESULT_INTERNAL_ERROR;
        goto exit;
    }

    // Check if the file path is valid and does not escape the destination directory
    if (!is_valid_relative_file(wctx->path, file_name, full_path)) {
        EDGEHOG_LOG_ERR("The full path as an invalid destination: %s", full_path);
        eres = EDGEHOG_RESULT_INTERNAL_ERROR;
        goto exit;
    }

    // NOLINTNEXTLINE (hicpp-signed-bitwise)
    int res = fs_open(&wctx->file, full_path, FS_O_CREATE | FS_O_WRITE);
    if (res != 0) {
        EDGEHOG_LOG_ERR("Failed to open file for writing in TAR archive, err %d", res);
        eres = EDGEHOG_RESULT_INTERNAL_ERROR;
        goto exit;
    }
    wctx->file_open = true;

    EDGEHOG_LOG_DBG("Appended a new entry to a TAR destionation.");
    EDGEHOG_LOG_DBG("Base path: %s, full file path: %s", wctx->path, full_path);

exit:
    k_free(full_path);
    return eres;
}

static edgehog_result_t write_append(void *ctx, const uint8_t *chunk_data, size_t chunk_size)
{
    write_ctx_t *wctx = (write_ctx_t *) ctx;
    size_t total_written = 0;

    if (!wctx->file_open) {
        EDGEHOG_LOG_ERR("Attempted to write chunk but no file is open");
        return EDGEHOG_RESULT_INTERNAL_ERROR;
    }

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

    if (!wctx) {
        return EDGEHOG_RESULT_OK;
    }

    if (wctx->file_open) {
        fs_close(&wctx->file);
        wctx->file_open = false;
    }

    EDGEHOG_LOG_DBG("File write has been completed.");

    if (wctx->cbks && wctx->cbks->on_filesystem_transfer_done) {
        wctx->cbks->on_filesystem_transfer_done(EDGEHOG_FT_TYPE_SERVER_TO_DEVICE, wctx->path);
    }

    k_free(ctx);
    return EDGEHOG_RESULT_OK;
}

static void write_abort(void *ctx)
{
    int ret = 0;
    write_ctx_t *wctx = (write_ctx_t *) ctx;

    if (!wctx) {
        return;
    }

    // Close any open file
    if (wctx->file_open) {
        fs_close(&wctx->file);
        wctx->file_open = false;
    }

    // For TAR traverse the destination directory and delete each file/dir
    if (wctx->is_tar) {
        ret = fs_walk(wctx->path, delete_files_callback, NULL);
        if (ret < 0) {
            EDGEHOG_LOG_ERR("File system delete-walk failed with error: %d.", ret);
        }
    }

    // Now delete the TAR root or the only file if not in a TAR transfer
    ret = fs_unlink(wctx->path);
    if (ret < 0) {
        EDGEHOG_LOG_ERR("File system delete failed with error: %d.", ret);
    }

    EDGEHOG_LOG_ERR("File write has been aborted.");

    k_free(ctx);
}

static edgehog_result_t read_init(
    void **ctx, edgehog_ft_cbks_t *cbks, char *source, size_t *out_file_size, bool is_tar)
{
    edgehog_result_t eres = EDGEHOG_RESULT_OK;
    read_ctx_t *rctx = NULL;

    if (!is_valid_source(source, is_tar)) {
        EDGEHOG_LOG_ERR("Invalid source path: %s", source);
        eres = EDGEHOG_RESULT_INVALID_PARAM;
        goto error;
    }

    if (!is_valid_partition(cbks, source, EDGEHOG_FT_FILESYSTEM_PERM_READ, NULL)) {
        EDGEHOG_LOG_ERR("Permission denied or partition unavailable for read from %s", source);
        eres = EDGEHOG_RESULT_INTERNAL_ERROR;
        goto error;
    }

    rctx = k_malloc(sizeof(read_ctx_t));
    if (!rctx) {
        EDGEHOG_LOG_ERR("Out of memory %s: %d", __FILE__, __LINE__);
        return EDGEHOG_RESULT_OUT_OF_MEMORY;
    }

    fs_file_t_init(&rctx->file);
    rctx->cbks = cbks;
    rctx->path = source;
    rctx->file_open = false;

    if (is_tar) {
        rctx->is_dir = true;
        if (out_file_size) {
            *out_file_size = calculate_tar_directory_size(source);
        }

        rctx->tar_dir_depth = 0;
        strncpy(rctx->tar_current_path, source, MAX_PATH_SIZE - 1);
        rctx->tar_current_path[MAX_PATH_SIZE - 1] = '\0';

        fs_dir_t_init(&rctx->tar_dirs[0]);
        if (fs_opendir(&rctx->tar_dirs[0], rctx->tar_current_path) != 0) {
            EDGEHOG_LOG_ERR("Opening directory %s failed", source);
            eres = EDGEHOG_RESULT_INTERNAL_ERROR;
            goto error;
        }

        rctx->tar_path_lens[0] = strlen(rctx->tar_current_path);
        rctx->tar_dir_depth = 1;
    } else {
        rctx->is_dir = false;
        if (out_file_size) {
            struct fs_dirent entry;
            int stat_res = fs_stat(source, &entry);
            if (stat_res != 0) {
                EDGEHOG_LOG_ERR("Failed to stat source path %s, err %d", source, stat_res);
                eres = EDGEHOG_RESULT_INTERNAL_ERROR;
                goto error;
            }
            *out_file_size = entry.size;
        }

        if (fs_open(&rctx->file, source, FS_O_READ) != 0) {
            EDGEHOG_LOG_ERR("Opening file %s failed", source);
            eres = EDGEHOG_RESULT_INTERNAL_ERROR;
            goto error;
        }
        rctx->file_open = true;
    }

    EDGEHOG_LOG_DBG("Initialized a file transfer file system read context.");
    EDGEHOG_LOG_DBG("Is tar: %d, source: %s", is_tar, source);

    *ctx = rctx;
    return EDGEHOG_RESULT_OK;

error:
    k_free(rctx);
    return eres;
}

static edgehog_result_t read_get_next_entry(
    void *ctx, char *file_name, size_t file_name_size, size_t *file_size, bool *has_next)
{
    read_ctx_t *rctx = (read_ctx_t *) ctx;
    *has_next = false;

    if (!rctx->is_dir) {
        EDGEHOG_LOG_ERR("Attempted to get next file entry from a non-directory source");
        return EDGEHOG_RESULT_INTERNAL_ERROR;
    }

    // Close the previous file handled during the last TAR iteration
    if (rctx->file_open) {
        fs_close(&rctx->file);
        rctx->file_open = false;
    }

    // Search for the next file to add in the archive
    struct fs_dirent entry = { 0 };
    while (rctx->tar_dir_depth > 0) {

        // If there are more things to look into in the deepest directory
        int current_idx = rctx->tar_dir_depth - 1;

        if ((fs_readdir(&rctx->tar_dirs[current_idx], &entry) != 0) || (entry.name[0] == '\0')) {
            tar_ascend_directory(rctx);
            continue;
        }

        if (strcmp(entry.name, ".") == 0 || strcmp(entry.name, "..") == 0) {
            continue;
        }

        size_t base_len = 0;
        edgehog_result_t path_res = tar_build_entry_path(rctx, entry.name, &base_len);
        if (path_res != EDGEHOG_RESULT_OK) {
            return path_res;
        }

        if (entry.type == FS_DIR_ENTRY_DIR) {
            edgehog_result_t eres = tar_handle_dir_entry(rctx, base_len);
            if (eres != EDGEHOG_RESULT_OK) {
                return eres;
            }
            continue;
        }

        if (entry.type == FS_DIR_ENTRY_FILE) {
            edgehog_result_t eres
                = tar_handle_file_entry(rctx, file_name, file_name_size, base_len);

            if (eres == EDGEHOG_RESULT_OK) {
                *has_next = true;
                *file_size = entry.size;
            }
            return eres;
        }
    }
    return EDGEHOG_RESULT_OK;
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
    EDGEHOG_LOG_DBG("File read has been completed, freeing context.");
    if (!rctx) {
        return EDGEHOG_RESULT_OK;
    }

    if (rctx->file_open) {
        fs_close(&rctx->file);
    }
    if (rctx->is_dir) {
        while (rctx->tar_dir_depth > 0) {
            fs_closedir(&rctx->tar_dirs[rctx->tar_dir_depth - 1]);
            rctx->tar_dir_depth--;
        }
    }

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
    if (!rctx) {
        return;
    }

    if (rctx->file_open) {
        fs_close(&rctx->file);
    }
    if (rctx->is_dir) {
        while (rctx->tar_dir_depth > 0) {
            fs_closedir(&rctx->tar_dirs[rctx->tar_dir_depth - 1]);
            rctx->tar_dir_depth--;
        }
    }

    k_free(ctx);
}

static void tar_ascend_directory(read_ctx_t *rctx)
{
    int current_idx = rctx->tar_dir_depth - 1;
    fs_closedir(&rctx->tar_dirs[current_idx]);
    rctx->tar_dir_depth--;

    if (rctx->tar_dir_depth > 0) {
        rctx->tar_current_path[rctx->tar_path_lens[rctx->tar_dir_depth - 1]] = '\0';
    }
}

static edgehog_result_t tar_build_entry_path(
    read_ctx_t *rctx, const char *entry_name, size_t *out_base_len)
{
    int current_idx = rctx->tar_dir_depth - 1;
    size_t base_len = rctx->tar_path_lens[current_idx];
    bool needs_slash = (rctx->tar_current_path[base_len - 1] != '/');
    size_t entry_len = strlen(entry_name);

    if (base_len + (needs_slash ? 1 : 0) + entry_len >= MAX_PATH_SIZE) {
        EDGEHOG_LOG_ERR("Path too long for %s", entry_name);
        return EDGEHOG_RESULT_INTERNAL_ERROR;
    }

    // Extend the current path with the new entry name
    if (needs_slash) {
        rctx->tar_current_path[base_len] = '/';
        strncpy(&rctx->tar_current_path[base_len + 1], entry_name, MAX_PATH_SIZE - base_len - 2);
    } else {
        strncpy(&rctx->tar_current_path[base_len], entry_name, MAX_PATH_SIZE - base_len - 1);
    }

    *out_base_len = base_len;
    return EDGEHOG_RESULT_OK;
}

static edgehog_result_t tar_handle_dir_entry(read_ctx_t *rctx, size_t base_len)
{
    if (rctx->tar_dir_depth >= MAX_TAR_DIR_DEPTH) {
        rctx->tar_current_path[base_len] = '\0';
        return EDGEHOG_RESULT_INTERNAL_ERROR;
    }

    fs_dir_t_init(&rctx->tar_dirs[rctx->tar_dir_depth]);
    if (fs_opendir(&rctx->tar_dirs[rctx->tar_dir_depth], rctx->tar_current_path) == 0) {
        rctx->tar_path_lens[rctx->tar_dir_depth] = strlen(rctx->tar_current_path);
        rctx->tar_dir_depth++;
    } else {
        rctx->tar_current_path[base_len] = '\0';
    }
    return EDGEHOG_RESULT_OK;
}

static edgehog_result_t tar_handle_file_entry(
    read_ctx_t *rctx, char *file_name, size_t file_name_size, size_t base_len)
{
    const char *rel_path = rctx->tar_current_path + strlen(rctx->path);
    if (*rel_path == '/') {
        rel_path++;
    }

    if (strlen(rel_path) >= file_name_size
        || fs_open(&rctx->file, rctx->tar_current_path, FS_O_READ) != 0) {
        rctx->tar_current_path[base_len] = '\0';
        return EDGEHOG_RESULT_INTERNAL_ERROR;
    }

    rctx->file_open = true;
    strncpy(file_name, rel_path, file_name_size - 1);
    file_name[file_name_size - 1] = '\0';

    rctx->tar_current_path[base_len] = '\0';
    return EDGEHOG_RESULT_OK;
}
