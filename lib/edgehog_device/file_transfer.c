/*
 * (C) Copyright 2026, SECO Mind Srl
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "file_transfer_private.h"

#include "edgehog_private.h"
#include "generated_interfaces.h"
#include "hardware_info.h"
#include "settings.h"
#include "storage_usage.h"
#include "system_status.h"

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

// NOLINTBEGIN(cppcoreguidelines-avoid-non-const-global-variables)
K_THREAD_STACK_DEFINE(file_transfer_service_stack_area, FILE_TRANSFER_SERVICE_THREAD_STACK_SIZE);
// NOLINTEND(cppcoreguidelines-avoid-non-const-global-variables)

/************************************************
 *         Static functions declarations        *
 ***********************************************/

static void ft_handle_server_to_device(edgehog_device_handle_t edgehog_device, ft_msgq_data_t *msg);

static void ft_handle_device_to_server(edgehog_device_handle_t edgehog_device, ft_msgq_data_t *msg);

// entry point for the thread handling the file transfer operations
static void ft_service_thread_entry_point(void *device_ptr, void *queue_ptr, void *unused);

// Equivalent of POSIX strdup() funciton
static char *duplicate_string(const char *src);

/************************************************
 *         Global functions definitions         *
 ***********************************************/

/**
 * @brief Create an Edgehog file transfer service.
 *
 * // TODO: add config params to populate the file transfer
 * @return A pointer to Edgehog telemetry or a NULL if an error occurred.
 */
edgehog_file_transfer_t *edgehog_file_transfer_new()
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
    if (ret != 0) {
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

    if (!ft_id || !ft_url || !ft_http_header_key || !ft_http_header_value || !ft_file_size_bytes
        || !ft_progress) {
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

    free(ft_id);
    free(ft_url);
    free(ft_http_header_key);
    free(ft_http_header_value);

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
        || !ft_source || !ft_progress) {
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

    free(ft_id);
    free(ft_url);
    free(ft_http_header_key);
    free(ft_http_header_value);
    free(ft_source_type);
    free(ft_source);

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

static void ft_handle_server_to_device(edgehog_device_handle_t edgehog_device, ft_msgq_data_t *msg)
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

    // TODO: handle file transfer request (http req to download file)
    //  now we simulate the operation with a sleep
    k_sleep(K_SECONDS(3));

    // Communicatre to Astarte a Response to the FT request
    astarte_object_entry_t object_entries[] = {
        { .path = "id", .data = astarte_data_from_string(msg->payload.server_to_device.id) },
        { .path = "type", .data = astarte_data_from_string("server_to_device") },
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
    free(msg->payload.server_to_device.id);
    free(msg->payload.server_to_device.url);
    free(msg->payload.server_to_device.http_header_key);
    free(msg->payload.server_to_device.http_header_value);
    memset(&msg->payload.server_to_device, 0, sizeof(msg->payload));
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
    free(msg->payload.device_to_server.id);
    free(msg->payload.device_to_server.url);
    free(msg->payload.device_to_server.http_header_key);
    free(msg->payload.device_to_server.http_header_value);
    free(msg->payload.device_to_server.source_type);
    free(msg->payload.device_to_server.source);
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
        // before performing the FT opration, check if there is an ongoing OTA operation
        k_sem_take(edgehog_device->sync_ota_ft_sem, K_FOREVER);

        if (k_msgq_get(msgq, &msg_rcv, K_FOREVER) == 0) {
            if (msg_rcv.type == FT_MSG_SERVER_TO_DEVICE) {
                EDGEHOG_LOG_DBG("handle server to device file transfer: %s",
                    msg_rcv.payload.server_to_device.id);
                ft_handle_server_to_device(edgehog_device, &msg_rcv);
            } else if (msg_rcv.type == FT_MSG_DEVICE_TO_SERVER) {
                EDGEHOG_LOG_DBG("handle device to server file transfer: %s",
                    msg_rcv.payload.device_to_server.source);
                ft_handle_device_to_server(edgehog_device, &msg_rcv);
            }
        }

        k_sem_give(edgehog_device->sync_ota_ft_sem);
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
    char *dest = malloc(len);

    if (dest != NULL) {
        memcpy(dest, src, len);
    }

    return dest;
}
