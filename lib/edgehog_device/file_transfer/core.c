/*
 * (C) Copyright 2026, SECO Mind Srl
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "file_transfer/core.h"

#include "edgehog_private.h"
#include "file_transfer/download.h"
#include "file_transfer/upload.h"
#include "file_transfer/utils.h"
#include "generated_interfaces.h"
#include "log.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

EDGEHOG_LOG_MODULE_REGISTER(file_transfer, CONFIG_EDGEHOG_DEVICE_FILE_TRANSFER_LOG_LEVEL);

/************************************************
 *        Defines, constants and typedef        *
 ***********************************************/

#define THREAD_STACK_SIZE 8192
#define THREAD_PRIORITY 5
#define THREAD_RUNNING_BIT (1)

// NOLINTBEGIN(cppcoreguidelines-avoid-non-const-global-variables)
K_THREAD_STACK_DEFINE(file_transfer_service_thread_stack_area, THREAD_STACK_SIZE);
// NOLINTEND(cppcoreguidelines-avoid-non-const-global-variables)

/************************************************
 *         Static functions declarations        *
 ***********************************************/

static void thread_entry_point(void *device_ptr, void *unused1, void *unused2);
static edgehog_ft_filesystem_partition_t *duplicate_partitions(
    edgehog_ft_filesystem_partition_t *partitions, size_t partitions_len);
static void free_partitions(edgehog_ft_filesystem_partition_t *partitions, size_t partitions_len);

/************************************************
 *         Global functions definitions         *
 ***********************************************/

void edgeghog_ft_publish_capabilities(edgehog_device_handle_t edgehog_device)
{
    EDGEHOG_LOG_DBG("Publishing Edgehog file transfer capabilities");

    // TODO: add support for missign encodings and update the list accordingly
    // Possible values: [gz, lz4, tar, tar.gz, tar.lz4]
    const char *supported_encodings[] = {
#ifdef CONFIG_EDGEHOG_DEVICE_FILE_TRANSFER_COMPRESSION
        "lz4",
#endif
    };
    size_t supported_encodings_len = ARRAY_SIZE(supported_encodings);
    astarte_result_t res = astarte_device_set_property(edgehog_device->astarte_device,
        io_edgehog_devicemanager_fileTransfer_Capabilities.name, "/encodings",
        astarte_data_from_string_array(supported_encodings, supported_encodings_len));
    if (res != ASTARTE_RESULT_OK) {
        EDGEHOG_LOG_ERR("Unable to publish capability: /encodings");
        return;
    }

    // Unix permissions are not supported on this plaform
    res = astarte_device_set_property(edgehog_device->astarte_device,
        io_edgehog_devicemanager_fileTransfer_Capabilities.name, "/unixPermissions",
        astarte_data_from_boolean(false));
    if (res != ASTARTE_RESULT_OK) {
        EDGEHOG_LOG_ERR("Unable to publish capability: /unixPermissions");
        return;
    }

    // Possible values: [storage, streaming, filesystem]
    const char *supported_targets[] = { "streaming", "filesystem" };
    size_t supported_targets_len = ARRAY_SIZE(supported_targets);
    res = astarte_device_set_property(edgehog_device->astarte_device,
        io_edgehog_devicemanager_fileTransfer_Capabilities.name, "/targets",
        astarte_data_from_string_array(supported_targets, supported_targets_len));
    if (res != ASTARTE_RESULT_OK) {
        EDGEHOG_LOG_ERR("Unable to publish capability: /targets");
        return;
    }
}

edgehog_ft_t *edgehog_ft_new(
    edgehog_ft_cbks_t cbks, edgehog_ft_filesystem_partition_t *partitions, size_t partitions_len)
{
    // Allocate space for the file transfer internal struct
    edgehog_ft_t *data = k_calloc(1, sizeof(edgehog_ft_t));
    if (!data) {
        EDGEHOG_LOG_ERR("Out of memory %s: %d", __FILE__, __LINE__);
        goto error;
    }

    if (partitions && partitions_len > 0) {
        data->partitions = duplicate_partitions(partitions, partitions_len);
        if (!data->partitions) {
            EDGEHOG_LOG_ERR("Failed to duplicate partitions");
            goto error;
        }
        data->partitions_len = partitions_len;
    }
    data->cbks = cbks;

    return data;

error:
    k_free(data);
    return NULL;
}

void edgehog_ft_destroy(edgehog_ft_t *file_transfer)
{
    if (file_transfer && file_transfer->partitions) {
        free_partitions(file_transfer->partitions, file_transfer->partitions_len);
    }
    k_free(file_transfer);
}

