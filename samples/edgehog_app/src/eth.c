/*
 * (C) Copyright 2024, SECO Mind Srl
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "eth.h"

#include <zephyr/kernel.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/net/ethernet_mgmt.h>
#include <zephyr/version.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(eth, CONFIG_APP_LOG_LEVEL); // NOLINT

/************************************************
 *        Constants, statics and defines        *
 ***********************************************/

// NOLINTBEGIN(cppcoreguidelines-avoid-non-const-global-variables)
static struct net_mgmt_event_callback eth_cb;
static struct net_mgmt_event_callback status_cb;
static struct net_mgmt_event_callback ipv6_cb;
static struct net_mgmt_event_callback ipv4_cb;
static struct net_mgmt_event_callback l4_cb;

static K_SEM_DEFINE(ipv4_address_obtained, 0, 1);
// NOLINTEND(cppcoreguidelines-avoid-non-const-global-variables)

/************************************************
 *       Callbacks declaration/definition       *
 ***********************************************/

#if (KERNEL_VERSION_MAJOR >= 4) && (KERNEL_VERSION_MINOR >= 2)
static void eth_mgmt_event_handler(
    struct net_mgmt_event_callback *event_cb, uint64_t mgmt_event, struct net_if *iface)
#else
static void eth_mgmt_event_handler(
    struct net_mgmt_event_callback *event_cb, uint32_t mgmt_event, struct net_if *iface)
#endif
{
    (void) event_cb;
    (void) iface;

    // NOLINTBEGIN(hicpp-signed-bitwise)
    switch (mgmt_event) {
        case NET_EVENT_ETHERNET_CARRIER_ON:
            LOG_DBG("Ethernet event: NET_EVENT_ETHERNET_CARRIER_ON."); // NOLINT
            break;

        case NET_EVENT_ETHERNET_CARRIER_OFF:
            LOG_DBG("Ethernet event: NET_EVENT_ETHERNET_CARRIER_OFF."); // NOLINT
            break;

        case NET_EVENT_ETHERNET_VLAN_TAG_ENABLED:
            LOG_DBG("Ethernet event: NET_EVENT_ETHERNET_VLAN_TAG_ENABLED."); // NOLINT
            break;

        case NET_EVENT_ETHERNET_VLAN_TAG_DISABLED:
            LOG_DBG("Ethernet event: NET_EVENT_ETHERNET_VLAN_TAG_DISABLED."); // NOLINT
            break;

        default:
#if (KERNEL_VERSION_MAJOR >= 4) && (KERNEL_VERSION_MINOR >= 2)
            LOG_DBG("Status network event: %lld.", mgmt_event); // NOLINT
#else
            LOG_DBG("Status network event: %d.", mgmt_event); // NOLINT
#endif
            break;
    }
    // NOLINTEND(hicpp-signed-bitwise)
}

#if (KERNEL_VERSION_MAJOR >= 4) && (KERNEL_VERSION_MINOR >= 2)
static void status_mgmt_event_handler(
    struct net_mgmt_event_callback *event_cb, uint64_t mgmt_event, struct net_if *iface)
#else
static void status_mgmt_event_handler(
    struct net_mgmt_event_callback *event_cb, uint32_t mgmt_event, struct net_if *iface)
#endif
{
    (void) event_cb;
    (void) iface;

    // NOLINTBEGIN(hicpp-signed-bitwise)
    switch (mgmt_event) {
        case NET_EVENT_IF_DOWN:
            LOG_DBG("Network event: NET_EVENT_IF_DOWN."); // NOLINT
            break;

        case NET_EVENT_IF_UP:
            LOG_DBG("Network event: NET_EVENT_IF_UP."); // NOLINT
            break;

        case NET_EVENT_IF_ADMIN_DOWN:
            LOG_DBG("Network event: NET_EVENT_IF_ADMIN_DOWN."); // NOLINT
            break;

        case NET_EVENT_IF_ADMIN_UP:
            LOG_DBG("Network event: NET_EVENT_IF_ADMIN_UP."); // NOLINT
            break;

        default:
#if (KERNEL_VERSION_MAJOR >= 4) && (KERNEL_VERSION_MINOR >= 2)
            LOG_DBG("Status network event: %lld.", mgmt_event); // NOLINT
#else
            LOG_DBG("Status network event: %d.", mgmt_event); // NOLINT
#endif
            break;
    }
    // NOLINTEND(hicpp-signed-bitwise)
}

