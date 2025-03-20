/*
 * (C) Copyright 2024, SECO Mind Srl
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "edgehog_device/wifi_scan.h"

#include "generated_interfaces.h"

#ifdef CONFIG_WIFI

#include "system_time.h"

#include <zephyr/net/net_event.h>
#include <zephyr/net/net_if.h>

#include <astarte_device_sdk/device.h>

#include "log.h"
EDGEHOG_LOG_MODULE_REGISTER(wifi_scan, CONFIG_EDGEHOG_DEVICE_WIFI_SCAN_LOG_LEVEL);

#define WIFI_SCAN_ACTIVE_TIME_MS 120
#define MAX_SCAN_RESULT 5
#define MAC_HEX_FORMAT_LEN 17 // sizeof("xx:xx:xx:xx:xx:xx")

/************************************************
 * Static functions declaration
 ***********************************************/

/**
 * @brief Handle received WiFi scan result.
 *
 * @details This function handles a wifi scan result publishing it to the Astarte.
 *
 * @param astarte_device A valid Astarte device handle.
 * @param scan_result A valid wifi_scan_result handle.
 * @param connected_ap_mac MacAddress of the Access Point to which the device is connected.
 *
 */
static void handle_wifi_scan_result(astarte_device_handle_t astarte_device,
    const struct wifi_scan_result *scan_result, char *connected_ap_mac);

/************************************************
 * Global functions definition
 ***********************************************/

void edgehog_wifi_scan_event_handler(
    uint32_t mgmt_event, struct net_if *iface, void *data, size_t data_len, void *user_data)
{
    switch (mgmt_event) {
        case NET_EVENT_WIFI_SCAN_RESULT: {
            const struct wifi_scan_result *scan_result = (const struct wifi_scan_result *) data;
            astarte_device_handle_t *astarte_device = (astarte_device_handle_t *) user_data;
            struct wifi_iface_status status = { 0 };

            if (net_mgmt(NET_REQUEST_WIFI_IFACE_STATUS, iface, &status,
                    sizeof(struct wifi_iface_status))) {
                LOG_INF("WiFi Status Request Failed");
            }

            uint8_t connected_ap_mac[MAC_HEX_FORMAT_LEN + 1] = { 0 };
            if (status.state >= WIFI_STATE_ASSOCIATED) {
                int snprintf_rc = snprintf(connected_ap_mac, MAC_HEX_FORMAT_LEN + 1,
                    "%02x:%02x:%02x:%02x:%02x:%02x", status.bssid[0], status.bssid[1],
                    status.bssid[2], status.bssid[3], status.bssid[4], status.bssid[5]);

                if (snprintf_rc != MAC_HEX_FORMAT_LEN) {
                    EDGEHOG_LOG_ERR(
                        "Failure in formatting the MacAddress of the connected Acces Point.");
                }
            }
            handle_wifi_scan_result(*astarte_device, scan_result, connected_ap_mac);
        } break;
        case NET_EVENT_WIFI_SCAN_DONE: {
            const struct wifi_status *status = (const struct wifi_status *) data;
            if (status->status) {
                EDGEHOG_LOG_ERR("Scan request failed (%d)", status->status);
            } else {
                EDGEHOG_LOG_DBG("Scan request done");
            }
        } break;
        default:
            break;
    }
}

edgehog_result_t edgehog_wifi_scan_start()
{
    struct net_if *iface = net_if_get_first_wifi();
    struct wifi_scan_params params = {
        .scan_type = WIFI_SCAN_TYPE_ACTIVE,
        .bands = WIFI_FREQ_BAND_2_4_GHZ | WIFI_FREQ_BAND_5_GHZ,
        .dwell_time_active = WIFI_SCAN_ACTIVE_TIME_MS,
        .max_bss_cnt = MAX_SCAN_RESULT,
    };

    if (net_mgmt(NET_REQUEST_WIFI_SCAN, iface, &params, sizeof(params))) {
        EDGEHOG_LOG_ERR("WiFi Scan request failed");
        return EDGEHOG_RESULT_WIFI_SCAN_REQUEST_FAIL;
    }

    return EDGEHOG_RESULT_OK;
}

static void handle_wifi_scan_result(astarte_device_handle_t astarte_device,
    const struct wifi_scan_result *scan_result, char *connected_ap_mac)
{
    if (scan_result->mac_length < WIFI_MAC_ADDR_LEN) {
        EDGEHOG_LOG_ERR("WiFi Scan request mac len error");
        return;
    }

    uint8_t mac_string_buf[MAC_HEX_FORMAT_LEN + 1] = { 0 };
    int snprintf_rc = snprintf(mac_string_buf, MAC_HEX_FORMAT_LEN + 1,
        "%02x:%02x:%02x:%02x:%02x:%02x", scan_result->mac[0], scan_result->mac[1],
        scan_result->mac[2], scan_result->mac[3], scan_result->mac[4], scan_result->mac[5]);

    if (snprintf_rc != MAC_HEX_FORMAT_LEN) {
        EDGEHOG_LOG_ERR("Failure in formatting the MacAddress of the Access Point.");
        return;
    }

    int64_t timestamp_ms = 0;
    system_time_current_ms(&timestamp_ms);

    bool ap_is_connected
        = connected_ap_mac != NULL && strcmp(connected_ap_mac, mac_string_buf) == 0;

    astarte_object_entry_t object_entries[] = {
        { .path = "channel", .individual = astarte_data_from_integer(scan_result->channel) },
        { .path = "essid", .individual = astarte_data_from_string(scan_result->ssid) },
        { .path = "macAddress", .individual = astarte_data_from_string(mac_string_buf) },
        { .path = "rssi", .individual = astarte_data_from_integer(scan_result->rssi) },
        { .path = "connected", .individual = astarte_individual_from_boolean(ap_is_connected) },
    };

    astarte_result_t res
        = astarte_device_send_object(astarte_device, io_edgehog_devicemanager_WiFiScanResults.name,
            "/ap", object_entries, ARRAY_SIZE(object_entries), &timestamp_ms);
    if (res != ASTARTE_RESULT_OK) {
        EDGEHOG_LOG_ERR("Unable to send WiFiScanResults"); // NOLINT
    }
}

#endif
