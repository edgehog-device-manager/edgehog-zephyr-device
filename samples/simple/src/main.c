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

#include <zephyr/net/sntp.h>
#include <zephyr/posix/time.h>

#include <astarte_device_sdk/device.h>
#include <edgehog_device/device.h>

#ifdef CONFIG_EDGEHOG_DEVICE_ZBUS_OTA_EVENT
#include <edgehog_device/ota_event.h>
#endif

#if defined(CONFIG_WIFI)
#include "wifi.h"
#else
#include "eth.h"
#endif

/************************************************
 * Constants and defines
 ***********************************************/

#define POLL_PERIOD_MS 100

#define HTTP_TIMEOUT_MS (3 * MSEC_PER_SEC)
#define MQTT_FIRST_POLL_TIMEOUT_MS (3 * MSEC_PER_SEC)
#define MQTT_POLL_TIMEOUT_MS 200

#define DEVICE_OPERATIONAL_TIME_MS (15 * SEC_PER_MIN * MSEC_PER_SEC)

#define EDGEHOG_STACK_SIZE 8196

#define EDGEHOG_DEVICE_THREAD_FLAGS_TERMINATION 1U

// NOLINTBEGIN(cppcoreguidelines-avoid-non-const-global-variables)
K_THREAD_STACK_DEFINE(edgehog_thread_stack, EDGEHOG_STACK_SIZE);
static struct k_thread edgehog_thread_data;
static atomic_t edgehog_device_thread_flags;
// NOLINTEND(cppcoreguidelines-avoid-non-const-global-variables)

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

/**
 * @brief Handler for astarte datastream individual event.
 *
 * @param event Astarte device datastream individual event pointer.
 */
static void datastream_individual_events_handler(
    astarte_device_datastream_individual_event_t event);

/**
 * @brief Initialize System Time
 */
static void system_time_init();

// NOLINTNEXTLINE(hicpp-function-size)
int main(void)
{
    astarte_result_t astarte_result = ASTARTE_RESULT_OK;

    LOG_INF("Edgehog example! %s\n", CONFIG_BOARD); // NOLINT

#if defined(CONFIG_WIFI)
    LOG_INF("Initializing WiFi driver."); // NOLINT
    wifi_init();
    k_sleep(K_SECONDS(5)); // sleep for 5 seconds
#else
    LOG_INF("Initializing Ethernet driver."); // NOLINT
    if (eth_connect() != 0) {
        LOG_ERR("Connectivity intialization failed!"); // NOLINT
        return -1;
    }
#endif

    system_time_init();

#if !defined(CONFIG_ASTARTE_DEVICE_SDK_DEVELOP_DISABLE_OR_IGNORE_TLS)
    tls_credential_add(CONFIG_ASTARTE_DEVICE_SDK_HTTPS_CA_CERT_TAG, TLS_CREDENTIAL_CA_CERTIFICATE,
        ca_certificate_root, sizeof(ca_certificate_root));
#endif

#if !defined(EDGEHOG_DEVICE_DEVELOP_DISABLE_OR_IGNORE_TLS)
    tls_credential_add(CONFIG_EDGEHOG_DEVICE_CA_CERT_OTA_TAG, TLS_CREDENTIAL_CA_CERTIFICATE,
        ota_ca_certificate_root, sizeof(ota_ca_certificate_root));
#endif

    char cred_secr[ASTARTE_PAIRING_CRED_SECR_LEN + 1] = CONFIG_CREDENTIAL_SECRET;
    char device_id[ASTARTE_PAIRING_DEVICE_ID_LEN + 1] = CONFIG_DEVICE_ID;

    edgehog_device_handle_t edgehog_device = NULL;

    astarte_device_config_t astarte_device_config = { 0 };
    memset(&astarte_device_config, 0, sizeof(astarte_device_config));
    astarte_device_config.http_timeout_ms = HTTP_TIMEOUT_MS;
    astarte_device_config.mqtt_connection_timeout_ms = MQTT_FIRST_POLL_TIMEOUT_MS;
    astarte_device_config.mqtt_poll_timeout_ms = MQTT_POLL_TIMEOUT_MS;
    astarte_device_config.connection_cbk = astarte_connection_events_handler;
    astarte_device_config.disconnection_cbk = astarte_disconnection_events_handler;
    astarte_device_config.datastream_object_cbk = datastream_object_events_handler;
    astarte_device_config.datastream_individual_cbk = datastream_individual_events_handler;
    astarte_device_config.cbk_user_data = &edgehog_device;

    memcpy(astarte_device_config.cred_secr, cred_secr, sizeof(cred_secr));
    memcpy(astarte_device_config.device_id, device_id, sizeof(device_id));

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

    while (astarte_result == ASTARTE_RESULT_OK || astarte_result == ASTARTE_RESULT_TIMEOUT) {
        k_timepoint_t timepoint = sys_timepoint_calc(K_MSEC(POLL_PERIOD_MS));

        astarte_result = astarte_device_poll(astarte_device);
#ifndef CONFIG_WIFI
        eth_poll();
#endif

        k_sleep(sys_timepoint_timeout(timepoint));
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

    while (
        !atomic_test_bit(&edgehog_device_thread_flags, EDGEHOG_DEVICE_THREAD_FLAGS_TERMINATION)) {
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
    (void) event;
    LOG_INF("Astarte device connected, session_present."); // NOLINT

    if (!atomic_test_and_set_bit(
            &edgehog_device_thread_flags, EDGEHOG_DEVICE_THREAD_FLAGS_TERMINATION)) {
        edgehog_device_handle_t *edgehog_device = (edgehog_device_handle_t *) event.user_data;

        k_thread_create(&edgehog_thread_data, edgehog_thread_stack, EDGEHOG_STACK_SIZE,
            edgehog_thread_entry_point, *edgehog_device, NULL, NULL, 0,
            K_HIGHEST_APPLICATION_THREAD_PRIO, K_NO_WAIT);
    }
}

static void datastream_object_events_handler(astarte_device_datastream_object_event_t event)
{
    edgehog_device_handle_t *edgehog_device
        = (edgehog_device_handle_t *) event.data_event.user_data;

    edgehog_device_datastream_object_events_handler(*edgehog_device, event);
}

static void datastream_individual_events_handler(astarte_device_datastream_individual_event_t event)
{
    edgehog_device_handle_t *edgehog_device
        = (edgehog_device_handle_t *) event.data_event.user_data;

    edgehog_device_datastream_individual_events_handler(*edgehog_device, event);
}

static void astarte_disconnection_events_handler(astarte_device_disconnection_event_t event)
{
    (void) event;
    LOG_INF("Astarte device disconnected"); // NOLINT
}

static void system_time_init()
{
#ifdef CONFIG_NET_CONFIG_SNTP_INIT
    int ret = 0;
    struct sntp_time now;
    struct timespec tspec;

    ret = sntp_simple(
        CONFIG_NET_CONFIG_SNTP_INIT_SERVER, CONFIG_NET_CONFIG_SNTP_INIT_TIMEOUT, &now);
    if (ret == 0) {
        tspec.tv_sec = (time_t) now.seconds;
        // NOLINTBEGIN(bugprone-narrowing-conversions, hicpp-signed-bitwise,
        // readability-magic-numbers)
        tspec.tv_nsec = ((uint64_t) now.fraction * (1000lu * 1000lu * 1000lu)) >> 32;
        // NOLINTEND(bugprone-narrowing-conversions, hicpp-signed-bitwise,
        // readability-magic-numbers)
        clock_settime(CLOCK_REALTIME, &tspec);
    }
#endif
}
