/*
 * (C) Copyright 2026, SECO Mind Srl
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "file_transfer/utils.h"

#include "edgehog_private.h"
#include "generated_interfaces.h"
#include "log.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

EDGEHOG_LOG_MODULE_REGISTER(file_transfer_utils, CONFIG_EDGEHOG_DEVICE_FILE_TRANSFER_LOG_LEVEL);

/************************************************
 *        Defines, constants and typedef        *
 ***********************************************/

// Endpoint strings used for communication with Astarte.
#define COMMON_ENDPOINT_REQUEST "/request"
#define ENDPOINT_ID "id"
#define ENDPOINT_URL "url"
#define ENDPOINT_HTTP_HEADER_KEYS "httpHeaderKeys"
#define ENDPOINT_HTTP_HEADER_VALUES "httpHeaderValues"
#define ENDPOINT_ENCODING "encoding"
#define ENDPOINT_PROGRESS "progress"
#define ENDPOINT_DIGEST "digest"
#define ENDPOINT_FILE_SIZE_BYTES "fileSizeBytes"
#define ENDPOINT_DESTINATION_TYPE "destinationType"
#define ENDPOINT_SOURCE_TYPE "sourceType"
#define ENDPOINT_DESTINATION "destination"
#define ENDPOINT_SOURCE "source"
#define ENDPOINT_TYPE "type"
#define ENDPOINT_CODE "code"
#define ENDPOINT_MSG "message"
#define ENDPOINT_BYTES "bytes"
#define ENDPOINT_TOTAL_BYTES "totalBytes"

#define CONTENT_TYPE_SERVER_TO_DEVICE "server_to_device"
#define CONTENT_TYPE_DEVICE_TO_SERVER "device_to_server"

#define PROGRESS_REPORT_INTERVAL_PERCENT 5
#define PROGRESS_REPORT_INTERVAL_BYTES (256 * 1024)
#define PROGRESS_ONE_HUNDRED_PERCENT 100

/**
 * @brief Temporary structure to hold parsed HTTP headers.
 * @details Used during the parsing of Astarte endpoints to keep track of the HTTP header keys,
 * values, and their respective lengths before they are serialized into a single array.
 */
typedef struct
{
    /** @brief Array of HTTP header key strings. */
    const char **keys;
    /** @brief Number of elements in the keys array. */
    size_t keys_len;
    /** @brief Array of HTTP header value strings. */
    const char **values;
    /** @brief Number of elements in the values array. */
    size_t values_len;
} parsed_http_headers_t;

/************************************************
 *         Static functions declarations        *
 ***********************************************/

static void parse_endpoint_value(const char *path, const astarte_data_t *rx_value,
    edgehog_ft_msg_t *tmp, parsed_http_headers_t *headers);
static char **serialize_http_headers(
    const char *keys[], size_t keys_size, const char *values[], size_t values_size);
static enum edgehog_ft_encoding parse_encoding_string(const char *string);
static void free_http_headers(char *header_fields[]);
static void progress_work_handler(struct k_work *work);
static char *duplicate_string(const char *src);

/************************************************
 *         Global functions definitions         *
 ***********************************************/

edgehog_result_t edgehog_ft_msg_init(astarte_object_entry_t *rx_values, size_t rx_values_size,
    edgehog_ft_type_t type, edgehog_ft_msg_t *msg)
{
    edgehog_result_t eres = EDGEHOG_RESULT_OK;
    edgehog_ft_msg_t tmp = { 0 };

    parsed_http_headers_t parsed_http_headers = { 0 };

    tmp.type = type;

    for (size_t i = 0; i < rx_values_size; i++) {
        parse_endpoint_value(rx_values[i].path, &rx_values[i].data, &tmp, &parsed_http_headers);
    }

    if (!tmp.id || !tmp.url || !tmp.location_type || !tmp.location) {
        EDGEHOG_LOG_ERR("Missing required entries for transfer data");
        eres = EDGEHOG_RESULT_FILE_TRANSFER_INVALID_REQUEST;
        goto failure;
    }

    if ((type == EDGEHOG_FT_TYPE_SERVER_TO_DEVICE) && !tmp.digest) {
        EDGEHOG_LOG_ERR("Missing required digest for transfer data");
        eres = EDGEHOG_RESULT_FILE_TRANSFER_INVALID_REQUEST;
        goto failure;
    }

    if (parsed_http_headers.keys && parsed_http_headers.values) {
        tmp.http_headers
            = serialize_http_headers(parsed_http_headers.keys, parsed_http_headers.keys_len,
                parsed_http_headers.values, parsed_http_headers.values_len);
        if (!tmp.http_headers) {
            EDGEHOG_LOG_ERR("Serialization of the HTTP headers failed");
            eres = EDGEHOG_RESULT_FILE_TRANSFER_INVALID_REQUEST;
            goto failure;
        }
    }

    if ((tmp.encoding != EDGEHOG_FT_ENCODING_NONE)
#ifdef CONFIG_EDGEHOG_DEVICE_FILE_TRANSFER_COMPRESSION
        && (tmp.encoding != EDGEHOG_FT_ENCODING_LZ4)
#endif
    ) {
        EDGEHOG_LOG_ERR("Request with invalid encoding %d", tmp.encoding);
        eres = EDGEHOG_RESULT_FILE_TRANSFER_INVALID_REQUEST;
        goto failure;
    }

    *msg = tmp;
    return eres;

failure:
    edgehog_ft_msg_destroy(&tmp);
    return eres;
}

