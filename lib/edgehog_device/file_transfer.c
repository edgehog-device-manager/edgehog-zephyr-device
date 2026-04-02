/*
 * (C) Copyright 2026, SECO Mind Srl
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "file_transfer_private.h"

#include "edgehog_private.h"
#include "generated_interfaces.h"
#include "hardware_info.h"
#include "http.h"
#include "settings.h"
#include "storage_usage.h"
#include "system_status.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <astarte_device_sdk/device.h>

#include "log.h"

EDGEHOG_LOG_MODULE_REGISTER(file_transfer, CONFIG_EDGEHOG_DEVICE_FILE_TRANSFER_LOG_LEVEL);

/************************************************
 *        Defines, constants and typedef        *
 ***********************************************/

#define FILE_TRANSFER_SERVICE_THREAD_STACK_SIZE 8192
#define FILE_TRANSFER_SERVICE_THREAD_PRIORITY 5
#define FILE_TRANSFER_SERVICE_THREAD_RUNNING_BIT (1)
#define FILE_TRANSFER_SERVICE_MSGQ_GET_TIMEOUT 100
#define FILE_TRANSFER_MAX_HTTP_HEADERS 15
#define FILE_TRANSFER_PERCENTAGE 100

#define FT_REQ_TIMEOUT_MS (60 * 1000)

// NOLINTBEGIN(cppcoreguidelines-avoid-non-const-global-variables)
K_THREAD_STACK_DEFINE(file_transfer_service_stack_area, FILE_TRANSFER_SERVICE_THREAD_STACK_SIZE);
// NOLINTEND(cppcoreguidelines-avoid-non-const-global-variables)

/************************************************
 *         Static functions declarations        *
 ***********************************************/

static edgehog_result_t parse_http_headers(
    const char *header_key, const char *header_value, char *header_fields[], size_t max_elements);

static void free_http_headers(char *header_fields[], size_t max_elements);

static edgehog_result_t ft_handle_server_to_device(
    edgehog_device_handle_t edgehog_device, ft_msgq_data_t *msg);

static void ft_handle_device_to_server(edgehog_device_handle_t edgehog_device, ft_msgq_data_t *msg);

// entry point for the thread handling the file transfer operations
static void ft_service_thread_entry_point(void *device_ptr, void *queue_ptr, void *unused);

// Equivalent of POSIX strdup() funciton
static char *duplicate_string(const char *src);

/************************************************
 *     Callbacks definition and declaration     *
 ***********************************************/

