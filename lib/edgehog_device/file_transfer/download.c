/*
 * (C) Copyright 2026, SECO Mind Srl
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "file_transfer/download.h"

#include "edgehog_private.h"
#include "file_transfer/core.h"
#include "file_transfer/decompression.h"
#include "file_transfer/filesystem.h"
#include "file_transfer/stream.h"
#include "file_transfer/utils.h"
#include "http.h"
#include "log.h"

#include <psa/crypto.h>
#include <zephyr/sys/util.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

EDGEHOG_LOG_MODULE_REGISTER(file_transfer_download, CONFIG_EDGEHOG_DEVICE_FILE_TRANSFER_LOG_LEVEL);

/************************************************
 *        Defines, constants and typedef        *
 ***********************************************/

#define DIGEST_PREFIX "sha256:"
#define DIGEST_PREFIX_LEN (sizeof(DIGEST_PREFIX) - 1)
#define SHA256_BYTES_LEN 32
#define SHA256_HEX_STR_LEN (SHA256_BYTES_LEN * 2)

/************************************************
 *         Static functions declarations        *
 ***********************************************/

#ifdef CONFIG_EDGEHOG_DEVICE_FILE_TRANSFER_COMPRESSION
static edgehog_result_t process_compressed_chunk(
    edgehog_ft_http_cbk_data_t *data, const edgehog_http_response_chunk_t *response_chunk);
#endif
static edgehog_result_t process_uncompressed_chunk(
    edgehog_ft_http_cbk_data_t *data, const edgehog_http_response_chunk_t *response_chunk);
static const edgehog_ft_file_write_cbks_t *get_callbacks(const char *destination_type);
static edgehog_result_t setup_digest(edgehog_ft_http_cbk_data_t *data);
static int verify_digest(edgehog_ft_http_cbk_data_t *data, const char *expected_digest);

/************************************************
 *     Callbacks definition and declaration     *
 ***********************************************/

#ifdef CONFIG_EDGEHOG_DEVICE_FILE_TRANSFER_COMPRESSION
static int decompression_write_cbk(const uint8_t *data_chunk, size_t size, void *user_data)
{
    edgehog_ft_http_cbk_data_t *data = (edgehog_ft_http_cbk_data_t *) user_data;
    const edgehog_ft_file_write_cbks_t *file_cbks
        = (const edgehog_ft_file_write_cbks_t *) data->file_cbks;

    // Append the decompressed chunk to the file
    edgehog_result_t eres = file_cbks->file_append_chunk(data->file_cbks_ctx, data_chunk, size);
    if (eres != EDGEHOG_RESULT_OK) {
        data->posix_errno = EIO;
        data->message = "Failed to write decompressed chunk to file";
        return -1;
    }

    // Stream the uncompressed chunk into the SHA-256 hash operation
    if (data->expected_digest) {
        psa_status_t status = psa_hash_update(&data->hash_operation, data_chunk, size);
        if (status != PSA_SUCCESS) {
            data->posix_errno = EIO;
            data->message = "Failed to update file digest";
            return -1;
        }
    }

    // Update progress with the uncompressed size
    edgehog_ft_update_progress(data, size, false);
    return 0;
}
#endif

