/*
 * (C) Copyright 2026, SECO Mind Srl
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "file_transfer/filesystem_utils.h"

#include "file_transfer/core.h"

#include "log.h"
#include "ztar/core.h"
#include <string.h>
#include <zephyr/fs/fs.h>

#define MAX_PATH_SIZE 256

EDGEHOG_LOG_MODULE_REGISTER(
    file_transfer_filesystem_utils, CONFIG_EDGEHOG_DEVICE_FILE_TRANSFER_LOG_LEVEL);

/************************************************
 *         Static functions declarations        *
 ***********************************************/

static bool is_dir_empty(const char *destination);
static int walk_recursive(char path[static MAX_PATH_SIZE], fs_walk_cb_t cbk, void *user_data);
static bool is_valid_tar_destination(
    const char *destination, int stat_res, struct fs_dirent *entry);
static bool is_valid_file_destination(const char *destination, int stat_res);

/************************************************
 *     Callbacks definition and declaration     *
 ***********************************************/

void compute_size_callback(const char *path, struct fs_dirent *entry, void *user_data)
{
    ARG_UNUSED(path);
    size_t *total_size = (size_t *) user_data;

    if (entry->type == FS_DIR_ENTRY_FILE) {
        *total_size += ZTAR_BLOCK_SIZE;
        *total_size += entry->size + ZTAR_REQUIRED_FILE_PADDING(entry->size);
    }
}

/************************************************
 *         Global functions definitions         *
 ***********************************************/

bool is_valid_partition(edgehog_ft_cbks_t *cbks, const char *path,
    edgehog_ft_filesystem_permission_t req_perm, const size_t *expected_file_size)
{
    if (!cbks) {
        EDGEHOG_LOG_ERR("Missing file transfer callbacks.");
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
        if (expected_file_size && *expected_file_size > 0) {
            size_t free_space = stat.f_bfree * stat.f_frsize;
            if (free_space < *expected_file_size) {
                EDGEHOG_LOG_ERR(
                    "Insufficient k_free space on partition for %s (need %zu, have %zu)", path,
                    *expected_file_size, free_space);
                return false;
            }
        }
        return true;
    }

    return false;
}

bool is_valid_safe_path(const char *path, bool expect_root, bool expect_dir)
{
    if (path == NULL || path[0] == '\0') {
        EDGEHOG_LOG_ERR("Empty path provided.");
        return false;
    }

    // Absolute path check
    if (expect_root && (path[0] != '/')) {
        EDGEHOG_LOG_ERR("Relative path provided.");
        return false;
    }
    if (!expect_root && (path[0] == '/')) {
        EDGEHOG_LOG_ERR("Absolute path provided.");
        return false;
    }

    // Directory traversal check ("..")
    // We must look for ".." bounded by '/' or string boundaries.
    const char *curr = path;
    while ((curr = strstr(curr, "..")) != NULL) {
        // Check what comes immediately before the ".."
        bool start_bound = (curr == path) || (*(curr - 1) == '/');

        // Check what comes immediately after the ".."
        bool end_bound = (*(curr + 2) == '\0') || (*(curr + 2) == '/');

        // If ".." is a standalone component, it's a traversal attempt
        if (start_bound && end_bound) {
            EDGEHOG_LOG_ERR("Directory traversal attempt detected in path %s.", path);
            return false;
        }

        // Move past this occurrence and keep checking
        curr += 2;
    }

    // If we expect a file, it definitely should not end with a directory separator
    size_t len = strlen(path);
    if (!expect_dir && path[len - 1] == '/') {
        EDGEHOG_LOG_ERR("File path %s ends with a directory separator.", path);
        return false;
    }

    return true;
}

bool is_valid_destination(const char *destination, bool is_tar)
{
    if ((destination == NULL) || (strlen(destination) == 0)
        || (strlen(destination) >= MAX_PATH_SIZE)) {
        EDGEHOG_LOG_ERR("Destination filesystem path is missing, too long, or empty.");
        return false;
    }

    if (!is_valid_safe_path(destination, true, is_tar)) {
        EDGEHOG_LOG_ERR("Invalid destination path %s.", destination);
        return false;
    }

    struct fs_dirent entry;
    int stat_res = fs_stat(destination, &entry);
    if (stat_res != 0 && stat_res != -ENOENT) {
        EDGEHOG_LOG_ERR("Failed to stat destination path %s, err %d", destination, stat_res);
        return false;
    }

    if (is_tar) {
        return is_valid_tar_destination(destination, stat_res, &entry);
    }

    return is_valid_file_destination(destination, stat_res);
}

