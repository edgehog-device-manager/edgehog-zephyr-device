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

#include <astarte_device_sdk/device.h>
#include <edgehog_device/device.h>

#if defined(CONFIG_WIFI)
#include "wifi.h"
#endif

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

// NOLINTNEXTLINE(hicpp-function-size)
int main(void)
{
    astarte_result_t astarte_result = ASTARTE_RESULT_OK;

    LOG_INF("Edgehog example! %s\n", CONFIG_BOARD); // NOLINT

    LOG_INF("Initializing WiFi driver."); // NOLINT

#if defined(CONFIG_WIFI)
    LOG_INF("Initializing WiFi driver."); // NOLINT
    wifi_init();
    k_sleep(K_SECONDS(5)); // sleep for 5 seconds
#endif

#if !defined(CONFIG_ASTARTE_DEVICE_SDK_DEVELOP_DISABLE_OR_IGNORE_TLS)
    tls_credential_add(CONFIG_ASTARTE_DEVICE_SDK_CA_CERT_TAG, TLS_CREDENTIAL_CA_CERTIFICATE,
        ca_certificate_root, sizeof(ca_certificate_root));
#endif

    char cred_secr[ASTARTE_PAIRING_CRED_SECR_LEN + 1] = CONFIG_CREDENTIAL_SECRET;

    struct k_sem astarte_connection_sem;
    k_sem_init(&astarte_connection_sem, 0, 1);

    astarte_device_config_t astarte_device_config;
    memset(&astarte_device_config, 0, sizeof(astarte_device_config));
    astarte_device_config.http_timeout_ms = HTTP_TIMEOUT_MS;
    astarte_device_config.mqtt_connection_timeout_ms = MQTT_FIRST_POLL_TIMEOUT_MS;
    astarte_device_config.mqtt_connected_timeout_ms = MQTT_POLL_TIMEOUT_MS;
    astarte_device_config.connection_cbk = astarte_connection_events_handler;
    astarte_device_config.disconnection_cbk = astarte_disconnection_events_handler;
    astarte_device_config.cbk_user_data = &astarte_connection_sem;

    memcpy(astarte_device_config.cred_secr, cred_secr, sizeof(cred_secr));

    astarte_device_handle_t astarte_device = NULL;
    astarte_result = astarte_device_new(&astarte_device_config, &astarte_device);
    if (astarte_result != ASTARTE_RESULT_OK) {
        return -1;
    }

    edgehog_device_config_t edgehog_conf = {
        .astarte_device = astarte_device,
    };

    edgehog_device_handle_t edgehog_device = NULL;
    edgehog_result_t edgehog_result = edgehog_device_new(&edgehog_conf, &edgehog_device);

    if (edgehog_result != EDGEHOG_RESULT_OK) {
        LOG_ERR("Unable to create edgehog device handle"); // NOLINT
        return -1;
    }

    LOG_INF("Edgehog device created"); // NOLINT

    astarte_result = astarte_device_connect(astarte_device);
    if (astarte_result != ASTARTE_RESULT_OK) {
        return -1;
    }

    astarte_result = astarte_device_poll(astarte_device);
    if (astarte_result != ASTARTE_RESULT_OK) {
        LOG_ERR("First poll should not timeout as we should receive a connection ack."); // NOLINT
        return -1;
    }

    /* wait astarte connection semaphore */
    k_sem_take(&astarte_connection_sem, K_FOREVER);

    edgehog_result = edgehog_device_start(edgehog_device);
    if (edgehog_result != EDGEHOG_RESULT_OK) {
        LOG_ERR("Unable to start edgehog device"); // NOLINT
        return -1;
    }

    k_sleep(K_SECONDS(5)); // sleep for 5 seconds

    LOG_INF("End of sample operation, disconnection imminent."); // NOLINT

    edgehog_device_destroy(edgehog_device);

    astarte_result = astarte_device_destroy(astarte_device);
    if (astarte_result != ASTARTE_RESULT_OK) {
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
    struct k_sem *astarte_connection_sem = (struct k_sem *) event->user_data;
    if (astarte_connection_sem) {
        k_sem_give(astarte_connection_sem);
    }
}

static void astarte_disconnection_events_handler(astarte_device_disconnection_event_t *event)
{
    (void) event;
    LOG_INF("Astarte device disconnected"); // NOLINT
}
