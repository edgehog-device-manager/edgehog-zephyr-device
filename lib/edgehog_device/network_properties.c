/*
 * (C) Copyright 2026, SECO Mind Srl
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_l2.h>

#include "network_properties.h"

#include "edgehog_private.h"
#include "nvs.h"
#include "system_time.h"

#include <astarte_device_sdk/device.h>
#include <astarte_device_sdk/result.h>

#include "generated_interfaces.h"
#include "log.h"

#define MAC_LEN 18
#define IF_NAME_LEN 16
#define MAC_ADDR_ENDPOINT_LEN (IF_NAME_LEN + 13)
#define TECH_TYPE_ENDPOINT_LEN (IF_NAME_LEN + 17)

EDGEHOG_LOG_MODULE_REGISTER(network_properties, CONFIG_EDGEHOG_DEVICE_NETWORK_PROPERTIES_LOG_LEVEL);

void get_mac_address(struct net_if *iface, char *mac)
{
    const struct net_linkaddr *link_addr = net_if_get_link_addr(iface);

    // NOLINTBEGIN
    if (link_addr && link_addr->len == 6) {
        snprintf(mac, MAC_LEN, "%02X:%02X:%02X:%02X:%02X:%02X", link_addr->addr[0],
            link_addr->addr[1], link_addr->addr[2], link_addr->addr[3], link_addr->addr[4],
            link_addr->addr[5]);
    }
    // NOLINTEND
}

const char *get_technology_type(struct net_if *iface)
{
    const struct net_l2 *l2 = net_if_l2(iface); // NOLINT(readability-identifier-length)

#if defined(CONFIG_NET_L2_ETHERNET)
    if (l2 == &NET_L2_GET_NAME(ETHERNET)) {
        return "ethernet";
    }
#endif

#if defined(CONFIG_NET_L2_WIFI)
    if (l2 == &NET_L2_GET_NAME(WIFI)) {
        return "wifi";
    }
#endif

#if defined(CONFIG_NET_L2_PPP)
    if (l2 == &NET_L2_GET_NAME(PPP)) {
        return "cellular";
    }
#endif

#if defined(CONFIG_NET_L2_IEEE802154)
    if (l2 == &NET_L2_GET_NAME(IEEE802154)) {
        return "802.15.4";
    }
#endif

#if defined(CONFIG_NET_L2_DUMMY)
    if (l2 == &NET_L2_GET_NAME(DUMMY)) {
        return "loopback";
    }
#endif

    return "unknown";
}

edgehog_result_t publish_network_properties(edgehog_device_handle_t edgehog_device)
{
    EDGEHOG_LOG_DBG("Publishing Edgehog device network properties");

    edgehog_result_t eres = EDGEHOG_RESULT_OK;

    char if_name[IF_NAME_LEN] = { 0 };
    char if_mac[MAC_LEN] = { 0 };
    char mac_addr_endpoint[MAC_ADDR_ENDPOINT_LEN] = { 0 };
    char tech_type_endpoint[TECH_TYPE_ENDPOINT_LEN] = { 0 };

    // iterate through all network interfaces
    for (int i = 1;; i++) {
        struct net_if *iface = net_if_get_by_index(i);
        // end of network interface list
        if (!iface) {
            break;
        }

        net_if_get_name(iface, if_name, sizeof(if_name));
        get_mac_address(iface, if_mac);
        const char *tech_type = get_technology_type(iface);

        // NOLINTBEGIN
        snprintf(mac_addr_endpoint, MAC_ADDR_ENDPOINT_LEN, "/%s/macAddress", if_name);
        EDGEHOG_LOG_DBG("MAC ADDR ENDPOINT: %s", mac_addr_endpoint);
        snprintf(tech_type_endpoint, TECH_TYPE_ENDPOINT_LEN, "/%s/technologyType", if_name);
        EDGEHOG_LOG_DBG("TECH TYPE ENDPOINT: %s", tech_type_endpoint);
        // NOLINTEND

        astarte_result_t ares = astarte_device_set_property(edgehog_device->astarte_device,
            io_edgehog_devicemanager_NetworkInterfaceProperties.name, mac_addr_endpoint,
            astarte_data_from_string(if_mac));

        if (ares != ASTARTE_RESULT_OK) {
            EDGEHOG_LOG_ERR("Unable to send network interface: %s", if_name);
            eres = EDGEHOG_RESULT_NETWORK_INTERFACE_SCAN_REQUEST_FAIL;
        }

        ares = astarte_device_set_property(edgehog_device->astarte_device,
            io_edgehog_devicemanager_NetworkInterfaceProperties.name, tech_type_endpoint,
            astarte_data_from_string(tech_type));

        if (ares != ASTARTE_RESULT_OK) {
            EDGEHOG_LOG_ERR("Unable to send network interface: %s", if_name);
            eres = EDGEHOG_RESULT_NETWORK_INTERFACE_SCAN_REQUEST_FAIL;
        }
    }

    return eres;
}
