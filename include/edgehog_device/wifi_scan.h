/*
 * (C) Copyright 2024, SECO Mind Srl
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef EDGEHOG_DEVICE_WIFI_SCAN_H
#define EDGEHOG_DEVICE_WIFI_SCAN_H

/**
 * @file wifi_scan.h
 * @brief WiFi scan API
 */

#include "edgehog_device/device.h"

#ifdef CONFIG_WIFI
#include "edgehog_device/result.h"

#include <zephyr/net/wifi_mgmt.h>

#define EDGEHOG_WIFI_SCAN_EVENTS_SET (NET_EVENT_WIFI_SCAN_DONE | NET_EVENT_WIFI_CONNECT_RESULT)

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Perfom a wifi scan request.
 *
 * @return EDGEHOG_RESULT_OK if successful, otherwise an error code.
 */
edgehog_result_t edgehog_wifi_scan_start();

/**
 * @brief Handle received WiFi scan event.
 *
 * @details This function handles an WiFi scan event generate from network management.
 *
 * @param mgmt_event The exact request value the handler is being called through.
 * @param iface A valid pointer on struct net_if if the request is meant to be tight to a network
 * interface. NULL otherwise.
 * @param data A valid pointer on a data understood by the handler. NULL otherwise.
 * @param len Length in byte of the memory pointed by data.
 * @param user_data Data provided by the user to the handler.
 */
void edgehog_wifi_scan_event_handler(
    uint32_t mgmt_event, struct net_if *iface, void *data, size_t data_len, void *user_data);

#ifdef __cplusplus
}
#endif
#endif

#endif // EDGEHOG_DEVICE_WIFI_SCAN_H
