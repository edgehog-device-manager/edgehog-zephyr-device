/*
 * (C) Copyright 2026, SECO Mind Srl
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "file_transfer/upload.h"

#include "edgehog_private.h"
#include "file_transfer/compression.h"
#include "file_transfer/core.h"
#include "file_transfer/filesystem.h"
#include "file_transfer/stream.h"
#include "file_transfer/utils.h"
#include "http.h"
#include "log.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

EDGEHOG_LOG_MODULE_REGISTER(file_transfer_upload, CONFIG_EDGEHOG_DEVICE_FILE_TRANSFER_LOG_LEVEL);

#ifdef CONFIG_EDGEHOG_DEVICE_FILE_TRANSFER_COMPRESSION
// Defines a safe margin for LZ4 compression buffer overhead
#define EDGEHOG_COMPRESSION_SAFE_MARGIN 64
#endif

/************************************************
 *         Static functions declarations        *
 ***********************************************/

#ifdef CONFIG_EDGEHOG_DEVICE_FILE_TRANSFER_COMPRESSION
static edgehog_result_t process_compressed_upload_chunk(
    edgehog_ft_http_cbk_data_t *data, edgehog_http_payload_chunk_t *payload_chunk);
static edgehog_result_t init_upload_compression(
    edgehog_ft_http_cbk_data_t *data, size_t *lz4_bytes_written);
static edgehog_result_t compress_next_upload_chunk(
    edgehog_ft_http_cbk_data_t *data, size_t *lz4_bytes_written);
static edgehog_result_t write_upload_compression_footer(
    edgehog_ft_http_cbk_data_t *data, size_t *lz4_bytes_written);
#endif
static edgehog_result_t process_uncompressed_upload_chunk(
    edgehog_ft_http_cbk_data_t *data, edgehog_http_payload_chunk_t *payload_chunk);
static const edgehog_ft_file_read_cbks_t *get_callbacks(const char *source_type);

/************************************************
 *     Callbacks definition and declaration     *
 ***********************************************/

static edgehog_result_t http_put_device_to_server_payload_cbk(
    edgehog_http_payload_chunk_t *payload_chunk, void *user_data)
{
    edgehog_ft_http_cbk_data_t *data = NULL;

    if (!user_data) {
        EDGEHOG_LOG_ERR("Unable to read user data context");
        return EDGEHOG_RESULT_HTTP_REQUEST_ABORTED;
    }

    data = (edgehog_ft_http_cbk_data_t *) user_data;

    if (!payload_chunk) {
        data->posix_errno = EPIPE;
        data->message = "Unable to access payload chunk";
#ifdef CONFIG_EDGEHOG_DEVICE_FILE_TRANSFER_COMPRESSION
        if (data->encoding == EDGEHOG_FT_ENCODING_LZ4) {
            file_transfer_compression_free(&data->comp_ctx);
        }
#endif
        return EDGEHOG_RESULT_HTTP_REQUEST_ABORTED;
    }

#ifdef CONFIG_EDGEHOG_DEVICE_FILE_TRANSFER_COMPRESSION
    if (data->encoding == EDGEHOG_FT_ENCODING_LZ4) {
        return process_compressed_upload_chunk(data, payload_chunk);
    }
#endif

    return process_uncompressed_upload_chunk(data, payload_chunk);
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

    const edgehog_ft_file_read_cbks_t *file_cbks = get_callbacks(msg->location_type);
    if (!file_cbks) {
        EDGEHOG_LOG_DBG("Source type: %s", msg->location_type);
        posix_errno = EINVAL;
        message = "Unknown or unsupported file transfer source type";
        goto exit;
    }

    // Initialize file context on the first chunk
    void *file_cbks_ctx = NULL;
    eres = file_cbks->file_init(
        &file_cbks_ctx, &edgehog_device->file_transfer->cbks, msg->id, msg->location);
    if (eres != EDGEHOG_RESULT_OK) {
        posix_errno = EIO;
        message = "Failed to initialize the file backend";
        goto exit;
    }

    // TODO: add also errno and message in the `_new` function
    // Initialize the user data for the HTTP callback
    // Must be allocated on the heap since it needs to be accessed in the work thread.
    http_cbk_user_data
        = edgehog_ft_http_cbk_data_new(edgehog_device, msg, file_cbks, file_cbks_ctx);
    if (!http_cbk_user_data) {
        posix_errno = ENOSR;
        message = "Out of memory in file transfer.";
        goto exit;
    }
    http_cbk_user_data->posix_errno = posix_errno;
    http_cbk_user_data->message = message;

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
    edgehog_ft_http_cbk_data_destroy(http_cbk_user_data);
    edgehog_ft_send_response(
        edgehog_device, msg->id, EDGEHOG_FT_TYPE_DEVICE_TO_SERVER, posix_errno, message, eres);
    edgehog_ft_msg_destroy(msg);
}