static edgehog_result_t http_get_server_to_device_request_cbk(
    edgehog_http_response_chunk_t *response_chunk, void *user_data)
{
    edgehog_ft_http_cbk_data_t *data = NULL;

    if (!user_data) {
        EDGEHOG_LOG_ERR("Unable to read user data context");
        return EDGEHOG_RESULT_HTTP_REQUEST_ABORTED;
    }

    data = (edgehog_ft_http_cbk_data_t *) user_data;

    if (!response_chunk) {
        data->posix_errno = EPIPE;
        data->message = "Unable to read chunk data";
        return EDGEHOG_RESULT_HTTP_REQUEST_ABORTED;
    }

#ifdef CONFIG_EDGEHOG_DEVICE_FILE_TRANSFER_COMPRESSION
    if (data->encoding == EDGEHOG_FT_ENCODING_LZ4) {
        return process_compressed_chunk(data, response_chunk);
    }
#endif

    // Fallthrough for uncompressed, or if compression is disabled
    return process_uncompressed_chunk(data, response_chunk);
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
    bool digest_active = false;

    // Check that file size does not exceeds the max size contained in a size_t
    if ((msg->file_size_bytes < 0) || (msg->file_size_bytes > SIZE_MAX)) {
        EDGEHOG_LOG_ERR("Requested file transfer file size too big: %lld", msg->file_size_bytes);
        posix_errno = EINVAL;
        message = "Requested file transfer file size too big";
        goto exit;
    }

    const edgehog_ft_file_write_cbks_t *file_cbks = get_callbacks(msg->location_type);
    if (!file_cbks) {
        EDGEHOG_LOG_DBG("Destination type: %s", msg->location_type);
        posix_errno = EINVAL;
        message = "Unknown or unsupported file transfer destination type";
        goto exit;
    }

    // Initialize file context
    void *file_cbks_ctx = NULL;
    eres = file_cbks->file_init(&file_cbks_ctx, &edgehog_device->file_transfer->cbks, msg->id,
        msg->url, msg->file_size_bytes, msg->location);
    if (eres != EDGEHOG_RESULT_OK) {
        posix_errno = EIO;
        message = "Failed to initialize the file backend";
        goto exit;
    }

    // TODO: initialize also digest, posix errno and message int the `_new` function
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
    http_cbk_user_data->expected_digest = msg->digest;

    if (setup_digest(http_cbk_user_data) != EDGEHOG_RESULT_OK) {
        posix_errno = EIO;
        message = "Failed to initialize crypto subsystem";
        file_cbks->file_abort(file_cbks_ctx);
        goto exit;
    }
    digest_active = true;

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

    // Finalize and verify the digest before completing the file
    if (msg->digest) {
        int verify_res = verify_digest(http_cbk_user_data, msg->digest);
        if (verify_res != 0) {
            posix_errno = verify_res;
            message = (verify_res == EINVAL) ? "File digest mismatch"
                                             : "Failed to finalize file digest";
            goto exit;
        }
        digest_active = false;
    }

    eres = file_cbks->file_complete(file_cbks_ctx);
    if (eres != EDGEHOG_RESULT_OK) {
        posix_errno = EIO;
        message = "Failed to finalize file transfer";
        goto exit;
    }

exit:
    if (http_cbk_user_data && msg->digest && posix_errno != 0 && digest_active) {
        psa_hash_abort(&http_cbk_user_data->hash_operation);
    }

    edgehog_ft_http_cbk_data_destroy(http_cbk_user_data);
    edgehog_ft_send_response(
        edgehog_device, msg->id, EDGEHOG_FT_TYPE_SERVER_TO_DEVICE, posix_errno, message, eres);
    edgehog_ft_msg_destroy(msg);
}

/************************************************
 *         Static functions definitions         *
 ***********************************************/

#ifdef CONFIG_EDGEHOG_DEVICE_FILE_TRANSFER_COMPRESSION
static edgehog_result_t process_compressed_chunk(
    edgehog_ft_http_cbk_data_t *data, const edgehog_http_response_chunk_t *response_chunk)
{
    int ret = 0;

    // Initialize context on the first chunk
    if (!file_transfer_decompression_is_initialized(&data->decomp_ctx)) {
        ret = file_transfer_decompression_init(&data->decomp_ctx, decompression_write_cbk, data);
        if (ret != 0) {
            data->posix_errno = ENOMEM;
            data->message = "Failed to initialize decompression context";
            return EDGEHOG_RESULT_HTTP_REQUEST_ABORTED;
        }
    }

    // Process incoming HTTP chunk
    if (response_chunk->chunk_size > 0) {
        ret = file_transfer_decompression_process_chunk(
            &data->decomp_ctx, response_chunk->chunk_start_addr, response_chunk->chunk_size);
        if (ret != 0) {
            data->posix_errno = EIO;
            data->message = "Decompression chunk processing failed";
            file_transfer_decompression_free(&data->decomp_ctx);
            return EDGEHOG_RESULT_HTTP_REQUEST_ABORTED;
        }
    }

    // Finalize
    if (response_chunk->last_chunk) {
        edgehog_ft_update_progress(data, 0, true);
        file_transfer_decompression_free(&data->decomp_ctx);
    }

    return EDGEHOG_RESULT_OK;
}
#endif

