/*
 * (C) Copyright 2026, SECO Mind Srl
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef FILE_TRANSFER_FILESYSTEM_UTILS_H
#define FILE_TRANSFER_FILESYSTEM_UTILS_H

/**
 * @file file_transfer/filesystem_utils.h
 * @brief File transfer file system utility APIs.
 */

#include "edgehog_device/file_transfer.h"

#include <zephyr/fs/fs.h>

/**
 * @brief Callback function type for filesystem walk operations.
 *
 * @param path The current file or directory path being visited.
 * @param entry Pointer to the filesystem directory entry information.
 * @param user_data Opaque pointer to user-provided context data.
 */
typedef void (*fs_walk_cb_t)(const char *path, struct fs_dirent *entry, void *user_data);

/**
 * @brief Checks if a partition is valid for a file transfer operation.
 *
 * @param cbks Pointer to the file transfer callbacks context.
 * @param path The path to the partition or file being checked.
 * @param req_perm The required filesystem permissions for the operation.
 * @param expected_file_size Pointer to the expected size of the file (can be NULL).
 * @return true if the partition is valid and meets the requirements, false otherwise.
 */
bool is_valid_partition(edgehog_ft_cbks_t *cbks, const char *path,
    edgehog_ft_filesystem_permission_t req_perm, const size_t *expected_file_size);

/**
 * @brief Validates if a path is safe and meets structural expectations.
 *
 * @param path The path to validate.
 * @param expect_root Flag indicating if the path must be an absolute/root path.
 * @param expect_dir Flag indicating if the path is expected to be a directory.
 * @return true if the path is safe and valid, false otherwise.
 */
bool is_valid_safe_path(const char *path, bool expect_root, bool expect_dir);

/**
 * @brief Validates the destination path for a file transfer download.
 *
 * @param destination The target destination path.
 * @param is_tar Flag indicating if the incoming payload is a TAR archive.
 * @return true if the destination is valid, false otherwise.
 */
bool is_valid_destination(const char *destination, bool is_tar);

/**
 * @brief Validates a relative file path against a base path to prevent directory traversal attacks.
 *
 * @param base The base directory path.
 * @param relative The relative file path to append.
 * @param combined The resulting combined path to validate.
 * @return true if the combined path is safely within the base directory, false otherwise.
 */
bool is_valid_relative_file(const char *base, const char *relative, const char *combined);

/**
 * @brief Validates the source path for a file transfer upload.
 *
 * @param source The source path to validate.
 * @param is_tar Flag indicating if the source is expected to be treated as a TAR archive.
 * @return true if the source is valid, false otherwise.
 */
bool is_valid_source(const char *source, bool is_tar);

/**
 * @brief Calculates the total size required to package a directory into a TAR archive.
 *
 * @param dir The path to the directory.
 * @return The total calculated size in bytes.
 */
size_t calculate_tar_directory_size(const char *dir);

/**
 * @brief Recursively walks through a directory structure.
 *
 * @param base_path The starting directory path for the walk.
 * @param cbk The callback function executed for each file or directory entry found.
 * @param user_data Opaque pointer to user data passed to the callback function.
 * @return 0 on success, or a negative error code on failure.
 */
int fs_walk(const char *base_path, fs_walk_cb_t cbk, void *user_data);

/**
 * @brief Recursively creates directories for a given path.
 *
 * @param path The path for which directories should be created.
 * @param is_file_path If true, treats the last component of the path as a file and only creates its
 * parent directories.
 * @return 0 on success, or a negative error code on failure.
 */
int mkdir_recursive(const char *path, bool is_file_path);

#endif // FILE_TRANSFER_FILESYSTEM_UTILS_H