edgehog_result_t edgehog_ft_start(edgehog_device_handle_t device)
{
    edgehog_ft_t *file_transfer = device->file_transfer;

    if (!file_transfer) {
        EDGEHOG_LOG_ERR("Unable to start file transfer, reference is null");
        return EDGEHOG_RESULT_FILE_TRANSFER_START_FAIL;
    }

    if (atomic_test_and_set_bit(&file_transfer->thread_state, THREAD_RUNNING_BIT)) {
        EDGEHOG_LOG_ERR("Failed starting file transfer service as it's already running");
        return EDGEHOG_RESULT_FILE_TRANSFER_START_FAIL;
    }

    if (k_msgq_alloc_init(&file_transfer->msgq, sizeof(edgehog_ft_msg_t),
            CONFIG_EDGEHOG_DEVICE_ADVANCED_FILE_TRANSFER_QUEUE_SIZE)
        != 0) {
        EDGEHOG_LOG_ERR("Unable to allocate and init file transfer message queue");
        atomic_clear_bit(&file_transfer->thread_state, THREAD_RUNNING_BIT);
        return EDGEHOG_RESULT_FILE_TRANSFER_START_FAIL;
    }

    k_tid_t thread_id = k_thread_create(&file_transfer->thread,
        file_transfer_service_thread_stack_area, THREAD_STACK_SIZE, thread_entry_point,
        (void *) device, NULL, NULL, THREAD_PRIORITY, 0, K_NO_WAIT);

    if (!thread_id) {
        EDGEHOG_LOG_ERR("Unable to start file transfer message thread");
        k_msgq_cleanup(&file_transfer->msgq);
        atomic_clear_bit(&file_transfer->thread_state, THREAD_RUNNING_BIT);
        return EDGEHOG_RESULT_FILE_TRANSFER_START_FAIL;
    }

#ifdef CONFIG_THREAD_NAME
    int ret = k_thread_name_set(thread_id, "file_transfer");
    if (ret != 0) {
        EDGEHOG_LOG_ERR("Failed to set thread name, error %d", ret);
        k_thread_abort(thread_id);
        k_msgq_cleanup(&file_transfer->msgq);
        atomic_clear_bit(&file_transfer->thread_state, THREAD_RUNNING_BIT);
        return EDGEHOG_RESULT_FILE_TRANSFER_START_FAIL;
    }
#endif

    return EDGEHOG_RESULT_OK;
}

edgehog_result_t edgehog_ft_stop(edgehog_ft_t *file_transfer, k_timeout_t timeout)
{
    edgehog_result_t eres = EDGEHOG_RESULT_OK;
    if (!file_transfer) {
        EDGEHOG_LOG_ERR("Unable to stop file transfer, reference is null");
        return EDGEHOG_RESULT_INVALID_PARAM;
    }
    // Check if the file transfer is running
    if (!atomic_test_bit(&file_transfer->thread_state, THREAD_RUNNING_BIT)) {
        EDGEHOG_LOG_WRN("Stopping edgehog file transfer while not running");
        return EDGEHOG_RESULT_OK;
    }
    // Request the thread to self exit
    atomic_clear_bit(&file_transfer->thread_state, THREAD_RUNNING_BIT);
    // Wait for the thread to self exit
    int res = k_thread_join(&file_transfer->thread, timeout);
    switch (res) {
        case 0:
            break;
        case -EAGAIN:
            // Force stop the thread, this will likely leak memory and resources
            k_thread_abort(&file_transfer->thread);
            eres = EDGEHOG_RESULT_FILE_TRANSFER_STOP_TIMEOUT;
            break;
        default:
            EDGEHOG_LOG_ERR("Failed to stop file transfer thread, error %d", res);
            eres = EDGEHOG_RESULT_INTERNAL_ERROR;
    }
    // Empty the message queue from leftovers
    edgehog_ft_msg_t msg_rcv;
    while (k_msgq_get(&file_transfer->msgq, &msg_rcv, K_NO_WAIT) == 0) {
        edgehog_ft_msg_destroy(&msg_rcv);
    }
    // Safely free the message queue's internal buffer
    k_msgq_cleanup(&file_transfer->msgq);

    return eres;
}

bool edgehog_ft_is_running(edgehog_ft_t *file_transfer)
{
    if (!file_transfer) {
        EDGEHOG_LOG_ERR("Unable to check file transfer status, reference is null");
        return false;
    }
    return atomic_test_bit(&file_transfer->thread_state, THREAD_RUNNING_BIT);
}