static edgehog_result_t http_download_ft_std_payload_cbk(
    bool *abort_flag, http_download_chunk_t *download_chunk, void *user_data)
{
    if (!download_chunk) {
        EDGEHOG_LOG_ERR("Unable to read chunk, It is empty");
        return EDGEHOG_RESULT_HTTP_REQUEST_ERROR;
    }

    if (!user_data) {
        EDGEHOG_LOG_ERR("Unable to read user data context");
        return EDGEHOG_RESULT_INTERNAL_ERROR;
    }

    ft_server_to_device_http_cb_data_t *ft_data = (ft_server_to_device_http_cb_data_t *) user_data;

    // store the file
    if (download_chunk->chunk_size > 0 && ft_data->file != NULL) {
        // check potential buffer overflow
        if (ft_data->current_offset + download_chunk->chunk_size <= ft_data->file_size_bytes) {

            memcpy(ft_data->file + ft_data->current_offset, download_chunk->chunk_start_addr,
                download_chunk->chunk_size);

            ft_data->current_offset += download_chunk->chunk_size;
            EDGEHOG_LOG_DBG("Downloaded %d bytes", ft_data->current_offset);
        } else {
            EDGEHOG_LOG_ERR("Potential buffer overflow, the file exceed declared dimensions");
            edgehog_http_download_abort(abort_flag);
            return EDGEHOG_RESULT_INTERNAL_ERROR;
        }
    }

    if (ft_data->progress) {
        int32_t progress
            = (int32_t) (((uint64_t) ft_data->current_offset * FILE_TRANSFER_PERCENTAGE)
                / ft_data->file_size_bytes);

        astarte_object_entry_t object_entries[] = {
            { .path = "id", .data = astarte_data_from_string(ft_data->id) },
            { .path = "type", .data = astarte_data_from_string("server_to_device") },
            { .path = "progress", .data = astarte_data_from_integer(progress) },
        };

        astarte_result_t res = astarte_device_send_object(ft_data->edgehog_device->astarte_device,
            io_edgehog_devicemanager_fileTransfer_Progress.name, "/request", object_entries,
            ARRAY_SIZE(object_entries), NULL);

        if (res != ASTARTE_RESULT_OK) {
            EDGEHOG_LOG_ERR("Unable to send File transfer progress");
        }

        EDGEHOG_LOG_INF("File transfer ID %s progress: %d/100", ft_data->id, progress);
    }

    if (download_chunk->last_chunk) {
        if (ft_data->file_size_bytes != ft_data->current_offset) {
            EDGEHOG_LOG_ERR("File transfer download aborted");
            edgehog_http_download_abort(abort_flag);
            return EDGEHOG_RESULT_INTERNAL_ERROR;
        }

        EDGEHOG_LOG_DBG("Download completed successfully!");
        // TODO: close here the file

        astarte_object_entry_t object_entries[] = {
            { .path = "id", .data = astarte_data_from_string(ft_data->id) },
            { .path = "type", .data = astarte_data_from_string("server_to_device") },
            { .path = "code", .data = astarte_data_from_longinteger(0) },
            { .path = "message", .data = astarte_data_from_string("") },
        };

        astarte_result_t res = astarte_device_send_object(ft_data->edgehog_device->astarte_device,
            io_edgehog_devicemanager_fileTransfer_Response.name, "/request", object_entries,
            ARRAY_SIZE(object_entries), NULL);

        if (res != ASTARTE_RESULT_OK) {
            EDGEHOG_LOG_ERR("Unable to send File transfer response");
            edgehog_http_download_abort(abort_flag);
            return EDGEHOG_RESULT_INTERNAL_ERROR;
        }
    }

    return EDGEHOG_RESULT_OK;
}

/************************************************
 *         Global functions definitions         *
 ***********************************************/

/**
 * @brief Create an Edgehog file transfer service.
 *
 * @return A pointer to Edgehog telemetry or a NULL if an error occurred.
 */
edgehog_file_transfer_t *edgehog_ft_new()
{
    // Allocate space for the file transfer internal struct
    edgehog_file_transfer_t *file_transfer = calloc(1, sizeof(edgehog_file_transfer_t));
    if (!file_transfer) {
        EDGEHOG_LOG_ERR("Out of memory %s: %d", __FILE__, __LINE__);
        return NULL;
    }

    // TODO: add config operations

    return file_transfer;
}

edgehog_result_t edgehog_ft_start(edgehog_device_handle_t device)
{
    edgehog_file_transfer_t *file_tansfer = device->file_transfer;

    if (!file_tansfer) {
        EDGEHOG_LOG_ERR("Unable to start file transfer, reference is null");
        return EDGEHOG_RESULT_FILE_TRANSFER_START_FAIL;
    }

    if (atomic_test_and_set_bit(
            &file_tansfer->thread_state, FILE_TRANSFER_SERVICE_THREAD_RUNNING_BIT)) {
        EDGEHOG_LOG_ERR("Failed starting file transfer service as it's already running");
        return EDGEHOG_RESULT_FILE_TRANSFER_START_FAIL;
    }

    k_msgq_init(&file_tansfer->msgq, file_tansfer->msgq_buffer, sizeof(ft_msgq_data_t),
        EDGEHOG_FILE_TRANSFER_LEN);

    k_tid_t thread_id = k_thread_create(&file_tansfer->thread, file_transfer_service_stack_area,
        FILE_TRANSFER_SERVICE_THREAD_STACK_SIZE, ft_service_thread_entry_point, (void *) device,
        (void *) &file_tansfer->msgq, NULL, FILE_TRANSFER_SERVICE_THREAD_PRIORITY, 0, K_NO_WAIT);

    if (!thread_id) {
        EDGEHOG_LOG_ERR("Unable to start file transfer message thread");
        atomic_clear_bit(&file_tansfer->thread_state, FILE_TRANSFER_SERVICE_THREAD_RUNNING_BIT);
        return EDGEHOG_RESULT_FILE_TRANSFER_START_FAIL;
    }

    // assign a name to the thread for debugging purposes
    int ret = k_thread_name_set(thread_id, "file_transfer");
    if (ret == -ENOSYS) {
        EDGEHOG_LOG_DBG(
            "Couldn't set thread name since the config CONFIG_THREAD_NAME is not enabled");
    } else if (ret != 0) {
        EDGEHOG_LOG_ERR("Failed to set thread name, error %d", ret);
        atomic_clear_bit(&file_tansfer->thread_state, FILE_TRANSFER_SERVICE_THREAD_RUNNING_BIT);
        return EDGEHOG_RESULT_FILE_TRANSFER_START_FAIL;
    }

    return EDGEHOG_RESULT_OK;
}