#if (KERNEL_VERSION_MAJOR >= 4) && (KERNEL_VERSION_MINOR >= 2)
static void ipv6_mgmt_event_handler(
    struct net_mgmt_event_callback *event_cb, uint64_t mgmt_event, struct net_if *iface)
#else
static void ipv6_mgmt_event_handler(
    struct net_mgmt_event_callback *event_cb, uint32_t mgmt_event, struct net_if *iface)
#endif
{
    (void) event_cb;
    (void) iface;

    // NOLINTBEGIN(hicpp-signed-bitwise)
    switch (mgmt_event) {

        case NET_EVENT_IPV6_ADDR_ADD:
            LOG_DBG("IPv6 network event: NET_EVENT_IPV6_ADDR_ADD."); // NOLINT
            break;

        case NET_EVENT_IPV6_ADDR_DEL:
            LOG_DBG("IPv6 network event: NET_EVENT_IPV6_ADDR_DEL."); // NOLINT
            break;

        case NET_EVENT_IPV6_MADDR_ADD:
            LOG_DBG("IPv6 network event: NET_EVENT_IPV6_MADDR_ADD."); // NOLINT
            break;

        case NET_EVENT_IPV6_MADDR_DEL:
            LOG_DBG("IPv6 network event: NET_EVENT_IPV6_MADDR_DEL."); // NOLINT
            break;

        case NET_EVENT_IPV6_PREFIX_ADD:
            LOG_DBG("IPv6 network event: NET_EVENT_IPV6_PREFIX_ADD."); // NOLINT
            break;

        case NET_EVENT_IPV6_PREFIX_DEL:
            LOG_DBG("IPv6 network event: NET_EVENT_IPV6_PREFIX_DEL."); // NOLINT
            break;

        case NET_EVENT_IPV6_MCAST_JOIN:
            LOG_DBG("IPv6 network event: NET_EVENT_IPV6_MCAST_JOIN."); // NOLINT
            break;

        case NET_EVENT_IPV6_MCAST_LEAVE:
            LOG_DBG("IPv6 network event: NET_EVENT_IPV6_MCAST_LEAVE."); // NOLINT
            break;

        case NET_EVENT_IPV6_ROUTER_ADD:
            LOG_DBG("IPv6 network event: NET_EVENT_IPV6_ROUTER_ADD."); // NOLINT
            break;

        case NET_EVENT_IPV6_ROUTER_DEL:
            LOG_DBG("IPv6 network event: NET_EVENT_IPV6_ROUTER_DEL."); // NOLINT
            break;

        case NET_EVENT_IPV6_ROUTE_ADD:
            LOG_DBG("IPv6 network event: NET_EVENT_IPV6_ROUTE_ADD."); // NOLINT
            break;

        case NET_EVENT_IPV6_ROUTE_DEL:
            LOG_DBG("IPv6 network event: NET_EVENT_IPV6_ROUTE_DEL."); // NOLINT
            break;

        case NET_EVENT_IPV6_DAD_SUCCEED:
            LOG_DBG("IPv6 network event: NET_EVENT_IPV6_DAD_SUCCEED."); // NOLINT
            break;

        case NET_EVENT_IPV6_DAD_FAILED:
            LOG_DBG("IPv6 network event: NET_EVENT_IPV6_DAD_FAILED."); // NOLINT
            break;

        case NET_EVENT_IPV6_NBR_ADD:
            LOG_DBG("IPv6 network event: NET_EVENT_IPV6_NBR_ADD."); // NOLINT
            break;

        case NET_EVENT_IPV6_NBR_DEL:
            LOG_DBG("IPv6 network event: NET_EVENT_IPV6_NBR_DEL."); // NOLINT
            break;

        case NET_EVENT_IPV6_DHCP_START:
            LOG_DBG("IPv6 network event: NET_EVENT_IPV6_DHCP_START."); // NOLINT
            break;

        case NET_EVENT_IPV6_DHCP_BOUND:
            LOG_DBG("IPv6 network event: NET_EVENT_IPV6_DHCP_BOUND."); // NOLINT
            break;

        case NET_EVENT_IPV6_DHCP_STOP:
            LOG_DBG("IPv6 network event: NET_EVENT_IPV6_DHCP_STOP."); // NOLINT
            break;

        default:
#if (KERNEL_VERSION_MAJOR >= 4) && (KERNEL_VERSION_MINOR >= 2)
            LOG_DBG("IPv6 network event: %lld.", mgmt_event); // NOLINT
#else
            LOG_DBG("IPv6 network event: %d.", mgmt_event); // NOLINT
#endif
            break;
    }
    // NOLINTEND(hicpp-signed-bitwise)
}

