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

#ifdef CONFIG_EDGEHOG_DEVICE_ZBUS_OTA_EVENT
#include <edgehog_device/ota_event.h>
#endif

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

#define EDGEHOG_STACK_SIZE 8196
#define EDEGEHOG_STATE_RUN_BIT (1)

K_THREAD_STACK_DEFINE(edgehog_thread_stack, EDGEHOG_STACK_SIZE);
static struct k_thread edgehog_thread_data;
static struct k_sem astarte_connection_sem;

/************************************************
 * Static functions declaration
 ***********************************************/

/**
 * @brief Handler for astarte connection events.
 *
 * @param event Astarte device connection event pointer.
 */
static void astarte_connection_events_handler(astarte_device_connection_event_t event);
/**
 * @brief Handler for astarte disconnection events.
 *
 * @param event Astarte device disconnection event pointer.
 */
static void astarte_disconnection_events_handler(astarte_device_disconnection_event_t event);

/**
 * @brief Entry point for the Edgehog device thread.
 *
 * @param device_handle Handle to the Edgehog device.
 * @param unused1 Unused parameter.
 * @param unused2 Unused parameter.
 */
static void edgehog_thread_entry_point(void *device_handle, void *unused1, void *unused2);

/**
 * @brief Handler for astarte datastream object event.
 *
 * @param event Astarte device datastream object event pointer.
 */
static void datastream_object_events_handler(astarte_device_datastream_object_event_t event);

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
    tls_credential_add(CONFIG_ASTARTE_DEVICE_SDK_HTTPS_CA_CERT_TAG, TLS_CREDENTIAL_CA_CERTIFICATE,
        ca_certificate_root, sizeof(ca_certificate_root));
#endif

#if !defined(EDGEHOG_DEVICE_DEVELOP_DISABLE_OR_IGNORE_TLS)
    tls_credential_add(CONFIG_EDGEHOG_DEVICE_CA_CERT_OTA_TAG, TLS_CREDENTIAL_CA_CERTIFICATE,
        ota_ca_certificate_root, sizeof(ota_ca_certificate_root));
#endif

    char cred_secr[ASTARTE_PAIRING_CRED_SECR_LEN + 1] = CONFIG_CREDENTIAL_SECRET;

    k_sem_init(&astarte_connection_sem, 0, 1);

    edgehog_device_handle_t edgehog_device = NULL;

    astarte_device_config_t astarte_device_config;
    memset(&astarte_device_config, 0, sizeof(astarte_device_config));
    astarte_device_config.http_timeout_ms = HTTP_TIMEOUT_MS;
    astarte_device_config.mqtt_connection_timeout_ms = MQTT_FIRST_POLL_TIMEOUT_MS;
    astarte_device_config.mqtt_connected_timeout_ms = MQTT_POLL_TIMEOUT_MS;
    astarte_device_config.connection_cbk = astarte_connection_events_handler;
    astarte_device_config.disconnection_cbk = astarte_disconnection_events_handler;
    astarte_device_config.datastream_object_cbk = datastream_object_events_handler;
    astarte_device_config.cbk_user_data = &edgehog_device;

    memcpy(astarte_device_config.cred_secr, cred_secr, sizeof(cred_secr));

    astarte_device_handle_t astarte_device = NULL;
    astarte_result = astarte_device_new(&astarte_device_config, &astarte_device);
    if (astarte_result != ASTARTE_RESULT_OK) {
        return -1;
    }

    edgehog_device_config_t edgehog_conf = {
        .astarte_device = astarte_device,
    };

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

    // /* wait astarte connection semaphore */
    k_sem_take(&astarte_connection_sem, K_FOREVER);

    k_thread_create(&edgehog_thread_data, edgehog_thread_stack, EDGEHOG_STACK_SIZE,
        edgehog_thread_entry_point, edgehog_device, NULL, NULL, 0,
        K_HIGHEST_APPLICATION_THREAD_PRIO, K_NO_WAIT);

    while (astarte_result == ASTARTE_RESULT_OK || astarte_result == ASTARTE_RESULT_TIMEOUT) {
        astarte_result = astarte_device_poll(astarte_device);
    }

    LOG_ERR("Astarte device poll failure. %d -> %s", astarte_result, // NOLINT
        astarte_result_to_name(astarte_result)); // NOLINT

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

static void edgehog_thread_entry_point(void *device_handle, void *unused1, void *unused2)
{
    (void) unused1;
    (void) unused2;

    edgehog_device_handle_t edgehog_device = (edgehog_device_handle_t) device_handle;

    edgehog_result_t edgehog_result = edgehog_device_start(edgehog_device);
    if (edgehog_result != EDGEHOG_RESULT_OK) {
        LOG_ERR("Unable to start edgehog device"); // NOLINT
    }

    while (1) {
        k_sleep(K_MSEC(500)); // sleep for 500 ms
    }
}

#ifdef CONFIG_EDGEHOG_DEVICE_ZBUS_OTA_EVENT
ZBUS_CHAN_DECLARE(edgehog_ota_chan);
ZBUS_SUBSCRIBER_DEFINE(ota_evt_subscriber, 4);
static void ota_subscriber_task(void *sub_ref)
{
    const struct zbus_channel *chan;

    while (!zbus_sub_wait(&ota_evt_subscriber, &chan, K_FOREVER)) {
        edgehog_ota_chan_event_t ota = { 0 };
        if (&edgehog_ota_chan == chan) {
            // Indirect message access
            zbus_chan_read(chan, &ota, K_NO_WAIT);
            switch (ota.event) {
                case EDGEHOG_OTA_INIT_EVENT:
                    LOG_DBG("From subscriber -> EDGEHOG_OTA_INIT_EVENT");
                    break;
                case EDGEHOG_OTA_SUCCESS_EVENT:
                    LOG_DBG("From subscriber -> EDGEHOG_OTA_SUCCESS_EVENT");
                    break;
                case EDGEHOG_OTA_FAILED_EVENT:
                    LOG_DBG("From subscriber -> EDGEHOG_OTA_FAILED_EVENT");
                    break;
                default:
                    LOG_DBG("From subscriber -> EDGEHOG_OTA_INIT_EVENT");
            }
        }
    }
}
K_THREAD_DEFINE(ota_subscriber_task_id, 1024, ota_subscriber_task, NULL, NULL, NULL, 3, 0, 0);
ZBUS_CHAN_ADD_OBS(edgehog_ota_chan, ota_evt_subscriber, 3);
#endif

static void astarte_connection_events_handler(astarte_device_connection_event_t event)
{
    LOG_INF("Astarte device connected, session_present: %d", event.session_present); // NOLINT
    k_sem_give(&astarte_connection_sem);
}

static void datastream_object_events_handler(astarte_device_datastream_object_event_t event)
{
    edgehog_device_handle_t *edgehog_device
        = (edgehog_device_handle_t *) event.data_event.user_data;

    edgehog_device_datastream_object_events_handler(*edgehog_device, event);
}

static void astarte_disconnection_events_handler(astarte_device_disconnection_event_t event)
{
    (void) event;
    LOG_INF("Astarte device disconnected"); // NOLINT
}