edgehog_result_t edgehog_ft_server_to_device_event(
    edgehog_device_handle_t device, astarte_device_datastream_object_event_t *object_event)
{
    edgehog_result_t res = EDGEHOG_RESULT_FILE_TRANSFER_INVALID_REQUEST;

    char *ft_id = NULL;
    char *ft_url = NULL;
    char *ft_http_header_key = NULL;
    char *ft_http_header_value = NULL;
    bool ft_progress = false;
    int64_t ft_file_size_bytes = -1;

    if (!object_event) {
        EDGEHOG_LOG_ERR(
            "Unable to handle fdile transfer server to device event, object event undefined");
        goto failure;
    }

    astarte_object_entry_t *rx_values = object_event->entries;
    size_t rx_values_length = object_event->entries_len;

    for (size_t i = 0; i < rx_values_length; i++) {
        const char *path = rx_values[i].path;
        astarte_data_t rx_value = rx_values[i].data;

        if (strcmp(path, "id") == 0) {
            ft_id = duplicate_string((char *) rx_value.data.string);
            EDGEHOG_LOG_INF("id: %s", ft_id ? ft_id : "NULL");
        } else if (strcmp(path, "url") == 0) {
            ft_url = duplicate_string((char *) rx_value.data.string);
            EDGEHOG_LOG_INF("url: %s", ft_url ? ft_url : "NULL");
        } else if (strcmp(path, "httpHeaderKey") == 0) {
            ft_http_header_key = duplicate_string((char *) rx_value.data.string);
            EDGEHOG_LOG_INF("httpHeaderKey: %s", ft_http_header_key ? ft_http_header_key : "NULL");
        } else if (strcmp(path, "httpHeaderValue") == 0) {
            ft_http_header_value = duplicate_string((char *) rx_value.data.string);
            EDGEHOG_LOG_INF(
                "httpHeaderValue: %s", ft_http_header_value ? ft_http_header_value : "NULL");
        } else if (strcmp(path, "fileSizeBytes") == 0) {
            ft_file_size_bytes = rx_value.data.longinteger;
            EDGEHOG_LOG_INF("fileSizeBytes: %lld", ft_file_size_bytes);
        } else if (strcmp(path, "progress") == 0) {
            ft_progress = rx_value.data.boolean;
            EDGEHOG_LOG_INF("progress: %d", ft_progress);
        }
    }

    if (!ft_id || !ft_url || !ft_http_header_key || !ft_http_header_value || !ft_file_size_bytes) {
        EDGEHOG_LOG_ERR("Unable to extract data from request");
        res = EDGEHOG_RESULT_FILE_TRANSFER_INVALID_REQUEST;
        goto failure;
    }

    ft_server_to_device_data_t data = {
        .id = ft_id,
        .url = ft_url,
        .http_header_key = ft_http_header_key,
        .http_header_value = ft_http_header_value,
        .file_size_bytes = ft_file_size_bytes,
        .progress = ft_progress,
    };

    ft_msgq_data_t msg = { 0 };
    msg.type = FT_MSG_SERVER_TO_DEVICE;
    msg.payload.server_to_device = data;

    if (k_msgq_put(&device->file_transfer->msgq, &msg, K_NO_WAIT) != 0) {
        EDGEHOG_LOG_ERR("Unable to send file transfer data to the task handling it");
        res = EDGEHOG_RESULT_FILE_TRANSFER_QUEUE_ERROR;
        goto failure;
    }

    return EDGEHOG_RESULT_OK;

failure:

    k_free(ft_id);
    k_free(ft_url);
    k_free(ft_http_header_key);
    k_free(ft_http_header_value);

    return res;
}

