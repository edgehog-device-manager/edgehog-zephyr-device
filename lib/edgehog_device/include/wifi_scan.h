/*
 * (C) Copyright 2024-2025, SECO Mind Srl
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef WIFI_SCAN_H
#define WIFI_SCAN_H

#ifdef CONFIG_WIFI

/**
 * @file wifi_scan.h
 */

/**
 * @defgroup wifi WiFi scan
 * @brief API for a WiFi scan.
 * @ingroup edgehog_device
 * @{
 */

#include "edgehog_device/device.h"
#include "edgehog_device/result.h"

#include <zephyr/kernel.h>
#include <zephyr/net/wifi_mgmt.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Maximum number of APs to detect with a single scan */
#define WIFI_SCAN_MAX_SCAN_RESULT 5
/** @brief Size of a MAC string representation, including the NULL terminator */
#define WIFI_SCAN_MAC_STRING_SIZE sizeof("xx:xx:xx:xx:xx:xx")

/** @brief Data for a single scan result event */
struct wifi_scan_result_data
{
    /** @brief Access point frequency bandwidth channel */
    uint8_t channel;
    /** @brief Human readable SSID for the access point */
    char essid[WIFI_SSID_MAX_LEN + 1];
    /** @brief Human readable MAC address for the access point */
    char mac_addr[WIFI_SCAN_MAC_STRING_SIZE];
    /** @brief RSSI for the access point */
    int8_t rssi;
    /** @brief Flag signaling if the device is connected to the access point */
    bool connected;
};

/**
 * @brief Data struct for a WiFi scan instance.
 */
struct wifi_scan
{
    /** @brief Message queue. */
    struct k_msgq msgq;
    /** @brief Ring buffer that holds queued messages. */
    char msgq_buffer[WIFI_SCAN_MAX_SCAN_RESULT * sizeof(struct wifi_scan_result_data)];
    /** @brief WiFi scan event callback. */
    struct net_mgmt_event_callback wifi_scan_cb;
    /** @brief Main thread deputated to the transmission of messages to Astarte. */
    struct k_thread thread;
    /** @brief State of the transmission thread. */
    atomic_t thread_state;
    /** @brief Scan timeout to avoid deadlocks. */
    struct k_timer timeout_timer;
};

/**
 * @brief Initialize the wifi scan module.
 *
 * @param edgehog_device The handle to the edgehog device that will manage the scans.
 * @return EDGEHOG_RESULT_OK if successful, otherwise an error code.
 */
edgehog_result_t edgehog_wifi_scan_init(edgehog_device_handle_t edgehog_device);

/**
 * @brief Destroy a wifi scan module, interrupting any scan being performed.
 *
 * @param edgehog_device The handle to the edgehog device that manages the scans.
 * @param timeout A timeout used to interrupt the internal wifi scan thread.
 * @return EDGEHOG_RESULT_OK if successful, otherwise an error code.
 */
edgehog_result_t edgehog_wifi_scan_destroy(
    edgehog_device_handle_t edgehog_device, k_timeout_t timeout);

/**
 * @brief Starte a WiFi scan request.
 *
 * @note This function will only start the scan request that will be performed in the
 * background.
 *
 * @param edgehog_device The handle to the edgehog device that requested the scan.
 * @return EDGEHOG_RESULT_OK if successful, otherwise an error code.
 */
edgehog_result_t edgehog_wifi_scan_start(edgehog_device_handle_t edgehog_device);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif

#endif // WIFI_SCAN_H
