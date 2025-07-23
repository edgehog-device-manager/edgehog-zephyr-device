/*
 * (C) Copyright 2024, SECO Mind Srl
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/net/net_mgmt.h>
#include <zephyr/net/sntp.h>
#include <zephyr/posix/time.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(edgehog_app, CONFIG_APP_LOG_LEVEL); // NOLINT

#if (!defined(CONFIG_ASTARTE_DEVICE_SDK_DEVELOP_USE_NON_TLS_HTTP)                                  \
    || !defined(CONFIG_ASTARTE_DEVICE_SDK_DEVELOP_USE_NON_TLS_MQTT))
#include "ca_certificates.h"
#include <zephyr/net/tls_credentials.h>
#endif

#include <astarte_device_sdk/device.h>

#ifdef CONFIG_EDGEHOG_DEVICE_ZBUS_OTA_EVENT
#include <edgehog_device/ota_event.h>
#endif
#include <edgehog_device/device.h>
#include <edgehog_device/telemetry.h>
#include <edgehog_device/wifi_scan.h>

#if defined(CONFIG_WIFI)
#include "wifi.h"
#else
#include "eth.h"
#endif

/************************************************
 * Constants and defines
 ***********************************************/

#define MAIN_THREAD_PERIOD_MS 500
#define EDGEHOG_DEVICE_PERIOD_MS 100
#ifdef CONFIG_EDGEHOG_DEVICE_ZBUS_OTA_EVENT
#define ZBUS_PERIOD_MS 500
#endif

#define HTTP_TIMEOUT_MS (3 * MSEC_PER_SEC)
#define MQTT_FIRST_POLL_TIMEOUT_MS (3 * MSEC_PER_SEC)
#define MQTT_POLL_TIMEOUT_MS 200

#define TELEMETRY_PERIOD_S 5

// NOLINTBEGIN(cppcoreguidelines-avoid-non-const-global-variables)
enum device_tread_flags
{
    DEVICE_THREADS_FLAGS_TERMINATION = 1U,
    DEVICE_THREADS_FLAGS_CONNECT_ASTARTE,
    DEVICE_THREADS_FLAGS_START_EDGEHOG,

};
static atomic_t device_threads_flags;

K_THREAD_STACK_DEFINE(edgehog_device_thread_stack_area, CONFIG_EDGEHOG_DEVICE_THREAD_STACK_SIZE);
static struct k_thread edgehog_device_thread_data;

static edgehog_device_handle_t edgehog_device;

#ifdef CONFIG_EDGEHOG_DEVICE_ZBUS_OTA_EVENT
K_THREAD_STACK_DEFINE(zbus_thread_stack_area, CONFIG_ZBUS_THREAD_STACK_SIZE);
static struct k_thread zbus_thread_data;
// Define a zbus subscriber and add it as an observer to the Edgehog OTA channel
#define EDGEHOG_OTA_SUBSCRIBER_NOTIFICATION_QUEUE_SIZE 5
#define EDGEHOG_OTA_OBSERVER_NOTIFY_PRIORITY 5
ZBUS_SUBSCRIBER_DEFINE(edgehog_ota_subscriber, EDGEHOG_OTA_SUBSCRIBER_NOTIFICATION_QUEUE_SIZE);
ZBUS_CHAN_ADD_OBS(edgehog_ota_chan, edgehog_ota_subscriber, EDGEHOG_OTA_OBSERVER_NOTIFY_PRIORITY);
#endif
// NOLINTEND(cppcoreguidelines-avoid-non-const-global-variables)

/************************************************
 * Static functions declaration
 ***********************************************/

/**
 * @brief Initialize System Time
 */
static void system_time_init(void);
/**
 * @brief Entry point for the Astarte device thread.
 *
 * @param arg1 Unused argument.
 * @param arg2 Unused argument.
 * @param arg3 Unused argument.
 */
static void edgehog_device_thread_entry_point(void *arg1, void *arg2, void *arg3);
#ifdef CONFIG_EDGEHOG_DEVICE_ZBUS_OTA_EVENT
/**
 * @brief Entry point for the Zbus reception thread.
 *
 * @param arg1 Unused argument.
 * @param arg2 Unused argument.
 * @param arg3 Unused argument.
 */
static void zbus_thread_entry_point(void *arg1, void *arg2, void *arg3);
#endif
#ifdef CONFIG_EDGEHOG_DEVICE_ZBUS_OTA_EVENT
/**
 * @brief Listen on the Edgehog zbus channel for events.
 *
 * @param timeout The timeout used for listening. In case of a received notification this timeout
 * is also used for reading the message.
 */
static void edgehog_listen_zbus_channel(k_timeout_t timeout);
#endif

/************************************************
 * Global functions definition
 ***********************************************/

