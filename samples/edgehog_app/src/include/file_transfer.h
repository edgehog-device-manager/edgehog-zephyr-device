/*
 * (C) Copyright 2026, SECO Mind Srl
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef APP_FILE_TRANSFER_H
#define APP_FILE_TRANSFER_H

#include <edgehog_device/file_transfer.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Callback invoked when a stream transfer is requested.
 */
bool app_on_stream_transfer_start(
    const char *name, edgehog_ft_type_t type, size_t *expected_size, edgehog_ft_stream_t *stream);

/**
 * @brief Callback invoked when a filesystem transfer has been completed.
 */
void app_on_filesystem_transfer_done(edgehog_ft_type_t type, const char *file_path);

#ifdef __cplusplus
}
#endif

#endif /* APP_FILE_TRANSFER_H */
