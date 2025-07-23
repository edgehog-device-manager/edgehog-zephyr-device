/*
 * (C) Copyright 2024-2025, SECO Mind Srl
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "wifi_scan.h"

#ifdef CONFIG_WIFI

#include <zephyr/kernel.h>
#include <zephyr/net/wifi_mgmt.h>

#include <astarte_device_sdk/device.h>

#include "edgehog_private.h"
#include "generated_interfaces.h"
#include "system_time.h"

#include "log.h"
EDGEHOG_LOG_MODULE_REGISTER(wifi_scan, CONFIG_EDGEHOG_DEVICE_WIFI_SCAN_LOG_LEVEL);

#define WIFI_SCAN_EVENTS (NET_EVENT_WIFI_SCAN_RESULT | NET_EVENT_WIFI_SCAN_DONE)

#define WIFI_SCAN_ACTIVE_TIME_MS 120

#define WIFI_SCAN_THREAD_STACK_SIZE 2048
#define WIFI_SCAN_THREAD_PRIORITY 5
#define WIFI_SCAN_THREAD_START_BIT (1)
#define WIFI_SCAN_THREAD_STOP_BIT (2)
#define WIFI_SCAN_THREAD_KILL_BIT (3)
#define WIFI_SCAN_MSGQ_GET_TIMEOUT_MS 100

#define WIFI_SCAN_TIMEOUT_S 10

// NOLINTBEGIN(cppcoreguidelines-avoid-non-const-global-variables)
K_THREAD_STACK_DEFINE(wifi_scan_stack_area, WIFI_SCAN_THREAD_STACK_SIZE);
// NOLINTEND(cppcoreguidelines-avoid-non-const-global-variables)

/************************************************
 * Static functions declaration
 ***********************************************/

/**
 * @brief Handle a WiFi scan done event.
 *
 * @param cb Networking management event callback data struct.
 */
static void handle_wifi_scan_done(edgehog_device_handle_t edgehog_device);
/**
 * @brief Convert a MAC address to a string.
 *
 * @param mac Input mac address as a uint8_t array.
 * @param mac_len Size of the @p mac_len array.
 * @param str Buffer to fill with the formatter string.
 * @result NULL when the operation failed, otherwise the @p str parameter.
 */
static const char *mac_to_string(
    const uint8_t *mac, size_t mac_len, char str[static WIFI_SCAN_MAC_STRING_SIZE]);
/**
 * @brief Check if provided MAC address corresponds to the AP the device is connected to.
 *
 * @param iface Interface for the event.
 * @param ap_mac_str MAC address of an access point to check.
 * @result True if the provided MAC address is the same of the AP the device is connected to.
 * False otherwise.
 */
static bool is_connected_to_ap(
    struct net_if *iface, char ap_mac_str[static WIFI_SCAN_MAC_STRING_SIZE]);
/**
 * @brief Transmit a single WiFi scan result to Astarte.
 *
 * @param data The information to transmit.
 * @param astarte_device The Astarte device to use to transmit the data.
 */
static void transmit_wifi_scan_result(
    struct wifi_scan_result_data data, astarte_device_handle_t astarte_device);
/**
 * @brief Handle a WiFi scan result event.
 *
 * @param iface Interface for the event.
 * @param edgehog_device Edgehog device connected to the scan request.
 */
static void handle_wifi_scan_result(struct net_if *iface, edgehog_device_handle_t edgehog_device);

/************************************************
 *       Callbacks declaration/definition       *
 ***********************************************/

static void wifi_mgmt_scan_event_handler(
    struct net_mgmt_event_callback *cb, uint64_t mgmt_event, struct net_if *iface)
{
    struct wifi_scan *wifi_scan_handle = CONTAINER_OF(cb, struct wifi_scan, wifi_scan_cb);
    edgehog_device_handle_t device
        = CONTAINER_OF(wifi_scan_handle, struct edgehog_device, wifi_scan_data);