// NOLINTNEXTLINE(hicpp-function-size)
int main(void)
{
    LOG_INF("Edgehog device sample"); // NOLINT
    LOG_INF("Board: %s", CONFIG_BOARD); // NOLINT

#if defined(CONFIG_WIFI)
    LOG_INF("Initializing WiFi driver."); // NOLINT
    app_wifi_init();
    k_sleep(K_SECONDS(5));
    const char *ssid = CONFIG_WIFI_SSID;
    enum wifi_security_type sec = WIFI_SECURITY_TYPE_PSK;
    const char *psk = CONFIG_WIFI_PASSWORD;
    if (app_wifi_connect(ssid, sec, psk) != 0) {
        LOG_ERR("Connectivity intialization failed!"); // NOLINT
        return -1;
    }
#else
    LOG_INF("Initializing Ethernet driver."); // NOLINT
    if (eth_connect() != 0) {
        LOG_ERR("Connectivity intialization failed!"); // NOLINT
        return -1;
    }
#endif

    // Add TLS certificate for Astarte if required
#if (!defined(CONFIG_ASTARTE_DEVICE_SDK_DEVELOP_USE_NON_TLS_HTTP)                                  \
    || !defined(CONFIG_ASTARTE_DEVICE_SDK_DEVELOP_USE_NON_TLS_MQTT))
    tls_credential_add(CONFIG_ASTARTE_DEVICE_SDK_HTTPS_CA_CERT_TAG, TLS_CREDENTIAL_CA_CERTIFICATE,
        ca_certificate_root, sizeof(ca_certificate_root));
#endif
    // Add TLS certificate for Edgehog if required
#if !defined(CONFIG_EDGEHOG_DEVICE_DEVELOP_DISABLE_OR_IGNORE_TLS)
    tls_credential_add(CONFIG_EDGEHOG_DEVICE_CA_CERT_OTA_TAG, TLS_CREDENTIAL_CA_CERTIFICATE,
        ota_ca_certificate_root, sizeof(ota_ca_certificate_root));
#endif

    // Initalize the system time
    system_time_init();

    // Spawn a new thread for the Edgehog device
    k_thread_create(&edgehog_device_thread_data, edgehog_device_thread_stack_area,
        K_THREAD_STACK_SIZEOF(edgehog_device_thread_stack_area), edgehog_device_thread_entry_point,
        NULL, NULL, NULL, CONFIG_EDGEHOG_DEVICE_THREAD_PRIORITY, 0, K_NO_WAIT);

    // Wait for a predefined operational time.
    k_timepoint_t finish_timepoint = sys_timepoint_calc(K_SECONDS(CONFIG_SAMPLE_DURATION_SECONDS));
    while (!K_TIMEOUT_EQ(sys_timepoint_timeout(finish_timepoint), K_NO_WAIT)) {
        k_timepoint_t timepoint = sys_timepoint_calc(K_MSEC(MAIN_THREAD_PERIOD_MS));
#if !defined(CONFIG_WIFI)
        eth_poll();
#endif
        k_sleep(sys_timepoint_timeout(timepoint));
    }

    // Signal to the Device threads that they should terminate.
    atomic_set_bit(&device_threads_flags, DEVICE_THREADS_FLAGS_TERMINATION);

    // Wait for the Edgehog thread to terminate.
    if (k_thread_join(&edgehog_device_thread_data, K_FOREVER) != 0) {
        LOG_ERR("Failed in waiting for the Edgehog thread to terminate."); // NOLINT
    }

    LOG_INF("Edgehog device sample finished."); // NOLINT
    k_sleep(K_MSEC(MSEC_PER_SEC));

    return 0;
}

/************************************************
 * Static functions definitions
 ***********************************************/