#if (KERNEL_VERSION_MAJOR >= 4) && (KERNEL_VERSION_MINOR >= 2)
static void ipv4_mgmt_event_handler(
    struct net_mgmt_event_callback *event_cb, uint64_t mgmt_event, struct net_if *iface)
#else
static void ipv4_mgmt_event_handler(
    struct net_mgmt_event_callback *event_cb, uint32_t mgmt_event, struct net_if *iface)
#endif
{
    (void) event_cb;
    (void) iface;

    // NOLINTBEGIN(hicpp-signed-bitwise)
    switch (mgmt_event) {

        case NET_EVENT_IPV4_ADDR_ADD:
            LOG_DBG("Network event: NET_EVENT_IPV4_ADDR_ADD."); // NOLINT
            k_sem_give(&ipv4_address_obtained);
            break;

        case NET_EVENT_IPV4_ADDR_DEL:
            LOG_DBG("Network event: NET_EVENT_IPV4_ADDR_DEL."); // NOLINT
            k_sem_take(&ipv4_address_obtained, K_NO_WAIT);
            break;

        case NET_EVENT_IPV4_MADDR_ADD:
            LOG_DBG("Network event: NET_EVENT_IPV4_MADDR_ADD."); // NOLINT
            break;

        case NET_EVENT_IPV4_MADDR_DEL:
            LOG_DBG("Network event: NET_EVENT_IPV4_MADDR_DEL."); // NOLINT
            break;

        case NET_EVENT_IPV4_ROUTER_ADD:
            LOG_DBG("Network event: NET_EVENT_IPV4_ROUTER_ADD."); // NOLINT
            break;

        case NET_EVENT_IPV4_ROUTER_DEL:
            LOG_DBG("Network event: NET_EVENT_IPV4_ROUTER_DEL."); // NOLINT
            break;

        case NET_EVENT_IPV4_DHCP_START:
            LOG_DBG("Network event: NET_EVENT_IPV4_DHCP_START."); // NOLINT
            break;

        case NET_EVENT_IPV4_DHCP_BOUND:
            LOG_DBG("Network event: NET_EVENT_IPV4_DHCP_BOUND."); // NOLINT
            break;

        case NET_EVENT_IPV4_DHCP_STOP:
            LOG_DBG("Network event: NET_EVENT_IPV4_DHCP_STOP."); // NOLINT
            break;

        case NET_EVENT_IPV4_MCAST_JOIN:
            LOG_DBG("Network event: NET_EVENT_IPV4_MCAST_JOIN."); // NOLINT
            break;

        case NET_EVENT_IPV4_MCAST_LEAVE:
            LOG_DBG("Network event: NET_EVENT_IPV4_MCAST_LEAVE."); // NOLINT
            break;

        default:
#if (KERNEL_VERSION_MAJOR >= 4) && (KERNEL_VERSION_MINOR >= 2)
            LOG_DBG("Network event: %lld.", mgmt_event); // NOLINT
#else
            LOG_DBG("Network event: %d.", mgmt_event); // NOLINT
#endif
            break;
    }
    // NOLINTEND(hicpp-signed-bitwise)
}