    switch (mgmt_event) {
        case NET_EVENT_WIFI_SCAN_RESULT:
            EDGEHOG_LOG_DBG("Event WiFi scan result received");
            handle_wifi_scan_result(iface, device);
            break;
        case NET_EVENT_WIFI_SCAN_DONE:
            EDGEHOG_LOG_DBG("Event WiFi scan done received");
            handle_wifi_scan_done(device);
            break;
        default:
            break;
    }
}

static void scan_timeout_handler(struct k_timer *timer)
{
    EDGEHOG_LOG_WRN("WiFi scan timed out. Forcing completion.");
    struct wifi_scan *wifi_scan_data = (struct wifi_scan *) k_timer_user_data_get(timer);
    net_mgmt_del_event_callback(&wifi_scan_data->wifi_scan_cb);
    atomic_set_bit(&wifi_scan_data->thread_state, WIFI_SCAN_THREAD_STOP_BIT);
}

static void wifi_scan_thread_entry_point(void *device_ptr, void *queue_ptr, void *unused)
{
    ARG_UNUSED(unused);
    EDGEHOG_LOG_DBG("WiFi scan thread started");

    edgehog_device_handle_t device = (edgehog_device_handle_t) device_ptr;
    struct wifi_scan *wifi_scan_data = &device->wifi_scan_data;
    struct k_msgq *msgq = (struct k_msgq *) queue_ptr;
    struct wifi_scan_result_data data = { 0 };

    while ((!atomic_test_bit(&wifi_scan_data->thread_state, WIFI_SCAN_THREAD_STOP_BIT)
               || (k_msgq_num_used_get(msgq) > 0))
        && (!atomic_test_bit(&wifi_scan_data->thread_state, WIFI_SCAN_THREAD_KILL_BIT))) {
        if (k_msgq_get(msgq, &data, K_MSEC(WIFI_SCAN_MSGQ_GET_TIMEOUT_MS)) == 0) {
            transmit_wifi_scan_result(data, device->astarte_device);
        }
    }

    EDGEHOG_LOG_DBG("WiFi scan thread terminated");
    atomic_clear_bit(&wifi_scan_data->thread_state, WIFI_SCAN_THREAD_STOP_BIT);
    atomic_clear_bit(&wifi_scan_data->thread_state, WIFI_SCAN_THREAD_KILL_BIT);
    atomic_clear_bit(&wifi_scan_data->thread_state, WIFI_SCAN_THREAD_START_BIT);
}

/************************************************
 * Global functions definition
 ***********************************************/

edgehog_result_t edgehog_wifi_scan_init(edgehog_device_handle_t edgehog_device)
{
    EDGEHOG_LOG_DBG("Initializing WiFi scan driver");
    struct wifi_scan *wifi_scan_data = &edgehog_device->wifi_scan_data;

    net_mgmt_init_event_callback(
        &wifi_scan_data->wifi_scan_cb, wifi_mgmt_scan_event_handler, WIFI_SCAN_EVENTS);

    k_msgq_init(&wifi_scan_data->msgq, wifi_scan_data->msgq_buffer,
        sizeof(struct wifi_scan_result_data), WIFI_SCAN_MAX_SCAN_RESULT);

    k_timer_init(&wifi_scan_data->timeout_timer, scan_timeout_handler, NULL);
    k_timer_user_data_set(&wifi_scan_data->timeout_timer, wifi_scan_data);

    atomic_clear_bit(&wifi_scan_data->thread_state, WIFI_SCAN_THREAD_START_BIT);
    atomic_clear_bit(&wifi_scan_data->thread_state, WIFI_SCAN_THREAD_STOP_BIT);
    atomic_clear_bit(&wifi_scan_data->thread_state, WIFI_SCAN_THREAD_KILL_BIT);

    return EDGEHOG_RESULT_OK;
}

edgehog_result_t edgehog_wifi_scan_destroy(
    edgehog_device_handle_t edgehog_device, k_timeout_t timeout)
{
    EDGEHOG_LOG_DBG("Destroying WiFi scan driver");
    struct wifi_scan *wifi_scan_data = &edgehog_device->wifi_scan_data;
    atomic_set_bit(&wifi_scan_data->thread_state, WIFI_SCAN_THREAD_KILL_BIT);

    int res = k_thread_join(&wifi_scan_data->thread, timeout);
    switch (res) {
        case 0:
            return EDGEHOG_RESULT_OK;
        case -EAGAIN:
            return EDGEHOG_RESULT_TELEMETRY_STOP_TIMEOUT;
        default:
            return EDGEHOG_RESULT_INTERNAL_ERROR;
    }
}