static edgehog_result_t process_uncompressed_chunk(
    edgehog_ft_http_cbk_data_t *data, const edgehog_http_response_chunk_t *response_chunk)
{
    const edgehog_ft_file_write_cbks_t *file_cbks
        = (const edgehog_ft_file_write_cbks_t *) data->file_cbks;
    edgehog_result_t eres = EDGEHOG_RESULT_OK;

    // Process this chunk of the file
    eres = file_cbks->file_append_chunk(
        data->file_cbks_ctx, response_chunk->chunk_start_addr, response_chunk->chunk_size);
    if (eres != EDGEHOG_RESULT_OK) {
        data->posix_errno = EIO;
        data->message = "Failed to write chunk to file";
        return eres;
    }

    // Stream the new chunk into the SHA-256 hash operation
    if (data->expected_digest) {
        psa_status_t status = psa_hash_update(
            &data->hash_operation, response_chunk->chunk_start_addr, response_chunk->chunk_size);
        if (status != PSA_SUCCESS) {
            data->posix_errno = EIO;
            data->message = "Failed to update file digest";
            return EDGEHOG_RESULT_HTTP_REQUEST_ABORTED;
        }
    }

    edgehog_ft_update_progress(data, response_chunk->chunk_size, response_chunk->last_chunk);

    return EDGEHOG_RESULT_OK;
}

static edgehog_result_t setup_digest(edgehog_ft_http_cbk_data_t *data)
{
    if (!data->expected_digest) {
        return EDGEHOG_RESULT_OK;
    }

    // initialize PSA
    psa_status_t status = psa_crypto_init();
    if (status != PSA_SUCCESS) {
        EDGEHOG_LOG_ERR("psa_crypto_init returned %d", status);
        return EDGEHOG_RESULT_INTERNAL_ERROR;
    }

    data->hash_operation = psa_hash_operation_init();
    status = psa_hash_setup(&data->hash_operation, PSA_ALG_SHA_256);
    if (status != PSA_SUCCESS) {
        EDGEHOG_LOG_ERR("Failed to initialize PSA hash operation");
        return EDGEHOG_RESULT_INTERNAL_ERROR;
    }
    return EDGEHOG_RESULT_OK;
}

static int verify_digest(edgehog_ft_http_cbk_data_t *data, const char *expected_digest)
{
    // Verify the expected_digest has the correct prefix and length
    if (strncmp(expected_digest, DIGEST_PREFIX, DIGEST_PREFIX_LEN) != 0) {
        EDGEHOG_LOG_ERR("Invalid digest prefix. Expected: %s", DIGEST_PREFIX);
        return EINVAL;
    }

    const char *expected_hex = expected_digest + DIGEST_PREFIX_LEN;
    if (strlen(expected_hex) != SHA256_HEX_STR_LEN) {
        EDGEHOG_LOG_ERR("Invalid digest length.");
        return EINVAL;
    }

    // Convert the expected hex string to binary bytes
    uint8_t expected_hash_bytes[SHA256_BYTES_LEN];
    size_t ret = hex2bin(
        expected_hex, SHA256_HEX_STR_LEN, expected_hash_bytes, sizeof(expected_hash_bytes));
    if (ret != sizeof(expected_hash_bytes)) {
        EDGEHOG_LOG_ERR("Failed to parse expected digest hex string %d.", ret);
        return EINVAL;
    }

    // Let the PSA cryptography API securely verify the digest
    psa_status_t status
        = psa_hash_verify(&data->hash_operation, expected_hash_bytes, sizeof(expected_hash_bytes));
    if (status == PSA_ERROR_INVALID_SIGNATURE) {
        EDGEHOG_LOG_ERR("Digest mismatch.");
        return EINVAL;
    }
    if (status != PSA_SUCCESS) {
        EDGEHOG_LOG_ERR("PSA Hash Verify failed with internal error: %d", status);
        return EIO;
    }
    return 0;
}

const edgehog_ft_file_write_cbks_t *get_callbacks(const char *destination_type)
{
    const edgehog_ft_file_write_cbks_t *file_cbks = NULL;
    if (strcmp(destination_type, "stream") == 0) {
        file_cbks = &edgehog_ft_stream_write_cbks;
    } else if (strcmp(destination_type, "filesystem") == 0) {
        file_cbks = &edgehog_ft_filesystem_write_cbks;
    }
    return file_cbks;
}