edgehog_result_t edgehog_ft_process_event(edgehog_device_handle_t device,
    astarte_device_datastream_object_event_t *object_event, edgehog_ft_type_t type)
{
    edgehog_result_t eres = EDGEHOG_RESULT_OK;
    edgehog_ft_msg_t msg = { 0 };
    int posix_errno = EPIPE;
    char *message = "Internal failure during file transfer request processing";

    if (!object_event) {
        EDGEHOG_LOG_ERR("File transfer event, object event undefined");
        eres = EDGEHOG_RESULT_FILE_TRANSFER_INVALID_REQUEST;
        goto failure;
    }

    eres = edgehog_ft_msg_init(object_event->entries, object_event->entries_len, type, &msg);
    if (eres != EDGEHOG_RESULT_OK) {
        EDGEHOG_LOG_ERR("Failed to initialize file transfer message from event data");
        goto failure;
    }

    if (!edgehog_ft_is_running(device->file_transfer)) {
        EDGEHOG_LOG_ERR("The file transfer is not yet ready");
        eres = EDGEHOG_RESULT_DEVICE_NOT_READY;
        posix_errno = EAGAIN;
        message = "Device not ready to perform file transfer";
        goto failure;
    }

    if (k_msgq_put(&device->file_transfer->msgq, &msg, K_NO_WAIT) != 0) {
        EDGEHOG_LOG_ERR("Unable to send file transfer data to the handler task");
        eres = EDGEHOG_RESULT_FILE_TRANSFER_QUEUE_ERROR;
        goto failure;
    }

    return eres;

failure:
    // Send a failure to Astarte if possible, otherwise just log the error and exit
    // TODO: Evaluate if it's possible to offload this operation outside of this function.
    //  to avoid performing a network operation within the event handler
    if (msg.id) {
        edgehog_ft_send_response(device, msg.id, type, posix_errno, message, eres);
    }
    edgehog_ft_msg_destroy(&msg);
    return eres;
}

/************************************************
 *         Static functions definitions         *
 ***********************************************/

static void thread_entry_point(void *device_ptr, void *unused1, void *unused2)
{
    EDGEHOG_LOG_DBG("File transfer thread entry point.");
    ARG_UNUSED(unused1);
    ARG_UNUSED(unused2);

    edgehog_device_handle_t edgehog_device = (edgehog_device_handle_t) device_ptr;
    edgehog_ft_t *file_transfer = edgehog_device->file_transfer;
    struct k_msgq *msgq = &file_transfer->msgq;

    edgehog_ft_msg_t msg_rcv = { 0 };
    while (atomic_test_bit(&file_transfer->thread_state, THREAD_RUNNING_BIT)) {
        if (k_msgq_get(msgq, &msg_rcv, K_MSEC(100)) == 0) {

            // Wait for OTA semaphore with a timeout to prevent deadlocks during shutdown
            bool sem_taken = false;
            while (atomic_test_bit(&file_transfer->thread_state, THREAD_RUNNING_BIT)) {
                if (k_sem_take(&edgehog_device->sync_ota_ft_sem, K_MSEC(100)) == 0) {
                    sem_taken = true;
                    break;
                }
            }

            // If the thread was stopped while waiting, clean up the popped message and exit
            if (!sem_taken) {
                edgehog_ft_msg_destroy(&msg_rcv);
                break;
            }

            // Proceed with the transfer since we hold the semaphore
            if (msg_rcv.type == EDGEHOG_FT_TYPE_SERVER_TO_DEVICE) {
                EDGEHOG_LOG_DBG("Server to device file transfer: %s", msg_rcv.id);
                edgehog_ft_handle_server_to_device(edgehog_device, &msg_rcv);
            } else if (msg_rcv.type == EDGEHOG_FT_TYPE_DEVICE_TO_SERVER) {
                EDGEHOG_LOG_DBG("Device to server file transfer: %s", msg_rcv.id);
                edgehog_ft_handle_device_to_server(edgehog_device, &msg_rcv);
            }

            k_sem_give(&edgehog_device->sync_ota_ft_sem);
        }
    }

    EDGEHOG_LOG_DBG("Exiting file transfer thread");
}

static edgehog_ft_filesystem_partition_t *duplicate_partitions(
    edgehog_ft_filesystem_partition_t *partitions, size_t partitions_len)
{
    edgehog_ft_filesystem_partition_t *dup = NULL;
    if (!partitions || (partitions_len == 0)) {
        goto error;
    }

    // Initialize the array of partition structures
    dup = k_calloc(partitions_len, sizeof(edgehog_ft_filesystem_partition_t));
    if (!dup) {
        EDGEHOG_LOG_ERR("Out of memory %s: %d", __FILE__, __LINE__);
        goto error;
    }

    // Initialize each partition structure in the array
    for (size_t i = 0; i < partitions_len; i++) {
        // Deep copy the partition name string
        size_t mount_point_size = strlen(partitions[i].mount_point) + 1;
        char *mount_point = k_malloc(mount_point_size);
        if (!mount_point) {
            EDGEHOG_LOG_ERR("Out of memory %s: %d", __FILE__, __LINE__);
            goto error;
        }
        dup[i].mount_point = memcpy(mount_point, partitions[i].mount_point, mount_point_size);
        dup[i].permissions = partitions[i].permissions;
    }

    return dup;

error:
    if (dup) {
        for (size_t i = 0; dup[i].mount_point; i++) {
            k_free((void *) dup[i].mount_point);
        }
    }
    k_free(dup);
    return NULL;
}

static void free_partitions(edgehog_ft_filesystem_partition_t *partitions, size_t partitions_len)
{
    if (!partitions) {
        return;
    }

    for (size_t i = 0; i < partitions_len; i++) {
        k_free((void *) partitions[i].mount_point);
    }
    k_free(partitions);
}
