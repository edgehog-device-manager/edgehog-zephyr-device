/*
 * (C) Copyright 2026, SECO Mind Srl
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "file_transfer/download.h"

#include "edgehog_private.h"
#include "file_transfer/core.h"
#include "file_transfer/storage.h"
#include "file_transfer/stream.h"
#include "file_transfer/utils.h"
#include "http.h"
#include "log.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

EDGEHOG_LOG_MODULE_REGISTER(file_transfer_download, CONFIG_EDGEHOG_DEVICE_FILE_TRANSFER_LOG_LEVEL);

/************************************************
 *        Defines, constants and typedef        *
 ***********************************************/

#define PROGRESS_REPORT_INTERVAL_PERCENT 5
#define PROGRESS_REPORT_INTERVAL_BYTES (256 * 1024)
#define PROGRESS_ONE_HUNDRED_PERCENT 100

/************************************************
 *     Callbacks definition and declaration     *
 ***********************************************/

static edgehog_result_t http_get_server_to_device_request_cbk(
    edgehog_http_response_chunk_t *response_chunk, void *user_data)
{
    edgehog_result_t eres = EDGEHOG_RESULT_OK;
    edgehog_ft_http_cbk_data_t *data = NULL;
    if (!user_data) {
        EDGEHOG_LOG_ERR("Unable to read user data context");
        goto error;
    }

    data = (edgehog_ft_http_cbk_data_t *) user_data;
    const edgehog_ft_file_write_cbks_t *file_cbks = data->file_cbks;

    if (!response_chunk) {
        data->posix_errno = EPIPE;
        data->message = "Unable to read chunk data";
        goto error;
    }

    // Process this chunk of the file
    eres = file_cbks->file_append_chunk(
        data->file_cbks_ctx, response_chunk->chunk_start_addr, response_chunk->chunk_size);
    if (eres != EDGEHOG_RESULT_OK) {
        data->posix_errno = EIO;
        data->message = "Failed to write chunk to file";
        goto error;
    }

    // Transmit progress if enabled
    if (data->progress) {
        size_t last_reported_bytes = atomic_get(&data->last_reported_bytes);
        data->transferred_bytes = data->transferred_bytes + response_chunk->chunk_size;

        bool should_report = false;
        if (data->total_bytes > 0) {
            // If total size is known, report based on percentage intervals
            size_t current_percent
                = (data->transferred_bytes * PROGRESS_ONE_HUNDRED_PERCENT) / data->total_bytes;
            size_t last_percent
                = (last_reported_bytes * PROGRESS_ONE_HUNDRED_PERCENT) / data->total_bytes;

            if (current_percent >= last_percent + PROGRESS_REPORT_INTERVAL_PERCENT
                || response_chunk->last_chunk) {
                should_report = true;
            }
        } else {
            // If total size is unknown, report based on byte intervals
            if (data->transferred_bytes >= last_reported_bytes + PROGRESS_REPORT_INTERVAL_BYTES
                || response_chunk->last_chunk) {
                should_report = true;
            }
        }

        if (should_report) {
            atomic_set(&data->last_reported_bytes, (atomic_val_t) data->transferred_bytes);
            k_work_submit(&data->progress_work);
        }
    }

    return EDGEHOG_RESULT_OK;

error:
    return EDGEHOG_RESULT_HTTP_REQUEST_ABORTED;
}

/************************************************
 *         Global functions definitions         *
 ***********************************************/

edgehog_result_t edgehog_ft_server_to_device_event(
    edgehog_device_handle_t device, astarte_device_datastream_object_event_t *object_event)
{
    return edgehog_ft_process_event(device, object_event, EDGEHOG_FT_TYPE_SERVER_TO_DEVICE);
}

