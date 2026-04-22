/*
 * (C) Copyright 2026, SECO Mind Srl
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef FILE_TRANSFER_STREAM_H
#define FILE_TRANSFER_STREAM_H

/**
 * @file file_transfer/stream.h
 * @brief Stream APIs for file transfer.
 */

#include "file_transfer/download.h"
#include "file_transfer/upload.h"

/**
 * @brief Stream write callbacks for file transfer.
 * This structure provides the necessary callbacks for writing data through a stream.
 */
extern const edgehog_ft_file_write_cbks_t edgehog_ft_stream_write_cbks;

/**
 * @brief Stream read callbacks for file transfer.
 * This structure provides the necessary callbacks for reading data through a stream.
 */
extern const edgehog_ft_file_read_cbks_t edgehog_ft_stream_read_cbks;

#endif // FILE_TRANSFER_STREAM_H