edgehog_result_t edgehog_ft_device_to_server_event(
    edgehog_device_handle_t device, astarte_device_datastream_object_event_t *object_event)
{
    edgehog_result_t res = EDGEHOG_RESULT_FILE_TRANSFER_INVALID_REQUEST;

    char *ft_id = NULL;
    char *ft_url = NULL;
    char *ft_http_header_key = NULL;
    char *ft_http_header_value = NULL;
    char *ft_source_type = NULL;
    char *ft_source = NULL;
    bool ft_progress = false;

    if (!object_event) {
        EDGEHOG_LOG_ERR(
            "Unable to handle fdile transfer device to server event, object event undefined");
        goto failure;
    }

    astarte_object_entry_t *rx_values = object_event->entries;
    size_t rx_values_length = object_event->entries_len;

    for (size_t i = 0; i < rx_values_length; i++) {
        const char *path = rx_values[i].path;
        astarte_data_t rx_value = rx_values[i].data;

        if (strcmp(path, "id") == 0) {
            ft_id = duplicate_string((char *) rx_value.data.string);
            EDGEHOG_LOG_INF("id: %s", ft_id ? ft_id : "NULL");
        } else if (strcmp(path, "url") == 0) {
            ft_url = duplicate_string((char *) rx_value.data.string);
            EDGEHOG_LOG_INF("url: %s", ft_url ? ft_url : "NULL");
        } else if (strcmp(path, "httpHeaderKey") == 0) {
            ft_http_header_key = duplicate_string((char *) rx_value.data.string);
            EDGEHOG_LOG_INF("httpHeaderKey: %s", ft_http_header_key ? ft_http_header_key : "NULL");
        } else if (strcmp(path, "httpHeaderValue") == 0) {
            ft_http_header_value = duplicate_string((char *) rx_value.data.string);
            EDGEHOG_LOG_INF(
                "httpHeaderValue: %s", ft_http_header_value ? ft_http_header_value : "NULL");
        } else if (strcmp(path, "sourceType") == 0) {
            ft_source_type = duplicate_string((char *) rx_value.data.string);
            EDGEHOG_LOG_INF("sourceType: %s", ft_source_type ? ft_source_type : "NULL");
        } else if (strcmp(path, "source") == 0) {
            ft_source = duplicate_string((char *) rx_value.data.string);
            EDGEHOG_LOG_INF("source: %s", ft_source ? ft_source : "NULL");
        } else if (strcmp(path, "progress") == 0) {
            ft_progress = rx_value.data.boolean;
            EDGEHOG_LOG_INF("progress: %d", ft_progress);
        }
    }

    if (!ft_id || !ft_url || !ft_http_header_key || !ft_http_header_value || !ft_source_type
        || !ft_source) {
        EDGEHOG_LOG_ERR("Unable to extract data from request");
        res = EDGEHOG_RESULT_FILE_TRANSFER_INVALID_REQUEST;
        goto failure;
    }

    ft_device_to_server_data_t data = {
        .id = ft_id,
        .url = ft_url,
        .http_header_key = ft_http_header_key,
        .http_header_value = ft_http_header_value,
        .source_type = ft_source_type,
        .source = ft_source,
        .progress = ft_progress,
    };

    ft_msgq_data_t msg = { 0 };
    msg.type = FT_MSG_DEVICE_TO_SERVER;
    msg.payload.device_to_server = data;

    if (k_msgq_put(&device->file_transfer->msgq, &msg, K_NO_WAIT) != 0) {
        EDGEHOG_LOG_ERR("Unable to send file transfer data to the task handling it");
        res = EDGEHOG_RESULT_FILE_TRANSFER_QUEUE_ERROR;
        goto failure;
    }

    return EDGEHOG_RESULT_OK;

failure:

    k_free(ft_id);
    k_free(ft_url);
    k_free(ft_http_header_key);
    k_free(ft_http_header_value);
    k_free(ft_source_type);
    k_free(ft_source);

    return res;
}