void edgehog_ft_msg_destroy(edgehog_ft_msg_t *msg)
{
    k_free(msg->id);
    msg->id = NULL;
    k_free(msg->url);
    msg->url = NULL;
    k_free(msg->digest);
    msg->digest = NULL;
    if (msg->http_headers) {
        free_http_headers(msg->http_headers);
        msg->http_headers = NULL;
    }
    k_free(msg->location_type);
    msg->location_type = NULL;
    k_free(msg->location);
    msg->location = NULL;
}

edgehog_ft_http_cbk_data_t *edgehog_ft_http_cbk_data_new(edgehog_device_handle_t edgehog_device,
    edgehog_ft_msg_t *msg, const void *file_cbks, void *file_cbks_ctx)
{
    edgehog_ft_http_cbk_data_t *data = k_calloc(1, sizeof(edgehog_ft_http_cbk_data_t));
    if (!data) {
        return NULL;
    }

    data->edgehog_device = edgehog_device;
    data->id = msg->id;
    data->progress = msg->progress;
    data->type = msg->type;
    data->encoding = msg->encoding;
    data->file_cbks = file_cbks;
    data->file_cbks_ctx = file_cbks_ctx;
    data->transferred_bytes = 0;
    data->total_bytes = msg->file_size_bytes;
    data->last_reported_bytes = ATOMIC_INIT(0);

    k_work_init(&data->progress_work, progress_work_handler);
    return data;
}

void edgehog_ft_http_cbk_data_destroy(edgehog_ft_http_cbk_data_t *data)
{
    if (data) {
        struct k_work_sync sync;
        k_work_cancel_sync(&data->progress_work, &sync);
    }
    k_free(data);
}

void edgehog_ft_update_progress(
    edgehog_ft_http_cbk_data_t *data, size_t chunk_size, bool last_chunk)
{
    if (!data->progress) {
        return;
    }

    size_t last_reported_bytes = atomic_get(&data->last_reported_bytes);
    data->transferred_bytes += chunk_size;
    bool should_report = false;

    if (data->total_bytes > 0) {
        size_t current_percent
            = (data->transferred_bytes * PROGRESS_ONE_HUNDRED_PERCENT) / data->total_bytes;
        size_t last_percent
            = (last_reported_bytes * PROGRESS_ONE_HUNDRED_PERCENT) / data->total_bytes;

        if (current_percent >= last_percent + PROGRESS_REPORT_INTERVAL_PERCENT || last_chunk) {
            should_report = true;
        }
    } else {
        if (data->transferred_bytes >= last_reported_bytes + PROGRESS_REPORT_INTERVAL_BYTES
            || last_chunk) {
            should_report = true;
        }
    }

    if (should_report) {
        atomic_set(&data->last_reported_bytes, (atomic_val_t) data->transferred_bytes);
        k_work_submit(&data->progress_work);
    }
}

