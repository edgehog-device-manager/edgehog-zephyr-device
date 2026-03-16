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

#define FILE_TRANSFER_SERVICE_THREAD_STACK_SIZE 2048
#define FILE_TRANSFER_SERVICE_THREAD_PRIORITY 5
#define FILE_TRANSFER_SERVICE_THREAD_RUNNING_BIT (1)
#define FILE_TRANSFER_SERVICE_MSGQ_GET_TIMEOUT 100

// NOLINTBEGIN(cppcoreguidelines-avoid-non-const-global-variables)
K_THREAD_STACK_DEFINE(file_transfer_service_stack_area, FILE_TRANSFER_SERVICE_THREAD_STACK_SIZE);

// entry point for the thread handling the file transfer operations
// TODO: at the moment only astarte event reception is performed
//      implement also download (Astarte -> Device) and upload (Device -> Astarte) operations
static void file_transfer_service_thread_entry_point(
    void *device_ptr, void *queue_ptr, void *unused)
{
    EDGEHOG_LOG_DBG("FILE TRANSFER ENTRY POINT");
    ARG_UNUSED(unused);

    edgehog_device_handle_t device = (edgehog_device_handle_t) device_ptr;
    struct k_msgq *msgq = (struct k_msgq *) queue_ptr;

    ft_server_to_device_data_t msg_rcv = { 0 };

    while (atomic_test_bit(
        &device->file_transfer->thread_state, FILE_TRANSFER_SERVICE_THREAD_RUNNING_BIT)) {
        if (k_msgq_get(msgq, &msg_rcv, K_FOREVER) == 0) {
            EDGEHOG_LOG_DBG("FT - received id:                  %s", msg_rcv.id);
            EDGEHOG_LOG_DBG("FT - received url:                 %s", msg_rcv.url);
            EDGEHOG_LOG_DBG("FT - received http_header_key:     %s", msg_rcv.http_header_key);
            EDGEHOG_LOG_DBG("FT - received http_header_value:   %s", msg_rcv.http_header_value);
            EDGEHOG_LOG_DBG("FT - received file_size_bytes:     %lld", msg_rcv.file_size_bytes);
            EDGEHOG_LOG_DBG("FT - received progress:            %d", msg_rcv.progress);

            free(msg_rcv.id);
            free(msg_rcv.url);
            free(msg_rcv.http_header_key);
            free(msg_rcv.http_header_value);
            memset(&msg_rcv, 0, sizeof(msg_rcv));
        }
    }

    EDGEHOG_LOG_DBG("CLOSING FILE TRANSFER THREAD");
}

/************************************************
 *         Static functions declarations        *
 ***********************************************/

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
    edgehog_file_transfer_t *ft = calloc(1, sizeof(edgehog_file_transfer_t));
    if (!ft) {
        EDGEHOG_LOG_ERR("Out of memory %s: %d", __FILE__, __LINE__);
        return NULL;
    }

    // TODO: add config operations

    return ft;
}

edgehog_result_t edgehog_file_transfer_start(edgehog_device_handle_t device)
{
    edgehog_file_transfer_t *ft = device->file_transfer;

    if (!ft) {
        EDGEHOG_LOG_ERR("Unable to start file transfer, reference is null");
        return EDGEHOG_RESULT_FILE_TRANSFER_START_FAIL;
    }

    if (atomic_test_and_set_bit(&ft->thread_state, FILE_TRANSFER_SERVICE_THREAD_RUNNING_BIT)) {
        EDGEHOG_LOG_ERR("Failed starting file transfer service as it's already running");
        return EDGEHOG_RESULT_FILE_TRANSFER_START_FAIL;
    }

    k_msgq_init(
        &ft->msgq, ft->msgq_buffer, sizeof(ft_server_to_device_data_t), EDGEHOG_FILE_TRANSFER_LEN);

    k_tid_t thread_id = k_thread_create(&ft->thread, file_transfer_service_stack_area,
        FILE_TRANSFER_SERVICE_THREAD_STACK_SIZE, file_transfer_service_thread_entry_point,
        (void *) device, (void *) &ft->msgq, NULL, FILE_TRANSFER_SERVICE_THREAD_PRIORITY, 0,
        K_NO_WAIT);

    if (!thread_id) {
        EDGEHOG_LOG_ERR("Unable to start file transfer message thread");
        atomic_clear_bit(&ft->thread_state, FILE_TRANSFER_SERVICE_THREAD_RUNNING_BIT);
        return EDGEHOG_RESULT_FILE_TRANSFER_START_FAIL;
    }

    // assign a name to the thread for debugging purposes
    int ret = k_thread_name_set(thread_id, "file_transfer");
    if (ret != 0) {
        EDGEHOG_LOG_ERR("Failed to set thread name, error %d", ret);
        atomic_clear_bit(&ft->thread_state, FILE_TRANSFER_SERVICE_THREAD_RUNNING_BIT);
        return EDGEHOG_RESULT_FILE_TRANSFER_START_FAIL;
    }

    return EDGEHOG_RESULT_OK;
}