edgehog_result_t edgehog_ft_stop(edgehog_file_transfer_t *file_transfer, k_timeout_t timeout)
{
    // Request the thread to self exit
    atomic_clear_bit(&file_transfer->thread_state, FILE_TRANSFER_SERVICE_THREAD_RUNNING_BIT);
    // Wait for the thread to self exit
    int res = k_thread_join(&file_transfer->thread, timeout);
    switch (res) {
        case 0:
            return EDGEHOG_RESULT_OK;
        case -EAGAIN:
            return EDGEHOG_RESULT_FILE_TRANSFER_STOP_TIMEOUT;
        default:
            return EDGEHOG_RESULT_INTERNAL_ERROR;
    }
}

void edgehog_ft_destroy(edgehog_file_transfer_t *file_transfer)
{
    k_msgq_cleanup(&file_transfer->msgq);
    free(file_transfer);
}

bool edgehog_ft_is_running(edgehog_file_transfer_t *file_transfer)
{
    if (!file_transfer) {
        return false;
    }
    return atomic_test_bit(&file_transfer->thread_state, FILE_TRANSFER_SERVICE_THREAD_RUNNING_BIT);
}

/************************************************
 *         Static functions definitions         *
 ***********************************************/

// TODO: this function implementation must be changed once the httpHeaderKey and httpHeaderValue
// endpoint
//  of the FT Astarte interface value is changed from string to stringarray.
static edgehog_result_t parse_http_headers(
    const char *header_key, const char *header_value, char *header_fields[], size_t max_elements)
{
    if (!header_key || !header_value || !header_fields || max_elements == 0) {
        return EDGEHOG_RESULT_FILE_TRANSFER_INVALID_REQUEST;
    }

    char *key_copy = duplicate_string(header_key);
    char *val_copy = duplicate_string(header_value);
    edgehog_result_t eres = EDGEHOG_RESULT_OK;

    if (!key_copy || !val_copy) {
        eres = EDGEHOG_RESULT_OUT_OF_MEMORY;
        goto cleanup_copies;
    }

    char *saveptr_key = NULL;
    char *saveptr_val = NULL;
    char *token_key = strtok_r(key_copy, ",", &saveptr_key);
    char *token_val = strtok_r(val_copy, ",", &saveptr_val);

    size_t idx = 0;
    // the last element (max_elements - 1) must be NULL
    while (token_key && token_val && idx < (max_elements - 1)) {
        size_t needed = strlen(token_key) + strlen(token_val) + 3;

        header_fields[idx] = k_malloc(needed);
        if (header_fields[idx] == NULL) {
            eres = EDGEHOG_RESULT_OUT_OF_MEMORY;
            goto cleanup_allocated;
        }

        snprintf(header_fields[idx], needed, "%s: %s", token_key, token_val); // NOLINT
        idx++;

        token_key = strtok_r(NULL, ",", &saveptr_key);
        token_val = strtok_r(NULL, ",", &saveptr_val);
    }

    header_fields[idx] = NULL;

    goto cleanup_copies;

cleanup_allocated:
    // free the partially allocated data
    free_http_headers(header_fields, idx);

cleanup_copies:
    k_free(key_copy);
    k_free(val_copy);

    return eres;
}

static void free_http_headers(char *header_fields[], size_t max_elements)
{
    // free header values
    size_t idx = 0;
    while (idx < max_elements && header_fields[idx] != NULL) {
        k_free(header_fields[idx]);
        header_fields[idx] = NULL;
        idx++;
    }
}