static void system_time_init()
{
#ifdef CONFIG_SNTP
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

static void edgehog_device_thread_entry_point(void *arg1, void *arg2, void *arg3)
{
    ARG_UNUSED(arg1);
    ARG_UNUSED(arg2);
    ARG_UNUSED(arg3);

    // Configuring the Astarte device used by the Edgehog device to communicate with the cloud
    // Edgehog instance. This device can also be leveraged by the user to send and receive data
    // through Astarte, check out the sample README for more information.
    char cred_secr[ASTARTE_PAIRING_CRED_SECR_LEN + 1] = CONFIG_ASTARTE_CREDENTIAL_SECRET;
    char device_id[ASTARTE_DEVICE_ID_LEN + 1] = CONFIG_ASTARTE_DEVICE_ID;

    astarte_device_config_t astarte_device_config = { 0 };
    astarte_device_config.http_timeout_ms = HTTP_TIMEOUT_MS;
    astarte_device_config.mqtt_connection_timeout_ms = MQTT_FIRST_POLL_TIMEOUT_MS;
    astarte_device_config.mqtt_poll_timeout_ms = MQTT_POLL_TIMEOUT_MS;
    memcpy(astarte_device_config.cred_secr, cred_secr, sizeof(cred_secr));
    memcpy(astarte_device_config.device_id, device_id, sizeof(device_id));

    edgehog_result_t eres = EDGEHOG_RESULT_OK;

    edgehog_telemetry_config_t telemetry_config = {
        .type = EDGEHOG_TELEMETRY_SYSTEM_STATUS,
        .period_seconds = TELEMETRY_PERIOD_S,
    };
    edgehog_device_config_t edgehog_conf = {
        .astarte_device_config = astarte_device_config,
        .telemetry_config = &telemetry_config,
        .telemetry_config_len = 1,
    };
    eres = edgehog_device_new(&edgehog_conf, &edgehog_device);
    if (eres != EDGEHOG_RESULT_OK) {
        LOG_ERR("Unable to create edgehog device handle"); // NOLINT
        goto exit;
    }

    eres = edgehog_device_start(edgehog_device);
    if (eres != EDGEHOG_RESULT_OK) {
        LOG_ERR("Unable to start edgehog device"); // NOLINT
        goto exit;
    }

#ifdef CONFIG_EDGEHOG_DEVICE_ZBUS_OTA_EVENT
    // Spawn a new thread for listening on Zbus device
    k_thread_create(&zbus_thread_data, zbus_thread_stack_area,
        K_THREAD_STACK_SIZEOF(zbus_thread_stack_area), zbus_thread_entry_point, NULL, NULL, NULL,
        CONFIG_ZBUS_THREAD_PRIORITY, 0, K_NO_WAIT);
#endif

    while (!atomic_test_bit(&device_threads_flags, DEVICE_THREADS_FLAGS_TERMINATION)) {
        k_timepoint_t timepoint = sys_timepoint_calc(K_MSEC(EDGEHOG_DEVICE_PERIOD_MS));

        eres = edgehog_device_poll(edgehog_device);
        if (eres != EDGEHOG_RESULT_OK) {
            LOG_ERR("Edgehog device poll failure."); // NOLINT
            goto exit;
        }

        k_sleep(sys_timepoint_timeout(timepoint));
    }

    LOG_INF("End of sample, stopping Edgehog."); // NOLINT
    eres = edgehog_device_stop(edgehog_device, K_FOREVER);
    if (eres != EDGEHOG_RESULT_OK) {
        LOG_ERR("Unable to stop the edgehog device"); // NOLINT
        goto exit;
    }

    LOG_INF("Edgehog device will now be destroyed."); // NOLINT
    edgehog_device_destroy(edgehog_device);

exit:
    // Ensure to set the thread termination flag
    atomic_set_bit(&device_threads_flags, DEVICE_THREADS_FLAGS_TERMINATION);

#ifdef CONFIG_EDGEHOG_DEVICE_ZBUS_OTA_EVENT
    // Wait for the Zbus thread to terminate.
    if (k_thread_join(&zbus_thread_data, K_SECONDS(10)) != 0) {
        LOG_ERR("Failed in waiting for the Zbus thread to terminate."); // NOLINT
    }
#endif
}

#ifdef CONFIG_EDGEHOG_DEVICE_ZBUS_OTA_EVENT
static void zbus_thread_entry_point(void *arg1, void *arg2, void *arg3)
{
    ARG_UNUSED(arg1);
    ARG_UNUSED(arg2);
    ARG_UNUSED(arg3);

    while (!atomic_test_bit(&device_threads_flags, DEVICE_THREADS_FLAGS_TERMINATION)) {
        edgehog_listen_zbus_channel(K_MSEC(ZBUS_PERIOD_MS));
    }
}
#endif

#ifdef CONFIG_EDGEHOG_DEVICE_ZBUS_OTA_EVENT
static void edgehog_listen_zbus_channel(k_timeout_t timeout)
{
    const struct zbus_channel *chan = NULL;
    edgehog_ota_chan_event_t ota = { 0 };
    if ((zbus_sub_wait(&edgehog_ota_subscriber, &chan, timeout) == 0) && (&edgehog_ota_chan == chan)
        && (zbus_chan_read(chan, &ota, timeout) == 0)) {
        switch (ota.event) {
            case EDGEHOG_OTA_INIT_EVENT:
                LOG_WRN("To subscriber -> EDGEHOG_OTA_INIT_EVENT"); // NOLINT
                break;
            case EDGEHOG_OTA_PENDING_REBOOT_EVENT:
                LOG_WRN("To subscriber -> EDGEHOG_OTA_PENDING_REBOOT_EVENT"); // NOLINT
                edgehog_ota_chan_event_t ota_chan_event
                    = { .event = EDGEHOG_OTA_CONFIRM_REBOOT_EVENT };
                zbus_chan_pub(&edgehog_ota_chan, &ota_chan_event, K_SECONDS(1));
                break;
            case EDGEHOG_OTA_CONFIRM_REBOOT_EVENT:
                LOG_WRN("To subscriber -> EDGEHOG_OTA_CONFIRM_REBOOT_EVENT"); // NOLINT
                break;
            case EDGEHOG_OTA_FAILED_EVENT:
                LOG_WRN("To subscriber -> EDGEHOG_OTA_FAILED_EVENT"); // NOLINT
                break;
            case EDGEHOG_OTA_SUCCESS_EVENT:
                LOG_WRN("To subscriber -> EDGEHOG_OTA_SUCCESS_EVENT"); // NOLINT
                break;
            default:
                LOG_WRN("To subscriber -> EDGEHOG_OTA_INVALID_EVENT"); // NOLINT
        }
    }
}
#endif
