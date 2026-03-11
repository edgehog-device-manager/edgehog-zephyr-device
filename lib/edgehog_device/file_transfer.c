#include "file_transfer_private.h"

#include "edgehog_private.h"
#include "generated_interfaces.h"
#include "hardware_info.h"
#include "settings.h"
#include "storage_usage.h"
#include "system_status.h"

#include <stdlib.h>

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

    int32_t msg_rcv = -1;

    while (atomic_test_bit(
        &device->file_transfer->thread_state, FILE_TRANSFER_SERVICE_THREAD_RUNNING_BIT)) {
        if (k_msgq_get(msgq, &msg_rcv, K_FOREVER) == 0) {
            EDGEHOG_LOG_DBG("FT - received: %d", msg_rcv);
        }
    }

    EDGEHOG_LOG_DBG("CLOSING FILE TRANSFER THREAD");
}

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

    k_msgq_init(&ft->msgq, ft->msgq_buffer, sizeof(int32_t), EDGEHOG_FILE_TRANSFER_LEN);

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

    // TODO: This is only done for simulation. Handle reception of Astarte FT event with correct
    // data.
    edgehog_file_transfer_t *ft = device->file_transfer;

    for (int32_t i = 1; i <= 10; i++) {
        k_msgq_put(&ft->msgq, &i, K_NO_WAIT);
        k_sleep(K_MSEC(1000));
    }

    return EDGEHOG_RESULT_OK;
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