static edgehog_result_t ft_handle_server_to_device(
    edgehog_device_handle_t edgehog_device, ft_msgq_data_t *msg)
{
    EDGEHOG_LOG_DBG("FT (server to device) - received id: %s", msg->payload.server_to_device.id);
    EDGEHOG_LOG_DBG("FT (server to device) - received url: %s", msg->payload.server_to_device.url);
    EDGEHOG_LOG_DBG("FT (server to device) - received http_header_key: %s",
        msg->payload.server_to_device.http_header_key);
    EDGEHOG_LOG_DBG("FT (server to device) - received http_header_value: %s",
        msg->payload.server_to_device.http_header_value);
    EDGEHOG_LOG_DBG("FT (server to device) - received file_size_bytes: %lld",
        msg->payload.server_to_device.file_size_bytes);
    EDGEHOG_LOG_DBG(
        "FT (server to device) - received progress: %d", msg->payload.server_to_device.progress);

    if (msg->payload.server_to_device.file_size_bytes > SIZE_MAX) {
        EDGEHOG_LOG_ERR("Requested file transfer file size too big: %lld",
            msg->payload.server_to_device.file_size_bytes);
        return EDGEHOG_RESULT_FILE_TRANSFER_INVALID_REQUEST;
    }

    size_t expected_file_size = (size_t) msg->payload.server_to_device.file_size_bytes;

    char *file = k_calloc(expected_file_size, sizeof(char));
    if (file == NULL) {
        EDGEHOG_LOG_ERR("Failed to allocate memory for file (%zu bytes)", expected_file_size);
        return EDGEHOG_RESULT_OUT_OF_MEMORY;
    }

    ft_server_to_device_http_cb_data_t user_data = {
        .id = msg->payload.server_to_device.id,
        .url = msg->payload.server_to_device.url,
        .file_size_bytes = (size_t) msg->payload.server_to_device.file_size_bytes,
        .progress = msg->payload.server_to_device.progress,
        .edgehog_device = edgehog_device,
        .file = file,
        .current_offset = 0,
    };

    char *header_fields[FILE_TRANSFER_MAX_HTTP_HEADERS];
    memset((void *) header_fields, 0, sizeof(header_fields));

    edgehog_result_t eres = parse_http_headers(msg->payload.server_to_device.http_header_key,
        msg->payload.server_to_device.http_header_value, header_fields,
        FILE_TRANSFER_MAX_HTTP_HEADERS);

    if (eres != EDGEHOG_RESULT_OK) {
        EDGEHOG_LOG_ERR("Failed to parse http headers");
        goto exit;
    }

    http_download_t http_download
        = { .user_data = &user_data, .download_cbk = http_download_ft_std_payload_cbk };

    eres = edgehog_http_download(msg->payload.server_to_device.url, (const char **) header_fields,
        FT_REQ_TIMEOUT_MS, &http_download);

    if (eres != EDGEHOG_RESULT_OK) {
        EDGEHOG_LOG_ERR("Failed to perform http file transfer request");
    }

exit:
    // cleanup header-allocated strings
    free_http_headers(header_fields, FILE_TRANSFER_MAX_HTTP_HEADERS);

    if (file != NULL) {
        k_free(file);
    }

    k_free(msg->payload.server_to_device.id);
    k_free(msg->payload.server_to_device.url);
    k_free(msg->payload.server_to_device.http_header_key);
    k_free(msg->payload.server_to_device.http_header_value);

    return eres;
}

