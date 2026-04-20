/*
 * (C) Copyright 2026, SECO Mind Srl
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef FILE_TRANSFER_STORAGE_H
#define FILE_TRANSFER_STORAGE_H

/**
 * @file file_transfer/storage.h
 * @brief Storage APIs for file transfer.
 */

#include "file_transfer/download.h"
#include "file_transfer/upload.h"

/**
 * @brief Dummy storage write callbacks for file transfer.
 * This structure provides a stub implementation of the file transfer
 * write callbacks that merely logs the operations and data to the console.
 */
extern const edgehog_ft_file_write_cbks_t file_transfer_storage_write_cbks;

/**
 * @brief Dummy storage read callbacks for file transfer.
 * This structure provides a stub implementation of the file transfer
 * read callbacks that merely logs the operations to the console.
 */
extern const edgehog_ft_file_read_cbks_t file_transfer_storage_read_cbks;

#endif // FILE_TRANSFER_STORAGE_H