bool is_valid_relative_file(const char *base, const char *relative, const char *combined)
{
    if (!base || strlen(base) == 0 || strlen(base) >= MAX_PATH_SIZE || !relative
        || strlen(relative) == 0 || strlen(relative) >= MAX_PATH_SIZE || !combined
        || strlen(combined) == 0 || strlen(combined) >= MAX_PATH_SIZE) {
        EDGEHOG_LOG_ERR("One of the parameters missing or too long.");
        return false;
    }

    if (!is_valid_safe_path(relative, false, false)) {
        EDGEHOG_LOG_ERR("Invalid relative path %s.", relative);
        return false;
    }

    if (mkdir_recursive(combined, true) != 0) {
        EDGEHOG_LOG_ERR("Failed to create parent directories for %s.", combined);
        return false;
    }

    struct fs_dirent entry;
    int stat_res = fs_stat(combined, &entry);
    if (stat_res == 0) {
        EDGEHOG_LOG_ERR("File or directory already exists at %s", combined);
        return false;
    }
    if (stat_res != -ENOENT) {
        EDGEHOG_LOG_ERR("Failed to stat combined path %s, err %d", combined, stat_res);
        return false;
    }

    return true;
}

bool is_valid_source(const char *source, bool is_tar)
{
    if ((source == NULL) || (strlen(source) == 0)) {
        EDGEHOG_LOG_ERR("Missing filesystem path in source.");
        return false;
    }

    if (!is_valid_safe_path(source, true, is_tar)) {
        EDGEHOG_LOG_ERR("Invalid source path %s.", source);
        return false;
    }

    struct fs_dirent entry;
    int stat_res = fs_stat(source, &entry);
    if (stat_res != 0) {
        EDGEHOG_LOG_ERR("Failed to stat source path %s, err %d", source, stat_res);
        return false;
    }

    if (is_tar) {
        // For TAR extractions, source should be a directory that might be empty but should exist.
        if (entry.type != FS_DIR_ENTRY_DIR) {
            EDGEHOG_LOG_ERR("Source %s exists and is not a directory.", source);
            return false;
        }
        return true;
    }

    // For non-TAR writes, the destination must exist and be a file.
    if (entry.type != FS_DIR_ENTRY_FILE) {
        EDGEHOG_LOG_ERR("Source %s exists and is not a file.", source);
        return false;
    }

    return true;
}

size_t calculate_tar_directory_size(const char *dir)
{
    size_t total_size = 0;
    int ret = fs_walk(dir, compute_size_callback, &total_size);
    if (ret < 0) {
        EDGEHOG_LOG_ERR("File system size-walk failed with error: %d.", ret);
    }

    // Two empty blocks at the end of the TAR archive
    total_size += (ZTAR_BLOCK_SIZE * 2);
    return total_size;
}

int fs_walk(const char *base_path, fs_walk_cb_t cbk, void *user_data)
{
    if (strlen(base_path) >= MAX_PATH_SIZE) {
        return -ENAMETOOLONG;
    }

    char path[MAX_PATH_SIZE];
    strncpy(path, base_path, MAX_PATH_SIZE - 1);
    path[MAX_PATH_SIZE - 1] = '\0';

    return walk_recursive(path, cbk, user_data);
}

int mkdir_recursive(const char *path, bool is_file_path)
{
    char temp_path[MAX_PATH_SIZE];
    if (strlen(path) >= MAX_PATH_SIZE) {
        return -ENAMETOOLONG;
    }
    strncpy(temp_path, path, MAX_PATH_SIZE);

    // If this path points to a file, strip the file name so we only create the parent directories
    if (is_file_path) {
        char *last_slash = strrchr(temp_path, '/');
        if (!last_slash) {
            return 0;
        }
        *last_slash = '\0';
    }

    // Traverse the path and create missing directories at each '/'
    for (char *ptr = temp_path + 1; *ptr; ptr++) {
        if (*ptr == '/') {
            *ptr = '\0';
            struct fs_dirent entry;
            if (fs_stat(temp_path, &entry) != 0) {
                if (fs_mkdir(temp_path) != 0) {
                    return -EIO;
                }
            }
            *ptr = '/';
        }
    }

    // Create the final directory component
    struct fs_dirent entry;
    if (fs_stat(temp_path, &entry) != 0) {
        if (fs_mkdir(temp_path) != 0) {
            return -EIO;
        }
    }

    return 0;
}