edgehog_result_t edgehog_wifi_scan_start(edgehog_device_handle_t edgehog_device)
{
    EDGEHOG_LOG_DBG("Starting a new WiFi scan");
    struct wifi_scan *wifi_scan_data = &edgehog_device->wifi_scan_data;

    if (atomic_test_and_set_bit(&wifi_scan_data->thread_state, WIFI_SCAN_THREAD_START_BIT)) {
        EDGEHOG_LOG_ERR("Failed starting wifi scan as one is already been executed");
        return EDGEHOG_RESULT_WIFI_SCAN_REQUEST_FAIL;
    }

    struct net_if *iface = net_if_get_wifi_sta();
    if (!iface) {
        EDGEHOG_LOG_ERR("Failed getting interface for the WiFi station");
        atomic_clear_bit(&wifi_scan_data->thread_state, WIFI_SCAN_THREAD_START_BIT);
        return EDGEHOG_RESULT_WIFI_SCAN_REQUEST_FAIL;
    }

    net_mgmt_add_event_callback(&wifi_scan_data->wifi_scan_cb);

    struct wifi_scan_params params = {
        .scan_type = WIFI_SCAN_TYPE_ACTIVE,
        .bands = WIFI_FREQ_BAND_2_4_GHZ | WIFI_FREQ_BAND_5_GHZ,
        .dwell_time_active = WIFI_SCAN_ACTIVE_TIME_MS,
        .max_bss_cnt = WIFI_SCAN_MAX_SCAN_RESULT,
    };
    if (net_mgmt(NET_REQUEST_WIFI_SCAN, iface, &params, sizeof(params))) {
        EDGEHOG_LOG_ERR("WiFi Scan request failed");
        atomic_clear_bit(&wifi_scan_data->thread_state, WIFI_SCAN_THREAD_START_BIT);
        return EDGEHOG_RESULT_WIFI_SCAN_REQUEST_FAIL;
    }

    k_tid_t thread_id = k_thread_create(&wifi_scan_data->thread, wifi_scan_stack_area,
        WIFI_SCAN_THREAD_STACK_SIZE, wifi_scan_thread_entry_point, (void *) edgehog_device,
        (void *) &wifi_scan_data->msgq, NULL, WIFI_SCAN_THREAD_PRIORITY, 0, K_NO_WAIT);
    if (!thread_id) {
        EDGEHOG_LOG_ERR("Unable to start wifi scan thread");
        atomic_clear_bit(&wifi_scan_data->thread_state, WIFI_SCAN_THREAD_START_BIT);
        return EDGEHOG_RESULT_WIFI_SCAN_REQUEST_FAIL;
    }
    k_timer_start(&wifi_scan_data->timeout_timer, K_SECONDS(WIFI_SCAN_TIMEOUT_S), K_NO_WAIT);

    EDGEHOG_LOG_DBG("WiFi scan started");
    return EDGEHOG_RESULT_OK;
}

/************************************************
 *         Static functions definitions         *
 ***********************************************/

static void handle_wifi_scan_done(edgehog_device_handle_t edgehog_device)
{
    struct wifi_scan *wifi_scan_data = &edgehog_device->wifi_scan_data;
    struct net_mgmt_event_callback *cb = &wifi_scan_data->wifi_scan_cb;
    const struct wifi_status *status = (const struct wifi_status *) cb->info;

    if (status->status) {
        EDGEHOG_LOG_ERR("Scan request failed (%d)", status->status);
    } else {
        EDGEHOG_LOG_DBG("Scan request done");
    }

    net_mgmt_del_event_callback(cb);
    k_timer_stop(&wifi_scan_data->timeout_timer);
    atomic_set_bit(&wifi_scan_data->thread_state, WIFI_SCAN_THREAD_STOP_BIT);
}

