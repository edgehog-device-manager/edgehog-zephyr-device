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

#include "eth.h"

/************************************************
 * Constants and defines
 ***********************************************/

#define MAIN_THREAD_PERIOD_MS 500
#define ASTARTE_DEVICE_PERIOD_MS 100
#define EDGEHOG_DEVICE_PERIOD_MS 500

#define HTTP_TIMEOUT_MS (3 * MSEC_PER_SEC)
#define MQTT_FIRST_POLL_TIMEOUT_MS (3 * MSEC_PER_SEC)
#define MQTT_POLL_TIMEOUT_MS 200

#define TELEMETRY_PERIOD_S 5

// NOLINTBEGIN(cppcoreguidelines-avoid-non-const-global-variables)
#define DEVICE_THREADS_FLAGS_TERMINATION 1U
#define DEVICE_THREADS_FLAGS_CREATE_EDGEHOG 2U
#define DEVICE_THREADS_FLAGS_CONNECT_ASTARTE 3U
#define DEVICE_THREADS_FLAGS_START_EDGEHOG 4U
static atomic_t device_threads_flags;

K_THREAD_STACK_DEFINE(astarte_device_thread_stack_area, CONFIG_ASTARTE_DEVICE_THREAD_STACK_SIZE);
static struct k_thread astarte_device_thread_data;
K_THREAD_STACK_DEFINE(edgehog_device_thread_stack_area, CONFIG_EDGEHOG_DEVICE_THREAD_STACK_SIZE);
static struct k_thread edgehog_device_thread_data;

// The Astarte and Edgehog handles have been declared statically file wide to make it possible
// to separate them in two threads.
static astarte_device_handle_t astarte_device;
static edgehog_device_handle_t edgehog_device;

#ifdef CONFIG_EDGEHOG_DEVICE_ZBUS_OTA_EVENT
// Define a zbus subscriber and add it as an observer to the Edgehog OTA channel
ZBUS_SUBSCRIBER_DEFINE(edgehog_ota_subscriber, 4);
ZBUS_CHAN_ADD_OBS(edgehog_ota_chan, edgehog_ota_subscriber, 3);

#define EDGEHOG_OTA_SUBSCRIBER_THREAD_STACK_SIZE 1024
#define EDGEHOG_OTA_SUBSCRIBER_THREAD_PRIORIYT 3
K_THREAD_STACK_DEFINE(
    edgehog_ota_subscriber_thread_stack_area, EDGEHOG_OTA_SUBSCRIBER_THREAD_STACK_SIZE);
static struct k_thread edgehog_ota_subscriber_thread_data;
#endif
// NOLINTEND(cppcoreguidelines-avoid-non-const-global-variables)

/************************************************
 * Static functions declaration
 ***********************************************/

/**
 * @brief Initialize System Time
 */
static void system_time_init(void);
#ifdef CONFIG_EDGEHOG_DEVICE_ZBUS_OTA_EVENT
/**
 * @brief Entry point for the Edgehog OTA zbus subscriber thread.
 *
 * @param arg1 Unused argument.
 * @param arg2 Unused argument.
 * @param arg3 Unused argument.
 */
static void edgehog_ota_subscriber_thread_entry_point(void *arg1, void *arg2, void *arg3);
#endif
/**
 * @brief Entry point for the Astarte device thread.
 *
 * @param arg1 Unused argument.
 * @param arg2 Unused argument.
 * @param arg3 Unused argument.
 */
static void edgehog_device_thread_entry_point(void *arg1, void *arg2, void *arg3);
/**
 * @brief Entry point for the Astarte device thread.
 *
 * @param arg1 Unused argument.
 * @param arg2 Unused argument.
 * @param arg3 Unused argument.
 */
static void astarte_device_thread_entry_point(void *arg1, void *arg2, void *arg3);
/**
 * @brief Callback handler for Astarte connection events.
 *
 * @param event Astarte device connection event.
 */
static void astarte_device_connection_callback(astarte_device_connection_event_t event);
/**
 * @brief Callback handler for Astarte disconnection events.
 *
 * @param event Astarte device disconnection event.
 */
static void astarte_device_disconnection_callback(astarte_device_disconnection_event_t event);
/**
 * @brief Callback handler for Astarte datastream individual event.
 *
 * @param event Astarte device datastream individual event.
 */
static void astarte_device_datastream_individual_callback(
    astarte_device_datastream_individual_event_t event);