/************************************************
 *         Static functions definitions         *
 ***********************************************/

#ifdef CONFIG_EDGEHOG_DEVICE_FILE_TRANSFER_COMPRESSION
static edgehog_result_t process_compressed_upload_chunk(
    edgehog_ft_http_cbk_data_t *data, edgehog_http_payload_chunk_t *payload_chunk)
{
    size_t lz4_bytes_written = 0;
    edgehog_result_t eres = EDGEHOG_RESULT_OK;

    // Initialize and write the LZ4 Header if not already done
    if (!file_transfer_compression_is_initialized(&data->comp_ctx)) {
        eres = init_upload_compression(data, &lz4_bytes_written);
        if (eres != EDGEHOG_RESULT_OK) {
            return eres;
        }
    } else {
        // Keep looping until we have something to send via HTTP or we write the footer
        while (lz4_bytes_written == 0 && !data->comp_footer_written) {

            if (!data->file_exhausted) {
                eres = compress_next_upload_chunk(data, &lz4_bytes_written);
                if (eres != EDGEHOG_RESULT_OK) {
                    return eres;
                }
            }

            // If file is fully read, write the LZ4 Footer
            if (data->file_exhausted && !data->comp_footer_written) {
                eres = write_upload_compression_footer(data, &lz4_bytes_written);
                if (eres != EDGEHOG_RESULT_OK) {
                    return eres;
                }
            }
        }
    }

    // Set the pointers for the HTTP payload chunk
    payload_chunk->chunk_start_addr = data->comp_out_buf;
    payload_chunk->chunk_size = lz4_bytes_written;
    payload_chunk->last_chunk = data->comp_footer_written;

    // Cleanup after final HTTP transmission
    if (data->comp_footer_written) {
        file_transfer_compression_free(&data->comp_ctx);
    }

    return EDGEHOG_RESULT_OK;
}

static edgehog_result_t init_upload_compression(
    edgehog_ft_http_cbk_data_t *data, size_t *lz4_bytes_written)
{
    int ret = file_transfer_compression_init(&data->comp_ctx);
    if (ret != 0) {
        data->posix_errno = ENOMEM;
        data->message = "Compression failure";
        EDGEHOG_LOG_ERR("%s in compression initialization: %d", data->message, ret);
        file_transfer_compression_free(&data->comp_ctx);
        return EDGEHOG_RESULT_HTTP_REQUEST_ABORTED;
    }

    ret = file_transfer_compression_begin(
        &data->comp_ctx, data->comp_out_buf, sizeof(data->comp_out_buf), lz4_bytes_written);
    if (ret != 0) {
        data->posix_errno = EIO;
        data->message = "Compression failure";
        EDGEHOG_LOG_ERR("%s in compression begin: %d", data->message, ret);
        file_transfer_compression_free(&data->comp_ctx);
        return EDGEHOG_RESULT_HTTP_REQUEST_ABORTED;
    }

    return EDGEHOG_RESULT_OK;
}

