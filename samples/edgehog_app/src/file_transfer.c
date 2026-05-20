/*
 * (C) Copyright 2026, SECO Mind Srl
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include "file_transfer.h"

LOG_MODULE_REGISTER(ft, CONFIG_APP_LOG_LEVEL); // NOLINT

/************************************************
 * Defines, constants and typedef        *
 ***********************************************/

// NOLINTBEGIN(cppcoreguidelines-avoid-non-const-global-variables)
#define MAX_LOOPBACK_FILE_SIZE 4096
#define FILE_TRANSFER_THREAD_PRIORITY 5
#define CHUNK_SIZE 256

static uint8_t loopback_file_buffer[MAX_LOOPBACK_FILE_SIZE];
static size_t loopback_file_size = 0;

K_THREAD_STACK_DEFINE(download_thread_stack_area, 2048);
static struct k_thread download_thread_data;

K_THREAD_STACK_DEFINE(upload_thread_stack_area, 2048);
static struct k_thread upload_thread_data;
// NOLINTEND(cppcoreguidelines-avoid-non-const-global-variables)

/************************************************
 *      Static functions definition            *
 ***********************************************/

static void download_thread_entry_point(void *stream_pipe, void *stream_event, void *unused3)
{
    ARG_UNUSED(unused3);

    LOG_INF("Started Loopback download thread (Server -> Device RAM)."); // NOLINT

    struct k_pipe *pipe = (struct k_pipe *) stream_pipe;
    struct k_event *event = (struct k_event *) stream_event;

    loopback_file_size = 0;
    bool download_complete = false;

    while (!download_complete) {
        uint8_t chunk[CHUNK_SIZE];

        // Read progressively from the pipe provided by Edgehog
        int ret = k_pipe_read(pipe, chunk, sizeof(chunk), K_MSEC(100));
        if (ret > 0) {
            if (loopback_file_size + ret <= MAX_LOOPBACK_FILE_SIZE) {
                memcpy(loopback_file_buffer + loopback_file_size, chunk, ret);
                loopback_file_size += ret;
            } else {
                // NOLINTNEXTLINE
                LOG_ERR("Loopback buffer overflow! Max size is %d", MAX_LOOPBACK_FILE_SIZE);
                k_event_post(event, EDGEHOG_FT_STREAM_ERROR_EVENT_FLAG);
                return;
            }
        } else if (ret < 0 && ret != -EAGAIN) {
            LOG_ERR("Failed to read from loopback pipe"); // NOLINT
            k_event_post(event, EDGEHOG_FT_STREAM_ERROR_EVENT_FLAG);
            return;
        }

        // Check if Edgehog posted the EOF flag indicating download completion
        if (k_event_test(event, EDGEHOG_FT_STREAM_EOF_EVENT_FLAG)) {
            download_complete = true;
        }
    }

    // NOLINTNEXTLINE
    LOG_INF("Loopback download complete: downloaded %zu bytes to RAM", loopback_file_size);

    // Acknowledge that reading is complete so the library can safely tear down memory structures
    if (event) {
        k_event_post(event, EDGEHOG_FT_STREAM_ACK_EVENT_FLAG);
    }

    LOG_INF("Exiting download thread."); // NOLINT
}

static void upload_thread_entry_point(void *stream_pipe, void *stream_event, void *unused3)
{
    ARG_UNUSED(unused3);

    LOG_INF("Started Loopback upload thread (Device RAM -> Server)."); // NOLINT

    struct k_pipe *pipe = (struct k_pipe *) stream_pipe;
    struct k_event *event = (struct k_event *) stream_event;

    size_t total_written = 0;

    // We only enter the upload phase if we actually received a file in the download phase
    if (loopback_file_size > 0) {
        while (total_written < loopback_file_size) {

            // Write progressively to the pipe. This will block if the pipe is full,
            // properly syncing with the upload HTTP stream.
            int ret = k_pipe_write(pipe, loopback_file_buffer + total_written,
                loopback_file_size - total_written, K_MSEC(100));

            if (ret > 0) {
                total_written += ret;
            } else if (ret < 0 && ret != -EAGAIN) {
                LOG_ERR("Failed to write to loopback pipe"); // NOLINT
                break;
            }
        }

        LOG_INF("Loopback upload complete: uploaded %zu bytes from RAM", total_written); // NOLINT

        // Post EOF to inform Edgehog's read stream that there is no more data
        if (event) {
            k_event_post(event, EDGEHOG_FT_STREAM_EOF_EVENT_FLAG);
        }
    } else {
        LOG_WRN("Loopback file size is 0. Nothing to upload."); // NOLINT
        if (event) {
            k_event_post(event, EDGEHOG_FT_STREAM_EOF_EVENT_FLAG);
        }
    }

    LOG_INF("Exiting upload thread."); // NOLINT
}

/************************************************
 * Global functions definitions         *
 ***********************************************/

bool app_on_stream_transfer_start(
    const char *name, edgehog_ft_type_t type, size_t *expected_size, edgehog_ft_stream_t *stream)
{
    if (strcmp(name, "loopback") == 0) {
        // Only one file transfer can be active at a time
        struct k_pipe *pipe = stream->pipe;
        struct k_event *event = stream->event;

        if (type == EDGEHOG_FT_TYPE_SERVER_TO_DEVICE) {
            // NOLINTNEXTLINE
            LOG_INF("Starting stream transfer [server-to-device] on pipe '%s'. Expected size: %zu "
                    "bytes",
                name, *expected_size);

            // Spawn the download thread on demand
            k_thread_create(&download_thread_data, download_thread_stack_area,
                K_THREAD_STACK_SIZEOF(download_thread_stack_area), download_thread_entry_point,
                (void *) pipe, (void *) event, NULL, FILE_TRANSFER_THREAD_PRIORITY, 0, K_NO_WAIT);
        } else {
            *expected_size = loopback_file_size;

            // NOLINTNEXTLINE
            LOG_INF("Starting stream transfer [device-to-server] on pipe '%s'. Expected size: %zu "
                    "bytes",
                name, *expected_size);

            // Spawn the upload thread on demand
            k_thread_create(&upload_thread_data, upload_thread_stack_area,
                K_THREAD_STACK_SIZEOF(upload_thread_stack_area), upload_thread_entry_point,
                (void *) pipe, (void *) event, NULL, FILE_TRANSFER_THREAD_PRIORITY, 0, K_NO_WAIT);
        }

        return true;
    }

    LOG_WRN("Rejected stream transfer for unknown path/name: %s", name); // NOLINT
    return false; // Reject the transfer
}

void app_on_filesystem_transfer_done(edgehog_ft_type_t type, const char *file_path)
{
    if (type == EDGEHOG_FT_TYPE_SERVER_TO_DEVICE) {
        // NOLINTNEXTLINE
        LOG_INF(
            "Filesystem transfer complete [server-to-device]: file downloaded to '%s'", file_path);
    } else if (type == EDGEHOG_FT_TYPE_DEVICE_TO_SERVER) {
        // NOLINTNEXTLINE
        LOG_INF(
            "Filesystem transfer complete [device-to-server]: file uploaded from '%s'", file_path);
    } else {
        // NOLINTNEXTLINE
        LOG_WRN(
            "Filesystem transfer complete: unknown transfer type for '%s'", file_path); // NOLINT
    }
}