void edgehog_ft_send_response(edgehog_device_handle_t device, const char *identifier,
    edgehog_ft_type_t type, int in_errno, const char *in_msg, edgehog_result_t eres)
{
    if (!device || !identifier) {
        EDGEHOG_LOG_ERR("Invalid parameters to send file transfer response");
        return;
    }

    int posix_errno = in_errno;
    const char *message = in_msg;

    if (posix_errno == 0) {
        switch (eres) {
            case EDGEHOG_RESULT_OK:
                if (!message) {
                    message = "File transfer completed successfully.";
                }
                break;
            case EDGEHOG_RESULT_HTTP_REQUEST_INVALID_HEADERS:
                posix_errno = EINVAL;
                message = "Invalid HTTP headers.";
                break;
            case EDGEHOG_RESULT_PARSE_URL_ERROR:
                posix_errno = EINVAL;
                message = "Couldn't parse HTTP URL.";
                break;
            case EDGEHOG_RESULT_NETWORK_ERROR:
                posix_errno = ECONNREFUSED;
                message = "Couldn't create or connect to socket.";
                break;
            case EDGEHOG_RESULT_HTTP_REQUEST_ERROR:
                posix_errno = EPROTO;
                message = "HTTP request failed.";
                break;
            case EDGEHOG_RESULT_OUT_OF_MEMORY:
                posix_errno = ENOSR;
                message = "Out of memory in HTTP request.";
                break;
            default:
                posix_errno = EPIPE;
                message = "Internal file transfer error.";
                break;
        }
    }

    // Safety check to ensure message is never NULL.
    if (!message) {
        message = "An unknown error occurred during file transfer.";
    }

    // Log the error if errno is set or if the result is not OK
    if (posix_errno != 0) {
        EDGEHOG_LOG_ERR("File transfer failed with error: %s", message);
    } else {
        EDGEHOG_LOG_INF("%s", message);
    }

    const char *type_str = (type == EDGEHOG_FT_TYPE_SERVER_TO_DEVICE)
        ? CONTENT_TYPE_SERVER_TO_DEVICE
        : CONTENT_TYPE_DEVICE_TO_SERVER;

    astarte_object_entry_t object_entries[] = {
        { .path = ENDPOINT_ID, .data = astarte_data_from_string(identifier) },
        { .path = ENDPOINT_TYPE, .data = astarte_data_from_string(type_str) },
        { .path = ENDPOINT_CODE, .data = astarte_data_from_longinteger(posix_errno) },
        { .path = ENDPOINT_MSG, .data = astarte_data_from_string(message) },
    };

    astarte_result_t ares = astarte_device_send_object(device->astarte_device,
        io_edgehog_devicemanager_fileTransfer_Response.name, COMMON_ENDPOINT_REQUEST,
        object_entries, ARRAY_SIZE(object_entries), NULL);

    if (ares != ASTARTE_RESULT_OK) {
        EDGEHOG_LOG_ERR("Unable to send file transfer response: %s.", astarte_result_to_name(ares));
    }
}

/************************************************
 *         Static functions definitions         *
 ***********************************************/

static void parse_endpoint_value(const char *path, const astarte_data_t *rx_value,
    edgehog_ft_msg_t *tmp, parsed_http_headers_t *headers)
{
    const char *tmp_string = NULL;
    bool tmp_bool = false;
    int64_t tmp_long = 0;

    bool is_ser_to_dev = (tmp->type == EDGEHOG_FT_TYPE_SERVER_TO_DEVICE);
    const char *loc_type_key = is_ser_to_dev ? ENDPOINT_DESTINATION_TYPE : ENDPOINT_SOURCE_TYPE;
    const char *loc_key = is_ser_to_dev ? ENDPOINT_DESTINATION : ENDPOINT_SOURCE;

    if (strcmp(path, ENDPOINT_ID) == 0
        && astarte_data_to_string(*rx_value, &tmp_string) == ASTARTE_RESULT_OK) {
        tmp->id = duplicate_string(tmp_string);
        return;
    }
    if (strcmp(path, ENDPOINT_URL) == 0
        && astarte_data_to_string(*rx_value, &tmp_string) == ASTARTE_RESULT_OK) {
        tmp->url = duplicate_string(tmp_string);
        return;
    }
    if (strcmp(path, ENDPOINT_HTTP_HEADER_KEYS) == 0) {
        astarte_data_to_string_array(*rx_value, &headers->keys, &headers->keys_len);
        return;
    }
    if (strcmp(path, ENDPOINT_HTTP_HEADER_VALUES) == 0) {
        astarte_data_to_string_array(*rx_value, &headers->values, &headers->values_len);
        return;
    }
    if (strcmp(path, ENDPOINT_ENCODING) == 0
        && astarte_data_to_string(*rx_value, &tmp_string) == ASTARTE_RESULT_OK) {
        tmp->encoding = parse_encoding_string(tmp_string);
        return;
    }
    if (strcmp(path, ENDPOINT_PROGRESS) == 0
        && astarte_data_to_boolean(*rx_value, &tmp_bool) == ASTARTE_RESULT_OK) {
        tmp->progress = tmp_bool;
        return;
    }
    if (strcmp(path, ENDPOINT_DIGEST) == 0
        && astarte_data_to_string(*rx_value, &tmp_string) == ASTARTE_RESULT_OK) {
        tmp->digest = duplicate_string(tmp_string);
        return;
    }
    if (is_ser_to_dev && strcmp(path, ENDPOINT_FILE_SIZE_BYTES) == 0
        && astarte_data_to_longinteger(*rx_value, &tmp_long) == ASTARTE_RESULT_OK) {
        tmp->file_size_bytes = tmp_long;
        return;
    }
    if (strcmp(path, loc_type_key) == 0
        && astarte_data_to_string(*rx_value, &tmp_string) == ASTARTE_RESULT_OK) {
        tmp->location_type = duplicate_string(tmp_string);
        return;
    }
    if (strcmp(path, loc_key) == 0
        && astarte_data_to_string(*rx_value, &tmp_string) == ASTARTE_RESULT_OK) {
        tmp->location = duplicate_string(tmp_string);
        return;
    }
}

