/*
 * (C) Copyright 2026, SECO Mind Srl
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "file_transfer/upload.h"

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

EDGEHOG_LOG_MODULE_REGISTER(file_transfer_upload, CONFIG_EDGEHOG_DEVICE_FILE_TRANSFER_LOG_LEVEL);

/************************************************
 *        Defines, constants and typedef        *
 ***********************************************/

#define PROGRESS_REPORT_INTERVAL 5
#define PROGRESS_MAXIMUM 100

/************************************************
 *     Callbacks definition and declaration     *
 ***********************************************/

static edgehog_result_t http_put_device_to_server_payload_cbk(
    edgehog_http_payload_chunk_t *payload_chunk, void *user_data)
{
    edgehog_result_t eres = EDGEHOG_RESULT_OK;
    edgehog_ft_http_cbk_data_t *data = NULL;
    if (!user_data) {
        EDGEHOG_LOG_ERR("Unable to read user data context");
        goto error;
    }

    data = (edgehog_ft_http_cbk_data_t *) user_data;
    const edgehog_ft_file_read_cbks_t *file_cbks = data->file_cbks;

    if (!payload_chunk) {
        data->posix_errno = EPIPE;
        data->message = "Unable to access payload chunk";
        goto error;
    }

    // Get a chunk of the file
    uint8_t *chunk_data = NULL;
    size_t chunk_size = 0;
    bool last_chunk = true;
    eres = file_cbks->file_read_chunk(data->file_cbks_ctx, &chunk_data, &chunk_size, &last_chunk);
    if (eres != EDGEHOG_RESULT_OK) {
        data->posix_errno = EIO;
        data->message = "Failed to read chunk from storage";
        goto error;
    }

    // Transmit progress if enabled
    if (data->progress) {
        int32_t progress = 0;
        file_cbks->file_get_progress(data->file_cbks_ctx, &progress);

        // Only send the progress at meaningful intervals (5%)
        if (progress >= data->last_reported_progress + PROGRESS_REPORT_INTERVAL
            || progress == PROGRESS_MAXIMUM) {
            atomic_set(&data->current_progress, progress);
            data->last_reported_progress = progress;
            k_work_submit(&data->progress_work);
        }
    }

    payload_chunk->chunk_start_addr = chunk_data;
    payload_chunk->chunk_size = chunk_size;
    payload_chunk->last_chunk = last_chunk;

    return EDGEHOG_RESULT_OK;

error:
    return EDGEHOG_RESULT_HTTP_REQUEST_ABORTED;
}

/************************************************
 *         Global functions definitions         *
 ***********************************************/

edgehog_result_t edgehog_ft_device_to_server_event(
    edgehog_device_handle_t device, astarte_device_datastream_object_event_t *object_event)
{
    return edgehog_ft_process_event(device, object_event, EDGEHOG_FT_TYPE_DEVICE_TO_SERVER);
}

void edgehog_ft_handle_device_to_server(
    edgehog_device_handle_t edgehog_device, edgehog_ft_msg_t *msg)
{
    edgehog_result_t eres = EDGEHOG_RESULT_OK;
    int posix_errno = 0;
    char *message = "Transfer completed successfully.";

    edgehog_ft_http_cbk_data_t *http_cbk_user_data = NULL;
    bool work_initialized = false;

    // Assign the proper callbacks depending on the source type
    const edgehog_ft_file_read_cbks_t *file_cbks = NULL;
    if (strcmp(msg->location_type, "storage") == 0) {
        file_cbks = &file_transfer_storage_read_cbks;
    } else if (strcmp(msg->location_type, "stream") == 0) {
        file_cbks = &edgehog_ft_stream_read_cbks;
    } else {
        EDGEHOG_LOG_DBG("Source type: %s", msg->location_type);
        posix_errno = EINVAL;
        message = "Unknown or unsupported file transfer source type";
        goto exit;
    }

    // Initialize file context on the first chunk
    void *file_cbks_ctx = NULL;
    eres = file_cbks->file_init(&file_cbks_ctx, &edgehog_device->ft_cbks, msg->id, msg->location);
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
    http_cbk_user_data->last_reported_progress = 0;
    http_cbk_user_data->current_progress = ATOMIC_INIT(0);

    k_work_init(&http_cbk_user_data->progress_work, edgehog_ft_progress_work_handler);
    work_initialized = true;

    // Initialize the HTTP put data
    edgehog_http_put_data_t http_put_data = { .url = msg->url,
        .header_fields = (const char **) msg->http_headers,
        .timeout_ms = EDGEHOG_FT_HTTP_REQ_TIMEOUT_MS,
        .payload_cbk = http_put_device_to_server_payload_cbk,
        .user_data = http_cbk_user_data };
    // Perform the HTTP put request to upload the file
    eres = edgehog_http_put(&http_put_data);
    if (eres != EDGEHOG_RESULT_OK) {
        EDGEHOG_LOG_ERR("File transfer HTTP put failure: %d.", eres);
        posix_errno = http_cbk_user_data->posix_errno;
        message = http_cbk_user_data->message;
        file_cbks->file_abort(file_cbks_ctx);
        goto exit;
    }

    // Complete the file transfer on the backend
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
        edgehog_device, msg->id, EDGEHOG_FT_TYPE_DEVICE_TO_SERVER, posix_errno, message, eres);

    k_free(http_cbk_user_data);

    edgehog_ft_msg_destroy(msg);
}