static edgehog_result_t compress_next_upload_chunk(
    edgehog_ft_http_cbk_data_t *data, size_t *lz4_bytes_written)
{
    const edgehog_ft_file_read_cbks_t *file_cbks
        = (const edgehog_ft_file_read_cbks_t *) data->file_cbks;
    uint8_t *chunk_data = NULL;
    size_t chunk_size = 0;
    int ret = 0;

    // Calculate safe read size based on remaining output buffer space
    size_t available_space = sizeof(data->comp_out_buf) - *lz4_bytes_written;
    size_t safe_max_read = (available_space > EDGEHOG_COMPRESSION_SAFE_MARGIN)
        ? (available_space - EDGEHOG_COMPRESSION_SAFE_MARGIN)
        : 0;

    edgehog_result_t eres = file_cbks->file_read_chunk(
        data->file_cbks_ctx, safe_max_read, &chunk_data, &chunk_size, &data->file_exhausted);

    if (eres != EDGEHOG_RESULT_OK) {
        data->posix_errno = EIO;
        data->message = "Failed to read chunk from file";
        EDGEHOG_LOG_ERR("%s: %d", data->message, eres);
        file_transfer_compression_free(&data->comp_ctx);
        return EDGEHOG_RESULT_HTTP_REQUEST_ABORTED;
    }

    // Feed it to the compressor
    if (chunk_size > 0) {
        size_t chunk_written = 0;
        ret = file_transfer_compression_update(&data->comp_ctx, chunk_data, chunk_size,
            data->comp_out_buf + *lz4_bytes_written,
            sizeof(data->comp_out_buf) - *lz4_bytes_written, &chunk_written);
        if (ret != 0) {
            data->posix_errno = EIO;
            data->message = "Compression failure";
            EDGEHOG_LOG_ERR("%s in update: %d", data->message, ret);
            file_transfer_compression_free(&data->comp_ctx);
            return EDGEHOG_RESULT_HTTP_REQUEST_ABORTED;
        }

        *lz4_bytes_written += chunk_written;
        edgehog_ft_update_progress(data, chunk_size, false);
    }

    return EDGEHOG_RESULT_OK;
}

static edgehog_result_t write_upload_compression_footer(
    edgehog_ft_http_cbk_data_t *data, size_t *lz4_bytes_written)
{
    size_t chunk_written = 0;
    int ret
        = file_transfer_compression_end(&data->comp_ctx, data->comp_out_buf + *lz4_bytes_written,
            sizeof(data->comp_out_buf) - *lz4_bytes_written, &chunk_written);

    if (ret != 0) {
        data->posix_errno = EIO;
        data->message = "Compression failure";
        EDGEHOG_LOG_ERR("%s, lz4 footer failure: %d", data->message, ret);
        file_transfer_compression_free(&data->comp_ctx);
        return EDGEHOG_RESULT_HTTP_REQUEST_ABORTED;
    }

    *lz4_bytes_written += chunk_written;
    data->comp_footer_written = true;
    edgehog_ft_update_progress(data, 0, true);

    return EDGEHOG_RESULT_OK;
}
#endif

static edgehog_result_t process_uncompressed_upload_chunk(
    edgehog_ft_http_cbk_data_t *data, edgehog_http_payload_chunk_t *payload_chunk)
{
    const edgehog_ft_file_read_cbks_t *file_cbks
        = (const edgehog_ft_file_read_cbks_t *) data->file_cbks;
    edgehog_result_t eres = EDGEHOG_RESULT_OK;
    uint8_t *chunk_data = NULL;
    size_t chunk_size = 0;
    bool last_chunk = true;

    // No maximum read in this case, leave it to the file read backend to limit the chunk size
    size_t max_read = SIZE_MAX;
    eres = file_cbks->file_read_chunk(
        data->file_cbks_ctx, max_read, &chunk_data, &chunk_size, &last_chunk);
    if (eres != EDGEHOG_RESULT_OK) {
        data->posix_errno = EIO;
        data->message = "Failed to read chunk from storage";
        return EDGEHOG_RESULT_HTTP_REQUEST_ABORTED;
    }

    edgehog_ft_update_progress(data, chunk_size, last_chunk);

    payload_chunk->chunk_start_addr = chunk_data;
    payload_chunk->chunk_size = chunk_size;
    payload_chunk->last_chunk = last_chunk;

    return EDGEHOG_RESULT_OK;
}

const edgehog_ft_file_read_cbks_t *get_callbacks(const char *source_type)
{
    const edgehog_ft_file_read_cbks_t *file_cbks = NULL;
    if (strcmp(source_type, "stream") == 0) {
        file_cbks = &edgehog_ft_stream_read_cbks;
    } else if (strcmp(source_type, "filesystem") == 0) {
        file_cbks = &edgehog_ft_filesystem_read_cbks;
    }
    return file_cbks;
}
