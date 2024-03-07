/*
 * (C) Copyright 2024, SECO Mind Srl
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>

#if !defined(CONFIG_ASTARTE_DEVICE_SDK_DEVELOP_DISABLE_OR_IGNORE_TLS)
#include "ca_certificates.h"
#include <zephyr/net/tls_credentials.h>
#endif

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(simple_main); // NOLINT

#include <edgehog_device/edgehog_device.h>

#include <astarte_device_sdk/device.h>

#include "wifi.h"

/************************************************
 * Constants and defines
 ***********************************************/

#define HTTP_TIMEOUT_MS (3 * MSEC_PER_SEC)
#define MQTT_FIRST_POLL_TIMEOUT_MS (3 * MSEC_PER_SEC)
#define MQTT_POLL_TIMEOUT_MS 200

#define DEVICE_OPERATIONAL_TIME_MS (15 * SEC_PER_MIN * MSEC_PER_SEC)

/************************************************
 * Static functions declaration
 ***********************************************/

/**
 * @brief Handler for astarte connection events.
 *
 * @param event Astarte device connection event pointer.
 */
static void astarte_connection_events_handler(astarte_device_connection_event_t *event);
/**
 * @brief Handler for astarte disconnection events.
 *
 * @param event Astarte device disconnection event pointer.
 */
static void astarte_disconnection_events_handler(astarte_device_disconnection_event_t *event);
/**
 * @brief Handler for astarte data event.
 *
 * @param event Astarte device data event pointer.
 */
static void astarte_data_events_handler(astarte_device_data_event_t *event);

// NOLINTNEXTLINE(hicpp-function-size)
int main(void)
{
    astarte_err_t astarte_err = ASTARTE_OK;

    LOG_INF("Edgehog example! %s\n", CONFIG_BOARD); // NOLINT

    LOG_INF("Initializing WiFi driver."); // NOLINT
    wifi_init();

    k_sleep(K_SECONDS(5)); // sleep for 5 seconds

#if !defined(CONFIG_ASTARTE_DEVICE_SDK_DEVELOP_DISABLE_OR_IGNORE_TLS)
    tls_credential_add(CONFIG_ASTARTE_DEVICE_SDK_CA_CERT_TAG, TLS_CREDENTIAL_CA_CERTIFICATE,
        ca_certificate_root, sizeof(ca_certificate_root));
#endif

    char cred_secr[ASTARTE_PAIRING_CRED_SECR_LEN + 1] = CONFIG_CREDENTIAL_SECRET;

    const astarte_interface_t *interfaces[] = { 0 };

    astarte_device_config_t device_config;
    device_config.http_timeout_ms = HTTP_TIMEOUT_MS;
    device_config.mqtt_connection_timeout_ms = MQTT_FIRST_POLL_TIMEOUT_MS;
    device_config.mqtt_connected_timeout_ms = MQTT_POLL_TIMEOUT_MS;
    device_config.connection_cbk = astarte_connection_events_handler;
    device_config.disconnection_cbk = astarte_disconnection_events_handler;
    device_config.data_cbk = astarte_data_events_handler;
    device_config.interfaces = interfaces;
    device_config.interfaces_size = ARRAY_SIZE(interfaces);

    memcpy(device_config.cred_secr, cred_secr, sizeof(cred_secr));

    astarte_device_handle_t astarte_device = NULL;
    astarte_err = astarte_device_new(&device_config, &astarte_device);
    if (astarte_err != ASTARTE_OK) {
        return -1;
    }

    edgehog_device_config_t edgehog_conf = {
        .astarte_device = astarte_device,
    };

    edgehog_device_handle_t edgehog_device = edgehog_device_new(&edgehog_conf);
    if (!edgehog_device) {
        LOG_ERR("Unable to create edgehog device handle"); // NOLINT
        return -1;
    }
    LOG_INF("Edgehog device created"); // NOLINT

    astarte_err = astarte_device_connect(astarte_device);
    if (astarte_err != ASTARTE_OK) {
        return -1;
    }

    astarte_err = astarte_device_poll(astarte_device);
    if (astarte_err != ASTARTE_OK) {
        LOG_ERR("First poll should not timeout as we should receive a connection ack."); // NOLINT
        return -1;
    }

    k_timepoint_t disconnect_timepoint = sys_timepoint_calc(K_MSEC(DEVICE_OPERATIONAL_TIME_MS));
    int count = 0;
    while (1) {
        k_timepoint_t timepoint = sys_timepoint_calc(K_MSEC(MQTT_POLL_TIMEOUT_MS));

        astarte_err = astarte_device_poll(astarte_device);
        if ((astarte_err != ASTARTE_ERR_TIMEOUT) && (astarte_err != ASTARTE_OK)) {
            return -1;
        }

        if (++count % (CONFIG_SLEEP_MS / MQTT_POLL_TIMEOUT_MS) == 0) {
            LOG_INF("Hello world! %s", CONFIG_BOARD); // NOLINT
            count = 0;
        }
        k_sleep(sys_timepoint_timeout(timepoint));

        if (K_TIMEOUT_EQ(sys_timepoint_timeout(disconnect_timepoint), K_NO_WAIT)) {
            break;
        }
    }
    LOG_INF("End of loop, disconnection imminent."); // NOLINT

    edgehog_device_destroy(edgehog_device);

    astarte_err = astarte_device_destroy(astarte_device);
    if (astarte_err != ASTARTE_OK) {
        LOG_ERR("Failed destroying the device."); // NOLINT
        return -1;
    }

    return 0;
}

/************************************************
 * Static functions definitions
 ***********************************************/

static void astarte_connection_events_handler(astarte_device_connection_event_t *event)
{
    LOG_INF("Astarte device connected, session_present: %d", event->session_present); // NOLINT
}

static void astarte_disconnection_events_handler(astarte_device_disconnection_event_t *event)
{
    (void) event;
    LOG_INF("Astarte device disconnected"); // NOLINT
}

static void astarte_data_events_handler(astarte_device_data_event_t *event)
{
    // NOLINTNEXTLINE
    LOG_INF("Got Astarte data event, interface_name: %s, path: %s, bson_type: %d",
        event->interface_name, event->path, event->bson_element.type);
}