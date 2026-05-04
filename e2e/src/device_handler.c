/*
 * (C) Copyright 2025, SECO Mind Srl
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "device_handler.h"

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/atomic.h>

#include <edgehog_device/device.h>
#include <edgehog_device/file_transfer.h>

#include "utilities.h"

LOG_MODULE_REGISTER(device_handler, CONFIG_DEVICE_HANDLER_LOG_LEVEL);

/************************************************
 *   Constants, static variables and defines    *
 ***********************************************/

#define GENERIC_WAIT_SLEEP_500_MS 500

static edgehog_device_handle_t device_handle = NULL;

K_THREAD_STACK_DEFINE(device_thread_stack_area, CONFIG_E2E_DEVICE_THREAD_STACK_SIZE);
static struct k_thread device_thread_data;
static atomic_t device_thread_flags;
enum device_thread_flags
{
    DEVICE_THREAD_CONNECTED_FLAG = 0,
    DEVICE_THREAD_TERMINATION_FLAG,
};

/************************************************
 *         Static functions declaration         *
 ***********************************************/

static void device_thread_entry_point(void *unused1, void *unused2, void *unused3);
static void connection_callback(astarte_device_connection_event_t event);
static void disconnection_callback(astarte_device_disconnection_event_t event);
static void download_thread_entry_point(void *unused1, void *unused2, void *unused3);
static void upload_thread_entry_point(void *unused1, void *unused2, void *unused3);

/************************************************
 *         Global functions definition          *
 ***********************************************/

// Large static buffer to hold the loopback payload in memory
#define MAX_LOOPBACK_FILE_SIZE 32768
static uint8_t loopback_file_buffer[MAX_LOOPBACK_FILE_SIZE];
static size_t loopback_file_size = 0;

K_THREAD_STACK_DEFINE(download_thread_stack_area, 2048);
static struct k_thread download_thread_data;

K_THREAD_STACK_DEFINE(upload_thread_stack_area, 2048);
static struct k_thread upload_thread_data;

// Pointers to the dynamic pipe and event provided by the file transfer logic
static struct k_pipe *current_loopback_pipe = NULL;
static struct k_event *current_loopback_event = NULL;

static bool on_stream_transfer_start(
    const char *name, edgehog_ft_type_t type, size_t expected_size, edgehog_ft_stream_t *stream)
{
    if (strcmp(name, "loopback") == 0) {
        // Only one file transfer can be active at a time
        current_loopback_pipe = stream->pipe;
        current_loopback_event = stream->event;

        if (type == EDGEHOG_FT_TYPE_SERVER_TO_DEVICE) {
            LOG_INF("Starting stream transfer [server-to-device] on pipe '%s'. Expected size: %zu "
                    "bytes",
                name, expected_size);

            // Spawn the download thread on demand
            k_thread_create(&download_thread_data, download_thread_stack_area,
                K_THREAD_STACK_SIZEOF(download_thread_stack_area), download_thread_entry_point,
                NULL, NULL, NULL, CONFIG_E2E_DEVICE_THREAD_PRIORITY, 0, K_NO_WAIT);
        } else {
            LOG_INF("Starting stream transfer [device-to-server] on pipe '%s'. Expected size: %zu "
                    "bytes",
                name, expected_size);

            // Spawn the upload thread on demand
            k_thread_create(&upload_thread_data, upload_thread_stack_area,
                K_THREAD_STACK_SIZEOF(upload_thread_stack_area), upload_thread_entry_point, NULL,
                NULL, NULL, CONFIG_E2E_DEVICE_THREAD_PRIORITY, 0, K_NO_WAIT);
        }

        return true;
    }

    LOG_WRN("Rejected stream transfer for unknown path/name: %s", name);
    return false; // Reject the transfer
}

void setup_device()
{
    LOG_INF("Initializing an Edgehog device.");

    LOG_INF("Spawning a new thread to poll data from the Edgehog device.");
    k_thread_create(&device_thread_data, device_thread_stack_area,
        K_THREAD_STACK_SIZEOF(device_thread_stack_area), device_thread_entry_point, NULL, NULL,
        NULL, CONFIG_E2E_DEVICE_THREAD_PRIORITY, 0, K_NO_WAIT);

    LOG_INF("Edgehog device created.");
}