/************************************************
 *         Static functions definitions         *
 ***********************************************/

static bool is_dir_empty(const char *destination)
{
    struct fs_dir_t dir;
    fs_dir_t_init(&dir);
    if (fs_opendir(&dir, destination) != 0) {
        EDGEHOG_LOG_ERR("Failed to open directory %s to check contents.", destination);
        return false;
    }

    struct fs_dirent dir_entry;
    bool is_empty = true;
    while (fs_readdir(&dir, &dir_entry) == 0 && dir_entry.name[0] != '\0') {
        if (strcmp(dir_entry.name, ".") != 0 && strcmp(dir_entry.name, "..") != 0) {
            is_empty = false;
            break;
        }
    }
    fs_closedir(&dir);
    return is_empty;
}

static bool is_valid_tar_destination(const char *destination, int stat_res, struct fs_dirent *entry)
{
    if (stat_res != 0) {
        if (mkdir_recursive(destination, false) != 0) {
            EDGEHOG_LOG_ERR("Failed to create destination directories for %s.", destination);
            return false;
        }
        return true;
    }

    if (entry->type != FS_DIR_ENTRY_DIR) {
        EDGEHOG_LOG_ERR("Destination %s exists and is not a directory.", destination);
        return false;
    }

    if (!is_dir_empty(destination)) {
        EDGEHOG_LOG_ERR("Destination directory %s is not empty.", destination);
        return false;
    }

    return true;
}

static bool is_valid_file_destination(const char *destination, int stat_res)
{
    if (stat_res == 0) {
        EDGEHOG_LOG_ERR("Destination path %s already exists.", destination);
        return false;
    }

    if (mkdir_recursive(destination, true) != 0) {
        EDGEHOG_LOG_ERR("Failed to create parent directories for %s.", destination);
        return false;
    }

    return true;
}

// Unfortunately a recursive chain seems the best way to perform a file system walk
// NOLINTNEXTLINE(misc-no-recursion)
static int walk_recursive(char path[static MAX_PATH_SIZE], fs_walk_cb_t cbk, void *user_data)
{
    struct fs_dir_t dir = { 0 };
    struct fs_dirent entry = { 0 };
    int err = 0;

    fs_dir_t_init(&dir);
    err = fs_opendir(&dir, path);
    if (err < 0) {
        return err;
    }

    size_t path_len = strlen(path);

    while (true) {
        err = fs_readdir(&dir, &entry);
        if (err < 0 || entry.name[0] == '\0') {
            break;
        }

        // Ignore standard current/parent directory links
        if (strcmp(entry.name, ".") == 0 || strcmp(entry.name, "..") == 0) {
            continue;
        }

        // Calculate lengths to safely construct the new path
        size_t name_len = strlen(entry.name);
        bool needs_slash = path[path_len - 1] != '/';

        // Ensure we don't overflow the shared path buffer
        if (path_len + (needs_slash ? 1 : 0) + name_len >= MAX_PATH_SIZE) {
            err = -ENAMETOOLONG;
            break;
        }

        // Append the current entry name to the path buffer
        if (needs_slash) {
            path[path_len] = '/';
            strncpy(&path[path_len + 1], entry.name, MAX_PATH_SIZE - path_len - 2);
        } else {
            strncpy(&path[path_len], entry.name, MAX_PATH_SIZE - path_len - 1);
        }
        path[MAX_PATH_SIZE - 1] = '\0';

        // If the entry is a directory, recurse into it
        if (entry.type == FS_DIR_ENTRY_DIR) {
            err = walk_recursive(path, cbk, user_data);
            if (err < 0 && err != -ENAMETOOLONG) {
                break;
            }
        }

        // Execute the user callback
        cbk(path, &entry, user_data);

        // Restore the path buffer back to the current directory level
        path[path_len] = '\0';
    }

    fs_closedir(&dir);
    return err;
}