static void ft_handle_device_to_server(edgehog_device_handle_t edgehog_device, ft_msgq_data_t *msg)
{
    EDGEHOG_LOG_DBG("FT (device to server) - received id: %s", msg->payload.device_to_server.id);
    EDGEHOG_LOG_DBG("FT (device to server) - received url: %s", msg->payload.device_to_server.url);
    EDGEHOG_LOG_DBG("FT (device to server) - received http_header_key: %s",
        msg->payload.device_to_server.http_header_key);
    EDGEHOG_LOG_DBG("FT (device to server) - received http_header_value: %s",
        msg->payload.device_to_server.http_header_value);
    EDGEHOG_LOG_DBG("FT (device to server) - received source_type: %s",
        msg->payload.device_to_server.source_type);
    EDGEHOG_LOG_DBG(
        "FT (device to server) - received source: %s", msg->payload.device_to_server.source);
    EDGEHOG_LOG_DBG(
        "FT (device to server) - received progress: %d", msg->payload.device_to_server.progress);

    // TODO: handle file transfer request (upload file)
    //  now we simulate the operation with a sleep
    k_sleep(K_SECONDS(3));

    // Communicatre to Astarte a Response to the FT request
    astarte_object_entry_t object_entries[] = {
        { .path = "id", .data = astarte_data_from_string(msg->payload.device_to_server.id) },
        { .path = "type", .data = astarte_data_from_string("device_to_server") },
        // TODO: put here the correct posix return code after finished processing the file transfer
        // request
        { .path = "code", .data = astarte_data_from_longinteger(0) },
        { .path = "message", .data = astarte_data_from_string("") },
    };

    astarte_result_t res = astarte_device_send_object(edgehog_device->astarte_device,
        io_edgehog_devicemanager_fileTransfer_Response.name, "/request", object_entries,
        ARRAY_SIZE(object_entries), NULL);
    if (res != ASTARTE_RESULT_OK) {
        EDGEHOG_LOG_ERR("Unable to send File transfer response"); // NOLINT
    }

    // free resources
    k_free(msg->payload.device_to_server.id);
    k_free(msg->payload.device_to_server.url);
    k_free(msg->payload.device_to_server.http_header_key);
    k_free(msg->payload.device_to_server.http_header_value);
    k_free(msg->payload.device_to_server.source_type);
    k_free(msg->payload.device_to_server.source);
    memset(&msg->payload.device_to_server, 0, sizeof(msg->payload));
}

// entry point for the thread handling the file transfer operations
static void ft_service_thread_entry_point(void *device_ptr, void *queue_ptr, void *unused)
{
    EDGEHOG_LOG_DBG("FILE TRANSFER ENTRY POINT");
    ARG_UNUSED(unused);

    edgehog_device_handle_t edgehog_device = (edgehog_device_handle_t) device_ptr;
    struct k_msgq *msgq = (struct k_msgq *) queue_ptr;

    ft_msgq_data_t msg_rcv = { 0 };

    while (atomic_test_bit(
        &edgehog_device->file_transfer->thread_state, FILE_TRANSFER_SERVICE_THREAD_RUNNING_BIT)) {

        if (k_msgq_get(msgq, &msg_rcv, K_MSEC(100)) == 0) {
            // before performing the FT operation, check if there is an ongoing OTA operation
            k_sem_take(&edgehog_device->sync_ota_ft_sem, K_FOREVER);

            if (msg_rcv.type == FT_MSG_SERVER_TO_DEVICE) {
                EDGEHOG_LOG_DBG("handle server to device file transfer: %s",
                    msg_rcv.payload.server_to_device.id);
                ft_handle_server_to_device(edgehog_device, &msg_rcv);
            } else if (msg_rcv.type == FT_MSG_DEVICE_TO_SERVER) {
                EDGEHOG_LOG_DBG("handle device to server file transfer: %s",
                    msg_rcv.payload.device_to_server.source);
                ft_handle_device_to_server(edgehog_device, &msg_rcv);
            }

            k_sem_give(&edgehog_device->sync_ota_ft_sem);
        }
    }

    EDGEHOG_LOG_DBG("CLOSING FILE TRANSFER THREAD");
}

static char *duplicate_string(const char *src)
{
    if (src == NULL) {
        return NULL;
    }

    // length including the null terminator
    size_t len = strlen(src) + 1;
    char *dest = k_malloc(len);

    if (dest != NULL) {
        memcpy(dest, src, len);
    }

    return dest;
}
