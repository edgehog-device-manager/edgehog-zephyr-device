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
 *         Static functions declarations        *
 ***********************************************/

static char **edgehog_ft_serialize_http_headers(
    const char *keys[], size_t keys_size, const char *values[], size_t values_size);
static void edgehog_ft_free_http_headers(char *header_fields[]);
static char *edgehog_ft_duplicate_string(const char *src);

/************************************************
 *         Global functions definitions         *
 ***********************************************/

edgehog_result_t edgehog_ft_msg_init(astarte_object_entry_t *rx_values, size_t rx_values_size,
    edgehog_ft_msg_type_t type, edgehog_ft_msg_t *msg)
{
    edgehog_result_t eres = EDGEHOG_RESULT_OK;
    edgehog_ft_msg_t tmp = { 0 };
    bool is_server_to_device = (type == EDGEHOG_FT_MSG_SERVER_TO_DEVICE);

    size_t http_header_keys_len = 0;
    const char **http_header_keys = NULL;
    size_t http_header_values_len = 0;
    const char **http_header_values = NULL;

    // Store the type
    tmp.type = type;

    // Fill all the other fields
    for (size_t i = 0; i < rx_values_size; i++) {
        const char *path = rx_values[i].path;
        astarte_data_t rx_value = rx_values[i].data;

        if (strcmp(path, "id") == 0) {
            tmp.id = edgehog_ft_duplicate_string((char *) rx_value.data.string);
        } else if (strcmp(path, "url") == 0) {
            tmp.url = edgehog_ft_duplicate_string((char *) rx_value.data.string);
        } else if (strcmp(path, "httpHeaderKeys") == 0) {
            http_header_keys_len = rx_value.data.string_array.len;
            http_header_keys = rx_value.data.string_array.buf;
        } else if (strcmp(path, "httpHeaderValues") == 0) {
            http_header_values_len = rx_value.data.string_array.len;
            http_header_values = rx_value.data.string_array.buf;
        } else if (strcmp(path, "progress") == 0) {
            tmp.progress = rx_value.data.boolean;
        } else if (is_server_to_device && strcmp(path, "fileSizeBytes") == 0) {
            tmp.file_size_bytes = rx_value.data.longinteger;
        } else if ((is_server_to_device && strcmp(path, "destinationType") == 0)
            || (!is_server_to_device && strcmp(path, "sourceType") == 0)) {
            tmp.location_type = edgehog_ft_duplicate_string((char *) rx_value.data.string);
        } else if ((tmp.type == EDGEHOG_FT_MSG_SERVER_TO_DEVICE && strcmp(path, "destination") == 0)
            || (tmp.type == EDGEHOG_FT_MSG_DEVICE_TO_SERVER && strcmp(path, "source") == 0)) {
            tmp.location = edgehog_ft_duplicate_string((char *) rx_value.data.string);
        }
    }

    // Check all the required entries where present
    if (!tmp.id || !tmp.url || !tmp.location_type || !tmp.location) {
        EDGEHOG_LOG_ERR("Missing required entries for transfer data");
        eres = EDGEHOG_RESULT_FILE_TRANSFER_INVALID_REQUEST;
        goto failure;
    }

    if (http_header_keys && http_header_values) {
        tmp.http_headers = edgehog_ft_serialize_http_headers(
            http_header_keys, http_header_keys_len, http_header_values, http_header_values_len);
        if (!tmp.http_headers) {
            EDGEHOG_LOG_ERR("Serialization of the HTTP headers failed");
            eres = EDGEHOG_RESULT_FILE_TRANSFER_INVALID_REQUEST;
            goto failure;
        }
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
    if (msg->http_headers) {
        edgehog_ft_free_http_headers(msg->http_headers);
        msg->http_headers = NULL;
    }
    k_free(msg->location_type);
    msg->location_type = NULL;
    k_free(msg->location);
    msg->location = NULL;
}

void edgehog_ft_progress_work_handler(struct k_work *work)
{
    edgehog_ft_http_cbk_data_t *data
        = CONTAINER_OF(work, edgehog_ft_http_cbk_data_t, progress_work);

    int32_t progress = (int32_t) atomic_get(&data->current_progress);

    const char *type_str
        = (data->type == EDGEHOG_FT_MSG_SERVER_TO_DEVICE) ? "server_to_device" : "device_to_server";

    astarte_object_entry_t object_entries[] = {
        { .path = "id", .data = astarte_data_from_string(data->id) },
        { .path = "type", .data = astarte_data_from_string(type_str) },
        { .path = "progress", .data = astarte_data_from_integer(progress) },
    };

    astarte_result_t ares = astarte_device_send_object(data->edgehog_device->astarte_device,
        io_edgehog_devicemanager_fileTransfer_Progress.name, "/request", object_entries,
        ARRAY_SIZE(object_entries), NULL);
    if (ares != ASTARTE_RESULT_OK) {
        EDGEHOG_LOG_ERR("Unable to send file transfer progress");
        return;
    }

    EDGEHOG_LOG_INF("File transfer ID %s progress: %d/100", data->id, progress);
}

void edgehog_ft_send_response(edgehog_device_handle_t device, const char *identifier,
    const char *type, int in_errno, const char *in_msg, edgehog_result_t eres)
{
    if (!device || !identifier || !type) {
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

    astarte_object_entry_t object_entries[] = {
        { .path = "id", .data = astarte_data_from_string(identifier) },
        { .path = "type", .data = astarte_data_from_string(type) },
        { .path = "code", .data = astarte_data_from_longinteger(posix_errno) },
        { .path = "message", .data = astarte_data_from_string(message) },
    };

    astarte_result_t ares = astarte_device_send_object(device->astarte_device,
        io_edgehog_devicemanager_fileTransfer_Response.name, "/request", object_entries,
        ARRAY_SIZE(object_entries), NULL);

    if (ares != ASTARTE_RESULT_OK) {
        EDGEHOG_LOG_ERR("Unable to send file transfer response: %s.", astarte_result_to_name(ares));
    }
}

/************************************************
 *         Static functions definitions         *
 ***********************************************/

static char **edgehog_ft_serialize_http_headers(
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
    char **out = (char **) k_malloc((num_headers + 1) * sizeof(char *));
    if (!out) {
        EDGEHOG_LOG_WRN("Failed to allocate memory for HTTP headers array");
        return NULL;
    }
    memset((void *) out, 0, (num_headers + 1) * sizeof(char *));

    // If empty array
    if (num_headers == 0) {
        out[0] = NULL;
        return out;
    }

    for (size_t i = 0; i < num_headers; i++) {
        if (!keys[i] || !values[i]) {
            EDGEHOG_LOG_WRN("Null pointer encountered in header keys or values");
            edgehog_ft_free_http_headers(out);
            return NULL;
        }

        // Calculate required output string size (key + value + ": " + "\r\n" + NULL)
        size_t needed = strlen(keys[i]) + strlen(values[i]) + sizeof(": \r\n");
        out[i] = k_malloc(needed);
        if (!out[i]) {
            EDGEHOG_LOG_WRN("Failed to allocate memory for file transfer http headers");
            edgehog_ft_free_http_headers(out);
            return NULL;
        }

        // NOLINTNEXTLINE(cert-err33-c)
        snprintf(out[i], needed, "%s: %s\r\n", keys[i], values[i]);
    }

    out[num_headers] = NULL;
    return out;
}

static void edgehog_ft_free_http_headers(char *header_fields[])
{
    if (header_fields) {
        for (size_t i = 0; header_fields[i] != NULL; i++) {
            k_free(header_fields[i]);
        }
    }
    k_free((void *) header_fields);
}

static char *edgehog_ft_duplicate_string(const char *src)
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
