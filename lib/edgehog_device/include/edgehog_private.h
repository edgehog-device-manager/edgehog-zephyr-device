/*
 * (C) Copyright 2024, SECO Mind Srl
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef EDGEHOG_PRIVATE_H
#define EDGEHOG_PRIVATE_H

/**
 * @file edgehog_private.h
 * @brief Private Edgehog Device APIs and fields
 */

#include "file_transfer/core.h"
#include "led.h"
#include "ota.h"
#include "telemetry_private.h"

#ifdef CONFIG_WIFI
#include "wifi_scan.h"
#endif

#include <astarte_device_sdk/device.h>

#include <zephyr/kernel.h>
#include <zephyr/sys/uuid.h>

/** @brief Possible states for the Edgehog device. */
enum device_states
{
    /** @brief The device is not operational. */
    EDGEHOG_DEVICE_STOPPED = 0U,
    /** @brief The device is has been started, but does not yet have connectivity. */
    EDGEHOG_DEVICE_STARTED,
    /** @brief The device is has been started, and has been connected to Astarte. */
    EDGEHOG_DEVICE_CONNECTED,
};

/**
 * @brief Internal struct for an instance of an Edgehog device.
 *
 * @warning Users should not modify the content of this struct directly.
 */
struct edgehog_device
{
    /** @brief Edgehog device state. */
    enum device_states state;
    /** @brief This flag marks if the initial publish has been performed. */
    bool initial_publish;
    /** @brief Handle of an Astarte device. */
    astarte_device_handle_t astarte_device;
    /** @brief The last returned error from Astarte. */
    astarte_result_t astarte_error;
    /** @brief User event callback for custom Astarte interfaces. */
    edgehog_device_event_cbk_t user_event_cbk;
    /** @brief User data context pointer for user_event_cbk. */
    void *user_cbk_user_data;
    /** @brief Zephyr thread for the Edgehog worker. */
    struct k_thread worker_thread;
    /** @brief Stack for the Edgehog worker thread. */
    K_KERNEL_STACK_MEMBER(worker_thread_stack, CONFIG_EDGEHOG_DEVICE_WORKER_THREAD_STACK_SIZE);
    /** @brief Zephyr event group for thread synchronization and signaling. */
    struct k_event events;
    /** @brief UUID representing the Boot Id. */
    char boot_id[UUID_STR_LEN];
    /** @brief OTA thread data used during the OTA Update operation. */
    ota_thread_t ota_thread;
    /** @brief LED thread data used during the LED blink. */
    led_thread_t led_thread;
    /** @brief Telemetry data. */
    edgehog_telemetry_t *telemetry;
    /** @brief File transfer data. */
    edgehog_ft_t *file_transfer;
    /** @brief Semaphore used to synchronize an OTA or File Transfer operation. */
    struct k_sem sync_ota_ft_sem;
    /** @brief User-provided storage partitions for telemetry. */
    edgehog_storage_partition_t *storage_partitions;
    /** @brief Length of user-provided storage partitions. */
    size_t storage_partitions_len;
#ifdef CONFIG_WIFI
    /** @brief WiFi scan data struct. */
    struct wifi_scan wifi_scan_data;
#endif
};

#endif // EDGEHOG_PRIVATE_H