static const char *mac_to_string(
    const uint8_t *mac, size_t mac_len, char str[static WIFI_SCAN_MAC_STRING_SIZE])
{
    if (mac_len != 6) {
        return NULL;
    }
    int snprintf_rc = snprintf(str, WIFI_SCAN_MAC_STRING_SIZE, "%02x:%02x:%02x:%02x:%02x:%02x",
        mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    if (snprintf_rc != WIFI_SCAN_MAC_STRING_SIZE - 1) {
        return NULL;
    }
    return str;
}

static bool is_connected_to_ap(
    struct net_if *iface, char ap_mac_str[static WIFI_SCAN_MAC_STRING_SIZE])
{
    struct wifi_iface_status status = { 0 };

    if (net_mgmt(NET_REQUEST_WIFI_IFACE_STATUS, iface, &status, sizeof(struct wifi_iface_status))) {
        EDGEHOG_LOG_ERR("WiFi Status Request Failed");
        return false;
    }

    char connected_ap_mac_str[WIFI_SCAN_MAC_STRING_SIZE] = { 0 };
    if (!mac_to_string(status.bssid, WIFI_MAC_ADDR_LEN, connected_ap_mac_str)) {
        EDGEHOG_LOG_ERR("WiFi scan request MAC conversion to string error in is connected");
        return false;
    }

    return strcmp(connected_ap_mac_str, ap_mac_str) == 0;
}

static void transmit_wifi_scan_result(
    struct wifi_scan_result_data data, astarte_device_handle_t astarte_device)
{
    EDGEHOG_LOG_DBG("Streaming scan result");
    int64_t timestamp_ms = 0;
    system_time_current_ms(&timestamp_ms);

    astarte_object_entry_t object_entries[] = {
        { .path = "channel", .data = astarte_data_from_integer(data.channel) },
        { .path = "essid", .data = astarte_data_from_string(data.essid) },
        { .path = "macAddress", .data = astarte_data_from_string(data.mac_addr) },
        { .path = "rssi", .data = astarte_data_from_integer(data.rssi) },
        { .path = "connected", .data = astarte_data_from_boolean(data.connected) },
    };

    astarte_result_t res
        = astarte_device_send_object(astarte_device, io_edgehog_devicemanager_WiFiScanResults.name,
            "/ap", object_entries, ARRAY_SIZE(object_entries), &timestamp_ms);
    if (res != ASTARTE_RESULT_OK) {
        EDGEHOG_LOG_ERR("Unable to send WiFiScanResults");
    }
}

static void handle_wifi_scan_result(struct net_if *iface, edgehog_device_handle_t edgehog_device)
{
    const struct wifi_scan_result *entry
        = (const struct wifi_scan_result *) edgehog_device->wifi_scan_data.wifi_scan_cb.info;

    struct wifi_scan_result_data scan_result = { 0 };

    scan_result.channel = entry->channel;

    strncpy(scan_result.essid, entry->ssid, sizeof(scan_result.essid) - 1);
    scan_result.essid[sizeof(scan_result.essid) - 1] = '\0';

    if (entry->mac_length < WIFI_MAC_ADDR_LEN) {
        EDGEHOG_LOG_ERR("WiFi scan request MAC length error");
        return;
    }

    if (!mac_to_string(entry->mac, WIFI_MAC_ADDR_LEN, scan_result.mac_addr)) {
        EDGEHOG_LOG_ERR("WiFi scan request MAC conversion to string error");
        return;
    }

    scan_result.rssi = entry->rssi;
    scan_result.connected = is_connected_to_ap(iface, scan_result.mac_addr);

    EDGEHOG_LOG_DBG("Chan | RSSI | MAC               | CONNECTED | (len) SSID ");
    EDGEHOG_LOG_DBG("%-4u | %-4d | %-17s | %-9d | (%-2u) %s", scan_result.channel, scan_result.rssi,
        scan_result.mac_addr, scan_result.connected, strlen(scan_result.essid), scan_result.essid);

    k_msgq_put(&edgehog_device->wifi_scan_data.msgq, &scan_result, K_NO_WAIT);
}

#endif