void free_device()
{
    atomic_set_bit(&device_thread_flags, DEVICE_THREAD_TERMINATION_FLAG);

    CHECK_HALT(k_thread_join(&device_thread_data, K_FOREVER) != 0,
        "Failed in waiting for the Edgehog thread to terminate.");

    LOG_INF("Destroing Edgehog device and freeing resources.");
    LOG_INF("Edgehog device destroyed.");
}

void wait_for_device_connection()
{
    while (!atomic_test_bit(&device_thread_flags, DEVICE_THREAD_CONNECTED_FLAG)) {
        k_sleep(K_MSEC(GENERIC_WAIT_SLEEP_500_MS));
    }
}

void disconnect_device()
{
    atomic_set_bit(&device_thread_flags, DEVICE_THREAD_TERMINATION_FLAG);
}

void wait_for_device_disconnection()
{
    while (atomic_test_bit(&device_thread_flags, DEVICE_THREAD_CONNECTED_FLAG)) {
        k_sleep(K_MSEC(GENERIC_WAIT_SLEEP_500_MS));
    }
}

/************************************************
 *         Static functions definitions         *
 ***********************************************/

static void device_thread_entry_point(void *unused1, void *unused2, void *unused3)
{
    ARG_UNUSED(unused1);
    ARG_UNUSED(unused2);
    ARG_UNUSED(unused3);

    edgehog_result_t eres = EDGEHOG_RESULT_OK;

    LOG_INF("Started Edgehog device thread.");

    char cred_secr[ASTARTE_PAIRING_CRED_SECR_LEN + 1] = CONFIG_E2E_ASTARTE_CREDENTIAL_SECRET;
    char device_id[ASTARTE_DEVICE_ID_LEN + 1] = CONFIG_E2E_ASTARTE_DEVICE_ID;

    astarte_device_config_t astarte_device_config = { 0 };
    astarte_device_config.http_timeout_ms = CONFIG_E2E_HTTP_TIMEOUT_MS;
    astarte_device_config.mqtt_connection_timeout_ms = CONFIG_E2E_MQTT_CONNECTION_TIMEOUT_MS;
    astarte_device_config.mqtt_poll_timeout_ms = CONFIG_E2E_MQTT_POLL_TIMEOUT_MS;
    astarte_device_config.connection_cbk = connection_callback;
    astarte_device_config.disconnection_cbk = disconnection_callback;
    memcpy(astarte_device_config.cred_secr, cred_secr, sizeof(cred_secr));
    memcpy(astarte_device_config.device_id, device_id, sizeof(device_id));

    edgehog_device_config_t edgehog_conf = {
        .astarte_device_config = astarte_device_config,
        .telemetry_config = NULL,
        .telemetry_config_len = 0,
        .file_transfer_cbks = { .on_stream_transfer_start = on_stream_transfer_start },
    };
    eres = edgehog_device_new(&edgehog_conf, &device_handle);
    if (eres != EDGEHOG_RESULT_OK) {
        LOG_ERR("Unable to create edgehog device handle");
        return;
    }

    LOG_INF("Starting Edgehog.");
    eres = edgehog_device_start(device_handle);
    if (eres != EDGEHOG_RESULT_OK) {
        LOG_ERR("Unable to start edgehog device");
        return;
    }

    while (!atomic_test_bit(&device_thread_flags, DEVICE_THREAD_TERMINATION_FLAG)) {
        k_timepoint_t timepoint = sys_timepoint_calc(K_MSEC(CONFIG_E2E_DEVICE_POLL_PERIOD_MS));

        eres = edgehog_device_poll(device_handle);
        if (eres != EDGEHOG_RESULT_OK) {
            LOG_ERR("Edgehog device poll failure.");
            goto exit;
        }

        k_sleep(sys_timepoint_timeout(timepoint));
    }

    LOG_INF("Stopping Edgehog.");
    eres = edgehog_device_stop(device_handle, K_FOREVER);
    if (eres != EDGEHOG_RESULT_OK) {
        LOG_ERR("Unable to stop the edgehog device");
        goto exit;
    }

exit:

    LOG_INF("Destroying Edgehog.");
    edgehog_device_destroy(device_handle);

    LOG_INF("Exiting from the polling thread.");
}

static void connection_callback(astarte_device_connection_event_t event)
{
    (void) event;
    LOG_INF("Device connected");
    atomic_set_bit(&device_thread_flags, DEVICE_THREAD_CONNECTED_FLAG);
}

