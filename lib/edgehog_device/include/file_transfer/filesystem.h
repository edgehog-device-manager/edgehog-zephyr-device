/*
 * (C) Copyright 2026, SECO Mind Srl
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef FILE_TRANSFER_FILESYSTEM_H
#define FILE_TRANSFER_FILESYSTEM_H

/**
 * @file file_transfer/filesystem.h
 * @brief Filesystem APIs for file transfer.
 */

#include "file_transfer/download.h"
#include "file_transfer/upload.h"

/**
 * @brief Filesystem write callbacks for file transfer.
 * This structure provides the necessary callbacks for writing data through the filesystem.
 */
extern const edgehog_ft_file_write_cbks_t edgehog_ft_filesystem_write_cbks;

/**
 * @brief Filesystem read callbacks for file transfer.
 * This structure provides the necessary callbacks for reading data through the filesystem.
 */
extern const edgehog_ft_file_read_cbks_t edgehog_ft_filesystem_read_cbks;

#endif // FILE_TRANSFER_FILESYSTEM_H