edgehog_result_t edgehog_file_transfer_event(
    edgehog_device_handle_t device, astarte_device_datastream_object_event_t *object_event)
{
    // TODO: should we check if the FT thread and other structs have been initialized?

    if (!object_event) {
        EDGEHOG_LOG_ERR("Unable to handle event, object event undefined");
        return EDGEHOG_RESULT_OTA_INVALID_REQUEST;
    }

    astarte_object_entry_t *rx_values = object_event->entries;
    size_t rx_values_length = object_event->entries_len;

    // If the file is to be stored, this MUST be unique to the file (e.g. hash of the content). For
    // a file that needs to be streamed, it can be unique for the single request (e.g. uuid)."
    char *ft_id = NULL;
    // The URL to get the file from
    char *ft_url = NULL;
    // Keys for the HTTP headers, must be in the order of the values
    char *ft_http_header_key = NULL;
    // Values for the HTTP headers, must be in the order of the keys
    char *ft_http_header_value = NULL;
    // // Optional enum string for the file compression with default value empty, other values are:
    // ['tar.gz']" char *compression = NULL; Total file size (if multiple files) uncompressed in
    // bytes. It's used to reserve this space on the device."
    int64_t ft_file_size_bytes = -1;
    // Flag to enable the progress reporting of the download
    bool ft_progress = false;
    // // Must be in the form sha256:deadbeaf, and should be used only if supported by the device."
    // char *digest = NULL;
    // // Optional file name of the file to download, default is empty"
    // char *file_name = NULL;
    // // Optional ttl for how long to keep the file for, if 0 is forever"
    // int64_t ttl_seconds = -1;
    // // Optional unix mode for the file, set to default if 0. All files are immutable, so setting
    // it to writable has no effect." int64_t file_mode = -1;
    // // Optional unix uid of the user owning the file, set to default if -1."
    // int64_t user_id = -1;
    // // Optional unix gid of the group owning the file, set to default if -1."
    // int64_t group_id = -1;
    // // String enum specifying the destination type, with allowed values: [storage, streaming,
    // filesystem]."
    // // The value depends on the selected destination type: for 'storage' and 'streaming' it's an
    // empty string, and for 'filesystem' is a path to a file on the device." char *destination =
    // NULL;

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
        return EDGEHOG_RESULT_FILE_TRANSFER_INVALID_REQUEST;
    }

    ft_server_to_device_data_t data = {
        .id = ft_id,
        .url = ft_url,
        .http_header_key = ft_http_header_key,
        .http_header_value = ft_http_header_value,
        .file_size_bytes = ft_file_size_bytes,
        .progress = ft_progress,
    };

    edgehog_result_t res = EDGEHOG_RESULT_OK;

    edgehog_file_transfer_t *ft = device->file_transfer;

    if (k_msgq_put(&ft->msgq, &data, K_NO_WAIT) != 0) {
        EDGEHOG_LOG_ERR("Unable to send file transfer data to the task handling it");
        // if message is not sent free the resources allocad onto the heap
        free(ft_id);
        free(ft_url);
        free(ft_http_header_key);
        free(ft_http_header_value);
        memset(&data, 0, sizeof(data));
        res = EDGEHOG_RESULT_FILE_TRANSFER_QUEUE_ERROR;
    }

    return res;
}

edgehog_result_t edgehog_file_transfer_stop(
    edgehog_file_transfer_t *file_transfer, k_timeout_t timeout)
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

void edgehog_file_transfer_destroy(edgehog_file_transfer_t *ft)
{
    k_msgq_cleanup(&ft->msgq);
    free(ft);
}

bool edgehog_file_transfer_is_running(edgehog_file_transfer_t *file_transfer)
{
    if (!file_transfer) {
        return false;
    }
    return atomic_test_bit(&file_transfer->thread_state, FILE_TRANSFER_SERVICE_THREAD_RUNNING_BIT);
}

/************************************************
 *         Static functions definitions         *
 ***********************************************/

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