#if (KERNEL_VERSION_MAJOR >= 4) && (KERNEL_VERSION_MINOR >= 2)
static void l4_mgmt_event_handler(
    struct net_mgmt_event_callback *event_cb, uint64_t mgmt_event, struct net_if *iface)
#else
static void l4_mgmt_event_handler(
    struct net_mgmt_event_callback *event_cb, uint32_t mgmt_event, struct net_if *iface)
#endif
{
    (void) event_cb;
    (void) iface;

    // NOLINTBEGIN(hicpp-signed-bitwise)
    switch (mgmt_event) {

        case NET_EVENT_L4_CONNECTED:
            LOG_DBG("Network event: NET_EVENT_L4_CONNECTED."); // NOLINT
            break;

        case NET_EVENT_L4_DISCONNECTED:
            LOG_DBG("Network event: NET_EVENT_L4_DISCONNECTED."); // NOLINT
            break;

        case NET_EVENT_DNS_SERVER_ADD:
            LOG_DBG("Network event: NET_EVENT_DNS_SERVER_ADD."); // NOLINT
            break;

        case NET_EVENT_DNS_SERVER_DEL:
            LOG_DBG("Network event: NET_EVENT_DNS_SERVER_DEL."); // NOLINT
            break;

        case NET_EVENT_HOSTNAME_CHANGED:
            LOG_DBG("Network event: NET_EVENT_HOSTNAME_CHANGED."); // NOLINT
            break;

        default:
#if (KERNEL_VERSION_MAJOR >= 4) && (KERNEL_VERSION_MINOR >= 2)
            LOG_DBG("Network event: %lld.", mgmt_event); // NOLINT
#else
            LOG_DBG("Network event: %d.", mgmt_event); // NOLINT
#endif
            break;
    }
    // NOLINTEND(hicpp-signed-bitwise)
}

/************************************************
 * Global functions definition
 ***********************************************/