static char **serialize_http_headers(
    const char *keys[], size_t keys_size, const char *values[], size_t values_size)
{
    if (!keys || !values) {
        EDGEHOG_LOG_WRN("Serializing HTTP headers without providing the appropriate buffers");
        return NULL;
    }
    if (keys_size != values_size) {
        EDGEHOG_LOG_WRN("Serializing HTTP headers with different sizes");
        return NULL;
    }
    size_t num_headers = values_size;

    // Dynamically allocate the array of string pointers (+1 for the NULL terminator)
    char **out = (char **) k_calloc(num_headers + 1, sizeof(char *));
    if (!out) {
        EDGEHOG_LOG_WRN("Failed to allocate memory for HTTP headers array");
        return NULL;
    }

    // If empty array
    if (num_headers == 0) {
        out[0] = NULL;
        return out;
    }

    for (size_t i = 0; i < num_headers; i++) {
        if (!keys[i] || !values[i]) {
            EDGEHOG_LOG_WRN("Null pointer encountered in header keys or values");
            free_http_headers(out);
            return NULL;
        }

        // Calculate required output string size (key + value + ": " + "\r\n" + NULL)
        size_t needed = strlen(keys[i]) + strlen(values[i]) + sizeof(": \r\n");
        out[i] = k_calloc(needed, sizeof(char));
        if (!out[i]) {
            EDGEHOG_LOG_WRN("Failed to allocate memory for file transfer http headers");
            free_http_headers(out);
            return NULL;
        }

        // NOLINTNEXTLINE(cert-err33-c)
        snprintf(out[i], needed, "%s: %s\r\n", keys[i], values[i]);
    }

    out[num_headers] = NULL;
    return out;
}

static enum edgehog_ft_encoding parse_encoding_string(const char *string)
{
    if (strlen(string) == 0) {
        return EDGEHOG_FT_ENCODING_NONE;
    }
    if (strcmp(string, "lz4") == 0) {
        return EDGEHOG_FT_ENCODING_LZ4;
    }
    if (strcmp(string, "tar") == 0) {
        return EDGEHOG_FT_ENCODING_TAR;
    }
    if (strcmp(string, "tar.lz4") == 0) {
        return EDGEHOG_FT_ENCODING_TAR_LZ4;
    }
    return EDGEHOG_FT_ENCODING_UNSUPPORTED;
}

static void free_http_headers(char *header_fields[])
{
    if (header_fields) {
        for (size_t i = 0; header_fields[i] != NULL; i++) {
            k_free(header_fields[i]);
        }
    }
    k_free((void *) header_fields);
}

static void progress_work_handler(struct k_work *work)
{
    edgehog_ft_http_cbk_data_t *data
        = CONTAINER_OF(work, edgehog_ft_http_cbk_data_t, progress_work);

    int64_t bytes = (int64_t) atomic_get(&data->last_reported_bytes);
    int64_t total_bytes = (int64_t) data->total_bytes;

    const char *type_str = (data->type == EDGEHOG_FT_TYPE_SERVER_TO_DEVICE)
        ? CONTENT_TYPE_SERVER_TO_DEVICE
        : CONTENT_TYPE_DEVICE_TO_SERVER;

    astarte_object_entry_t object_entries[] = {
        { .path = ENDPOINT_ID, .data = astarte_data_from_string(data->id) },
        { .path = ENDPOINT_TYPE, .data = astarte_data_from_string(type_str) },
        { .path = ENDPOINT_BYTES, .data = astarte_data_from_longinteger(bytes) },
        { .path = ENDPOINT_TOTAL_BYTES, .data = astarte_data_from_longinteger(total_bytes) },
    };

    astarte_result_t ares = astarte_device_send_object(data->edgehog_device->astarte_device,
        io_edgehog_devicemanager_fileTransfer_Progress.name, COMMON_ENDPOINT_REQUEST,
        object_entries, ARRAY_SIZE(object_entries), NULL);
    if (ares != ASTARTE_RESULT_OK) {
        EDGEHOG_LOG_ERR("Unable to send file transfer progress");
        return;
    }

    EDGEHOG_LOG_INF(
        "File transfer ID %s progress: %lld / %lld bytes", data->id, bytes, total_bytes);
}

static char *duplicate_string(const char *src)
{
    if (!src) {
        return NULL;
    }

    // length including the null terminator
    size_t len = strlen(src) + 1;
    char *dest = k_malloc(len);

    if (dest) {
        memcpy(dest, src, len);
    }

    return dest;
}