/**
 * @brief Callback handler for Astarte datastream object event.
 *
 * @param event Astarte device datastream object event pointer.
 */
static void astarte_device_datastream_object_callback(
    astarte_device_datastream_object_event_t event);
/**
 * @brief Callback handler for Astarte property set event.
 *
 * @param event Astarte device property set event pointer.
 */
static void astarte_device_property_set_callback(astarte_device_property_set_event_t event);
/**
 * @brief Callback handler for Astarte property unset event.
 *
 * @param event Astarte device property unset event pointer.
 */
static void astarte_device_property_unset_callback(astarte_device_data_event_t event);

/************************************************
 * Global functions definition
 ***********************************************/

// NOLINTNEXTLINE(hicpp-function-size)
int main(void)
{
    LOG_INF("Edgehog device sample"); // NOLINT
    LOG_INF("Board: %s", CONFIG_BOARD); // NOLINT

    LOG_INF("Initializing Ethernet driver."); // NOLINT
    if (eth_connect() != 0) {
        LOG_ERR("Connectivity intialization failed!"); // NOLINT
        return -1;
    }

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

#ifdef CONFIG_EDGEHOG_DEVICE_ZBUS_OTA_EVENT
    // Start an observer thread for the Edgehog OTA update events
    k_thread_create(&edgehog_ota_subscriber_thread_data, edgehog_ota_subscriber_thread_stack_area,
        K_THREAD_STACK_SIZEOF(edgehog_ota_subscriber_thread_stack_area),
        edgehog_ota_subscriber_thread_entry_point, NULL, NULL, NULL,
        EDGEHOG_OTA_SUBSCRIBER_THREAD_PRIORIYT, 0, K_NO_WAIT);
#endif

    // Spawn a new thread for the Astarte device and one for the Edgehog device
    k_thread_create(&astarte_device_thread_data, astarte_device_thread_stack_area,
        K_THREAD_STACK_SIZEOF(astarte_device_thread_stack_area), astarte_device_thread_entry_point,
        NULL, NULL, NULL, CONFIG_ASTARTE_DEVICE_THREAD_PRIORITY, 0, K_NO_WAIT);
    k_thread_create(&edgehog_device_thread_data, edgehog_device_thread_stack_area,
        K_THREAD_STACK_SIZEOF(edgehog_device_thread_stack_area), edgehog_device_thread_entry_point,
        NULL, NULL, NULL, CONFIG_EDGEHOG_DEVICE_THREAD_PRIORITY, 0, K_NO_WAIT);

    // Wait for a predefined operational time.
    k_timepoint_t finish_timepoint = sys_timepoint_calc(K_SECONDS(CONFIG_SAMPLE_DURATION_SECONDS));
    while (!K_TIMEOUT_EQ(sys_timepoint_timeout(finish_timepoint), K_NO_WAIT)) {
        k_timepoint_t timepoint = sys_timepoint_calc(K_MSEC(MAIN_THREAD_PERIOD_MS));
        // Ensure the connectivity is still present
        eth_poll();
        k_sleep(sys_timepoint_timeout(timepoint));
    }

    // Signal to the Astarte thread that is should terminate.
    atomic_set_bit(&device_threads_flags, DEVICE_THREADS_FLAGS_TERMINATION);

    // Wait for the Astarte thread to terminate.
    if (k_thread_join(&astarte_device_thread_data, K_FOREVER) != 0) {
        LOG_ERR("Failed in waiting for the Astarte thread to terminate."); // NOLINT
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

#ifdef CONFIG_EDGEHOG_DEVICE_ZBUS_OTA_EVENT
static void edgehog_ota_subscriber_thread_entry_point(void *arg1, void *arg2, void *arg3)
{
    ARG_UNUSED(arg1);
    ARG_UNUSED(arg2);
    ARG_UNUSED(arg3);

    const struct zbus_channel *chan = NULL;

    while (zbus_sub_wait(&edgehog_ota_subscriber, &chan, K_FOREVER) == 0) {
        edgehog_ota_chan_event_t ota = { 0 };
        if (&edgehog_ota_chan == chan) {
            // Indirect message access
            zbus_chan_read(chan, &ota, K_NO_WAIT);
            switch (ota.event) {
                case EDGEHOG_OTA_INIT_EVENT:
                    LOG_WRN("From subscriber -> EDGEHOG_OTA_INIT_EVENT"); // NOLINT
                    break;
                case EDGEHOG_OTA_REBOOTING_EVENT:
                    LOG_WRN("From subscriber -> EDGEHOG_OTA_REBOOTING_EVENT"); // NOLINT
                    edgehog_ota_chan_event_t ota_chan_event
                        = { .event = EDGEHOG_OTA_REBOOTING_EVENT };
                    zbus_chan_pub(&edgehog_ota_chan, &ota_chan_event, K_SECONDS(1));
                    break;
                case EDGEHOG_OTA_SUCCESS_EVENT:
                    LOG_WRN("From subscriber -> EDGEHOG_OTA_SUCCESS_EVENT"); // NOLINT
                    break;
                case EDGEHOG_OTA_FAILED_EVENT:
                    LOG_WRN("From subscriber -> EDGEHOG_OTA_FAILED_EVENT"); // NOLINT
                    break;
                default:
                    LOG_WRN("From subscriber -> EDGEHOG_OTA_INIT_EVENT"); // NOLINT
            }
        }
    }
}
#endif

static void edgehog_device_thread_entry_point(void *arg1, void *arg2, void *arg3)
{
    ARG_UNUSED(arg1);
    ARG_UNUSED(arg2);
    ARG_UNUSED(arg3);

    edgehog_result_t eres = EDGEHOG_RESULT_OK;

    while (!atomic_test_bit(&device_threads_flags, DEVICE_THREADS_FLAGS_CREATE_EDGEHOG)) {
        k_sleep(K_MSEC(100));
    }

    edgehog_telemetry_config_t telemetry_config = {
        .type = EDGEHOG_TELEMETRY_SYSTEM_STATUS,
        .period_seconds = TELEMETRY_PERIOD_S,
    };
    edgehog_device_config_t edgehog_conf = {
        .astarte_device = astarte_device,
        .telemetry_config = &telemetry_config,
        .telemetry_config_len = 1,
    };
    eres = edgehog_device_new(&edgehog_conf, &edgehog_device);
    if (eres != EDGEHOG_RESULT_OK) {
        LOG_ERR("Unable to create edgehog device handle"); // NOLINT
        return;
    }

    // Signal the Astarte thread to connect the device
    atomic_set_bit(&device_threads_flags, DEVICE_THREADS_FLAGS_CONNECT_ASTARTE);
    while (!atomic_test_bit(&device_threads_flags, DEVICE_THREADS_FLAGS_START_EDGEHOG)) {
        k_sleep(K_MSEC(100));
    }

    eres = edgehog_device_start(edgehog_device);
    if (eres != EDGEHOG_RESULT_OK) {
        LOG_ERR("Unable to start edgehog device"); // NOLINT
        return;
    }

    while (!atomic_test_bit(&device_threads_flags, DEVICE_THREADS_FLAGS_TERMINATION)) {
        k_sleep(K_MSEC(EDGEHOG_DEVICE_PERIOD_MS));
    }

    LOG_INF("End of sample, Edgehog stopping imminent."); // NOLINT
    eres = edgehog_device_stop(edgehog_device, K_FOREVER);
    if (eres != EDGEHOG_RESULT_OK) {
        LOG_ERR("Unable to stop the edgehog device"); // NOLINT
        return;
    }

    LOG_INF("Edgehog device will now be destroyed."); // NOLINT
    edgehog_device_destroy(edgehog_device);

    LOG_INF("Edgehog thread will now be terminated."); // NOLINT
    k_sleep(K_MSEC(MSEC_PER_SEC));
}

static void astarte_device_thread_entry_point(void *arg1, void *arg2, void *arg3)
{
    ARG_UNUSED(arg1);
    ARG_UNUSED(arg2);
    ARG_UNUSED(arg3);

    astarte_result_t ares = ASTARTE_RESULT_OK;

    // Initialize the Astarte device
    char cred_secr[ASTARTE_PAIRING_CRED_SECR_LEN + 1] = CONFIG_ASTARTE_CREDENTIAL_SECRET;
    char device_id[ASTARTE_DEVICE_ID_LEN + 1] = CONFIG_ASTARTE_DEVICE_ID;

    astarte_device_config_t astarte_device_config = { 0 };
    memset(&astarte_device_config, 0, sizeof(astarte_device_config));
    astarte_device_config.http_timeout_ms = HTTP_TIMEOUT_MS;
    astarte_device_config.mqtt_connection_timeout_ms = MQTT_FIRST_POLL_TIMEOUT_MS;
    astarte_device_config.mqtt_poll_timeout_ms = MQTT_POLL_TIMEOUT_MS;
    astarte_device_config.connection_cbk = astarte_device_connection_callback;
    astarte_device_config.disconnection_cbk = astarte_device_disconnection_callback;
    astarte_device_config.datastream_individual_cbk = astarte_device_datastream_individual_callback;
    astarte_device_config.datastream_object_cbk = astarte_device_datastream_object_callback;
    astarte_device_config.property_set_cbk = astarte_device_property_set_callback;
    astarte_device_config.property_unset_cbk = astarte_device_property_unset_callback;
    astarte_device_config.cbk_user_data = (void *) &edgehog_device;
    memcpy(astarte_device_config.cred_secr, cred_secr, sizeof(cred_secr));
    memcpy(astarte_device_config.device_id, device_id, sizeof(device_id));

    ares = astarte_device_new(&astarte_device_config, &astarte_device);
    if (ares != ASTARTE_RESULT_OK) {
        LOG_ERR("Astarte device creation failure."); // NOLINT
        return;
    }

    // Signal the Edgehog thread to create the device
    atomic_set_bit(&device_threads_flags, DEVICE_THREADS_FLAGS_CREATE_EDGEHOG);
    while (!atomic_test_bit(&device_threads_flags, DEVICE_THREADS_FLAGS_CONNECT_ASTARTE)) {
        k_sleep(K_MSEC(100));
    }

    // Connect and run the Astarte device
    ares = astarte_device_connect(astarte_device);
    if (ares != ASTARTE_RESULT_OK) {
        LOG_ERR("Astarte device connection failure."); // NOLINT
        return;
    }

    while (!atomic_test_bit(&device_threads_flags, DEVICE_THREADS_FLAGS_TERMINATION)) {
        k_timepoint_t timepoint = sys_timepoint_calc(K_MSEC(ASTARTE_DEVICE_PERIOD_MS));

        ares = astarte_device_poll(astarte_device);
        if (ares != ASTARTE_RESULT_OK) {
            LOG_ERR("Astarte device poll failure."); // NOLINT
            return;
        }

        k_sleep(sys_timepoint_timeout(timepoint));
    }

    LOG_INF("End of sample, Astarte disconnection imminent."); // NOLINT
    ares = astarte_device_disconnect(astarte_device);
    if (ares != ASTARTE_RESULT_OK) {
        LOG_ERR("Astarte device disconnection failure."); // NOLINT
        return;
    }

    LOG_INF("Astarte device will now be destroyed."); // NOLINT
    ares = astarte_device_destroy(astarte_device);
    if (ares != ASTARTE_RESULT_OK) {
        LOG_ERR("Astarte device destroy failure."); // NOLINT
        return;
    }

    LOG_INF("Astarte thread will now be terminated."); // NOLINT
    k_sleep(K_MSEC(MSEC_PER_SEC));
}

static void astarte_device_connection_callback(astarte_device_connection_event_t event)
{
    ARG_UNUSED(event);
    LOG_INF("Astarte device connected"); // NOLINT
    // Signal the Edgehog thread to start the Edgehog device
    atomic_set_bit(&device_threads_flags, DEVICE_THREADS_FLAGS_START_EDGEHOG);
}

static void astarte_device_disconnection_callback(astarte_device_disconnection_event_t event)
{
    ARG_UNUSED(event);
    LOG_INF("Astarte device disconnected"); // NOLINT
}

static void astarte_device_datastream_individual_callback(
    astarte_device_datastream_individual_event_t event)
{
    edgehog_device_handle_t *edgehog_device
        = (edgehog_device_handle_t *) event.data_event.user_data;

    edgehog_device_datastream_individual_events_handler(*edgehog_device, event);
}

static void astarte_device_datastream_object_callback(
    astarte_device_datastream_object_event_t event)
{
    edgehog_device_handle_t *edgehog_device
        = (edgehog_device_handle_t *) event.data_event.user_data;
    edgehog_device_datastream_object_events_handler(*edgehog_device, event);
}

static void astarte_device_property_set_callback(astarte_device_property_set_event_t event)
{

    edgehog_device_handle_t *edgehog_device
        = (edgehog_device_handle_t *) event.data_event.user_data;

    edgehog_device_property_set_events_handler(*edgehog_device, event);
}

static void astarte_device_property_unset_callback(astarte_device_data_event_t event)
{
    edgehog_device_handle_t *edgehog_device = (edgehog_device_handle_t *) event.user_data;

    edgehog_device_property_unset_events_handler(*edgehog_device, event);
}