int eth_connect(void)
{
    LOG_INF("Connecting through ethernet..."); // NOLINT

    // NOLINTBEGIN(hicpp-signed-bitwise, misc-redundant-expression)
    net_mgmt_init_event_callback(&eth_cb, eth_mgmt_event_handler,
        NET_EVENT_ETHERNET_CARRIER_ON | NET_EVENT_ETHERNET_CARRIER_OFF
            | NET_EVENT_ETHERNET_VLAN_TAG_ENABLED | NET_EVENT_ETHERNET_VLAN_TAG_DISABLED);
    net_mgmt_init_event_callback(&status_cb, status_mgmt_event_handler,
        NET_EVENT_IF_DOWN | NET_EVENT_IF_UP | NET_EVENT_IF_ADMIN_DOWN | NET_EVENT_IF_ADMIN_UP);
    net_mgmt_init_event_callback(&ipv6_cb, ipv6_mgmt_event_handler,
        NET_EVENT_IPV6_ADDR_ADD | NET_EVENT_IPV6_ADDR_DEL | NET_EVENT_IPV6_MADDR_ADD
            | NET_EVENT_IPV6_MADDR_DEL | NET_EVENT_IPV6_PREFIX_ADD | NET_EVENT_IPV6_PREFIX_DEL
            | NET_EVENT_IPV6_MCAST_JOIN | NET_EVENT_IPV6_MCAST_LEAVE | NET_EVENT_IPV6_ROUTER_ADD
            | NET_EVENT_IPV6_ROUTER_DEL | NET_EVENT_IPV6_ROUTE_ADD | NET_EVENT_IPV6_ROUTE_DEL
            | NET_EVENT_IPV6_DAD_SUCCEED | NET_EVENT_IPV6_DAD_FAILED | NET_EVENT_IPV6_NBR_ADD
            | NET_EVENT_IPV6_NBR_DEL | NET_EVENT_IPV6_DHCP_START | NET_EVENT_IPV6_DHCP_BOUND
            | NET_EVENT_IPV6_DHCP_STOP);
    net_mgmt_init_event_callback(&ipv4_cb, ipv4_mgmt_event_handler,
        NET_EVENT_IPV4_ADDR_ADD | NET_EVENT_IPV4_ADDR_DEL | NET_EVENT_IPV4_MADDR_ADD
            | NET_EVENT_IPV4_MADDR_DEL | NET_EVENT_IPV4_ROUTER_ADD | NET_EVENT_IPV4_ROUTER_DEL
            | NET_EVENT_IPV4_DHCP_START | NET_EVENT_IPV4_DHCP_BOUND | NET_EVENT_IPV4_DHCP_STOP
            | NET_EVENT_IPV4_MCAST_JOIN | NET_EVENT_IPV4_MCAST_LEAVE);
    net_mgmt_init_event_callback(&l4_cb, l4_mgmt_event_handler,
        NET_EVENT_L4_CONNECTED | NET_EVENT_L4_DISCONNECTED | NET_EVENT_DNS_SERVER_ADD
            | NET_EVENT_DNS_SERVER_DEL | NET_EVENT_HOSTNAME_CHANGED);
    // NOLINTEND(hicpp-signed-bitwise, misc-redundant-expression)

    net_mgmt_add_event_callback(&eth_cb);
    net_mgmt_add_event_callback(&status_cb);
    net_mgmt_add_event_callback(&ipv6_cb);
    net_mgmt_add_event_callback(&ipv4_cb);
    net_mgmt_add_event_callback(&l4_cb);

    struct net_if *iface = net_if_get_default();
    if (!iface) {
        LOG_ERR("Default interface non existant."); // NOLINT
        return -1;
    }

    LOG_INF("Waiting for Ethernet interface to be operational."); // NOLINT
    while (net_if_oper_state(iface) != NET_IF_OPER_UP) {
        k_sleep(K_MSEC(200));
    }

    const struct device *dev = net_if_get_device(iface);
    LOG_INF("Default network interface device name: %s.", dev->name); // NOLINT
    struct ethernet_context *eth_ctx = net_if_l2_data(iface);
    LOG_INF("Ethernet carrier: %s.", eth_ctx->is_net_carrier_up ? "UP" : "DOWN"); // NOLINT
    LOG_INF("Ethernet context is initialized: %s.", eth_ctx->is_init ? "YES" : "NO"); // NOLINT
    enum net_if_oper_state iface_oper_state = net_if_oper_state(iface);
    LOG_INF("Default network interface operational state: %d.", iface_oper_state); // NOLINT

#ifdef CONFIG_NET_DHCPV4
    net_dhcpv4_start(iface);

    LOG_INF("Waiting for an IPv4 address (DHCP)."); // NOLINT
    while (k_sem_count_get(&ipv4_address_obtained) == 0) {
        k_sleep(K_MSEC(200));
    }
#endif

    k_sleep(K_MSEC(500));

    LOG_INF("Ready..."); // NOLINT

    return 0;
}

void eth_poll(void)
{
    struct net_if *iface = net_if_get_default();
    if (!iface) {
        LOG_ERR("Default interface non existant."); // NOLINT
        return;
    }

    // Check if connected
    while (net_if_oper_state(iface) != NET_IF_OPER_UP) {
        LOG_WRN("Ethernet interface is non operational."); // NOLINT
        k_sleep(K_SECONDS(1));
    }

// Restart DHCP if required.
#ifdef CONFIG_NET_DHCPV4
    if (k_sem_count_get(&ipv4_address_obtained) == 0) {
        LOG_WRN("Missing IPv4 address."); // NOLINT
        net_dhcpv4_restart(iface);
        LOG_INF("Waiting for an IPv4 address (DHCP)."); // NOLINT
        while (k_sem_count_get(&ipv4_address_obtained) == 0) {
            k_sleep(K_MSEC(200));
        }
        LOG_INF("Ready..."); // NOLINT
    }
#endif

    k_sleep(K_MSEC(500));
}
