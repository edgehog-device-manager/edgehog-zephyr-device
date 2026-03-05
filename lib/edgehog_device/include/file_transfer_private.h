#ifndef FILE_TRANSFER_PRIVATE_H
#define FILE_TRANSFER_PRIVATE_H

/**
 * @file file_transfer_private.h
 * @brief Private File Transfer fields.
 */

#include "edgehog_device/device.h"

#include <zephyr/kernel.h>

#define EDGEHOG_FILE_TRANSFER_LEN 10

typedef struct
{
    /** @brief File transfer entries list. */
    int32_t *entries[EDGEHOG_FILE_TRANSFER_LEN];
    /** @brief Message queue. */
    struct k_msgq msgq;
    /** @brief Ring buffer that holds queued messages. */
    // TODO: at the moment we are only sending int32 values for testing
    char msgq_buffer[EDGEHOG_FILE_TRANSFER_LEN * sizeof(int32_t)];
    /** @brief Telemetry service thread, peeks the msgq and transmits eventual messages. */
    struct k_thread thread;
    /** @brief Run state for the telemetry service thread. */
    atomic_t thread_state;
} edgehog_file_transfer_t;

edgehog_file_transfer_t *edgehog_file_transfer_new();

edgehog_result_t edgehog_file_transfer_start(edgehog_device_handle_t device);

edgehog_result_t edgehog_file_transfer_stop(edgehog_file_transfer_t *file_transfer, k_timeout_t timeout);

void edgehog_file_transfer_destroy(edgehog_file_transfer_t *ft);

bool edgehog_file_transfer_is_running(edgehog_file_transfer_t *file_transfer);

#endif // FILE_TRANSFER_PRIVATE_H