static void disconnection_callback(astarte_device_disconnection_event_t event)
{
    (void) event;
    LOG_INF("Device disconnected");
    atomic_clear_bit(&device_thread_flags, DEVICE_THREAD_CONNECTED_FLAG);
}

static void download_thread_entry_point(void *unused1, void *unused2, void *unused3)
{
    ARG_UNUSED(unused1);
    ARG_UNUSED(unused2);
    ARG_UNUSED(unused3);

    LOG_INF("Started Loopback download thread (Server -> Device RAM).");

    loopback_file_size = 0;

    bool download_complete = false;
    while (!download_complete
        && !atomic_test_bit(&device_thread_flags, DEVICE_THREAD_TERMINATION_FLAG)) {
        uint8_t chunk[256];

        // Read progressively from the pipe provided by Edgehog
        int ret = k_pipe_read(current_loopback_pipe, chunk, sizeof(chunk), K_MSEC(100));
        if (ret > 0) {
            if (loopback_file_size + ret <= MAX_LOOPBACK_FILE_SIZE) {
                memcpy(loopback_file_buffer + loopback_file_size, chunk, ret);
                loopback_file_size += ret;
            } else {
                LOG_ERR("Loopback buffer overflow! Max size is %d", MAX_LOOPBACK_FILE_SIZE);
                k_event_post(current_loopback_event, EDGEHOG_FT_STREAM_ERROR_EVENT_FLAG);
                return;
            }
        } else if (ret < 0 && ret != -EAGAIN) {
            LOG_ERR("Failed to read from loopback pipe");
            k_event_post(current_loopback_event, EDGEHOG_FT_STREAM_ERROR_EVENT_FLAG);
            return;
        }

        // Check if Edgehog posted the EOF flag indicating download completion
        if (k_event_test(current_loopback_event, EDGEHOG_FT_STREAM_EOF_EVENT_FLAG)) {
            download_complete = true;
        }
    }

    if (!atomic_test_bit(&device_thread_flags, DEVICE_THREAD_TERMINATION_FLAG)) {
        LOG_INF("Loopback download complete: downloaded %zu bytes to RAM", loopback_file_size);
    }

    // Acknowledge that reading is complete so the library can safely tear down memory structures
    if (current_loopback_event) {
        k_event_post(current_loopback_event, EDGEHOG_FT_STREAM_ACK_EVENT_FLAG);
    }

    LOG_INF("Exiting download thread.");
}

static void upload_thread_entry_point(void *unused1, void *unused2, void *unused3)
{
    ARG_UNUSED(unused1);
    ARG_UNUSED(unused2);
    ARG_UNUSED(unused3);

    LOG_INF("Started Loopback upload thread (Device RAM -> Server).");

    size_t total_written = 0;

    // We only enter the upload phase if we actually received a file in the download phase
    if (loopback_file_size > 0) {
        while (total_written < loopback_file_size
            && !atomic_test_bit(&device_thread_flags, DEVICE_THREAD_TERMINATION_FLAG)) {

            // Write progressively to the pipe. This will block if the pipe is full,
            // properly syncing with the upload HTTP stream.
            int ret = k_pipe_write(current_loopback_pipe, loopback_file_buffer + total_written,
                loopback_file_size - total_written, K_MSEC(100));

            if (ret > 0) {
                total_written += ret;
            } else if (ret < 0 && ret != -EAGAIN) {
                LOG_ERR("Failed to write to loopback pipe");
            }
        }

        if (!atomic_test_bit(&device_thread_flags, DEVICE_THREAD_TERMINATION_FLAG)) {
            LOG_INF("Loopback upload complete: uploaded %zu bytes from RAM", total_written);
        }

        // Post EOF to inform Edgehog's read stream that there is no more data
        if (current_loopback_event) {
            k_event_post(current_loopback_event, EDGEHOG_FT_STREAM_EOF_EVENT_FLAG);
        }
    } else {
        LOG_WRN("Loopback file size is 0. Nothing to upload.");
        if (current_loopback_event) {
            k_event_post(current_loopback_event, EDGEHOG_FT_STREAM_EOF_EVENT_FLAG);
        }
    }

    LOG_INF("Exiting upload thread.");
}