void edgehog_ft_handle_server_to_device(
    edgehog_device_handle_t edgehog_device, edgehog_ft_msg_t *msg)
{
    edgehog_result_t eres = EDGEHOG_RESULT_OK;
    int posix_errno = 0;
    char *message = "Transfer completed successfully.";

    edgehog_ft_http_cbk_data_t *http_cbk_user_data = NULL;
    bool work_initialized = false;

    // Check that file size does not exceeds the max size contained in a size_t
    if ((msg->file_size_bytes < 0) || (msg->file_size_bytes > SIZE_MAX)) {
        EDGEHOG_LOG_ERR("Requested file transfer file size too big: %lld", msg->file_size_bytes);
        posix_errno = EINVAL;
        message = "Requested file transfer file size too big";
        goto exit;
    }

    // Assign the proper callbacks depending on the destination
    const edgehog_ft_file_write_cbks_t *file_cbks = NULL;
    if (strcmp(msg->location_type, "storage") == 0) {
        file_cbks = &file_transfer_storage_write_cbks;
    } else if (strcmp(msg->location_type, "stream") == 0) {
        file_cbks = &edgehog_ft_stream_write_cbks;
    } else {
        EDGEHOG_LOG_DBG("Destination type: %s", msg->location_type);
        posix_errno = EINVAL;
        message = "Unknown or unsupported file transfer destination type";
        goto exit;
    }

    // Initialize file context
    void *file_cbks_ctx = NULL;
    eres = file_cbks->file_init(&file_cbks_ctx, &edgehog_device->ft_cbks, msg->id, msg->url,
        msg->file_size_bytes, msg->location);
    if (eres != EDGEHOG_RESULT_OK) {
        posix_errno = EIO;
        message = "Failed to initialize the file backend";
        goto exit;
    }

    // Initialize the user data for the HTTP callback
    // Must be allocated on the heap since it needs to be accessed in the work thread.
    http_cbk_user_data = k_calloc(1, sizeof(edgehog_ft_http_cbk_data_t));
    if (!http_cbk_user_data) {
        posix_errno = ENOSR;
        message = "Out of memory in file transfer.";
        goto exit;
    }

    http_cbk_user_data->edgehog_device = edgehog_device;
    http_cbk_user_data->id = msg->id;
    http_cbk_user_data->progress = msg->progress;
    http_cbk_user_data->type = msg->type;
    http_cbk_user_data->file_cbks = file_cbks;
    http_cbk_user_data->file_cbks_ctx = file_cbks_ctx;
    http_cbk_user_data->posix_errno = posix_errno;
    http_cbk_user_data->message = message;
    http_cbk_user_data->transferred_bytes = 0;
    http_cbk_user_data->total_bytes = msg->file_size_bytes;
    http_cbk_user_data->last_reported_bytes = ATOMIC_INIT(0);

    // Initialize the worker for progress updates
    k_work_init(&http_cbk_user_data->progress_work, edgehog_ft_progress_work_handler);
    work_initialized = true;
    // Initialize the HTTP get msg
    edgehog_http_get_data_t http_get_data = {
        .url = msg->url,
        .header_fields = (const char **) msg->http_headers,
        .timeout_ms = EDGEHOG_FT_HTTP_REQ_TIMEOUT_MS,
        .response_cbk = http_get_server_to_device_request_cbk,
        .user_data = http_cbk_user_data,
    };
    // Perform the HTTP get request to fetch the file
    eres = edgehog_http_get(&http_get_data);
    if (eres != EDGEHOG_RESULT_OK) {
        EDGEHOG_LOG_ERR("File transfer HTTP get failure: %d.", eres);
        posix_errno = http_cbk_user_data->posix_errno;
        message = http_cbk_user_data->message;
        file_cbks->file_abort(file_cbks_ctx);
        goto exit;
    }

    eres = file_cbks->file_complete(file_cbks_ctx);
    if (eres != EDGEHOG_RESULT_OK) {
        posix_errno = EIO;
        message = "Failed to finalize file transfer";
    }

exit:
    if (work_initialized) {
        struct k_work_sync sync;
        k_work_cancel_sync(&http_cbk_user_data->progress_work, &sync);
    }

    edgehog_ft_send_response(
        edgehog_device, msg->id, EDGEHOG_FT_TYPE_SERVER_TO_DEVICE, posix_errno, message, eres);

    k_free(http_cbk_user_data);

    edgehog_ft_msg_destroy(msg);
}
