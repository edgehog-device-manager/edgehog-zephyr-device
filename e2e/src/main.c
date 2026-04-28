/*
 * (C) Copyright 2024, SECO Mind Srl
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/sntp.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys_clock.h>

#if defined(CONFIG_ARCH_POSIX)
#include <nsi_main.h>
#endif

#if !defined(CONFIG_ASTARTE_DEVICE_SDK_DEVELOP_USE_NON_TLS_HTTP)                                   \
    || !defined(CONFIG_ASTARTE_DEVICE_SDK_DEVELOP_USE_NON_TLS_MQTT)                                \
    || !!defined(CONFIG_EDGEHOG_DEVICE_DEVELOP_USE_NON_TLS_HTTP)
#define NEEDS_TLS
#endif

#if defined(NEEDS_TLS)

// Enable mbed tls debug
#define MBEDTLS_DEBUG_C

#if defined(CONFIG_E2E_ASTARTE_TLS_CERTIFICATE_PATH)
#include "e2e_ca_certificates.h"
#else
#error TLS enabled but no generated certificate found: check the CONFIG_E2E_ASTARTE_TLS_CERTIFICATE_PATH option
#endif

#include <mbedtls/debug.h>
#include <zephyr/net/tls_credentials.h>

#endif

#include "eth.h"

#include "runner.h"
#include "utilities.h"

LOG_MODULE_REGISTER(main, CONFIG_APP_LOG_LEVEL);

/************************************************
 *       Checks over configuration values       *
 ***********************************************/

/************************************************
 *   Constants, static variables and defines    *
 ***********************************************/

#define ETH_THREAD_STACK_SIZE 4096
K_THREAD_STACK_DEFINE(eth_thread_stack_area, ETH_THREAD_STACK_SIZE);
static struct k_thread eth_thread_data;
enum eth_thread_flags
{
    ETH_THREAD_TERMINATION_FLAG = 0,
};
static atomic_t eth_thread_flags;

/************************************************
 *         Static functions declaration         *
 ***********************************************/

static void system_time_init(void);
static void eth_thread_entry_point(void *unused1, void *unused2, void *unused3);

/************************************************
 *         Global functions definition          *
 ***********************************************/

int main(void)
{
    LOG_INF("Edgehog device end to end test");

    // Initialize Ethernet driver
    LOG_INF("Initializing Ethernet driver.");
    if (eth_connect() != 0) {
        LOG_ERR("Connectivity intialization failed!");
        return -1;
    }

#if defined(NEEDS_TLS)

    // Add TLS certificate
    tls_credential_add(CONFIG_ASTARTE_DEVICE_SDK_HTTPS_CA_CERT_TAG, TLS_CREDENTIAL_CA_CERTIFICATE,
        astarte_ca_certificate_root, ARRAY_SIZE(astarte_ca_certificate_root));

    tls_credential_add(CONFIG_EDGEHOG_DEVICE_OTA_HTTPS_CA_CERT_TAG, TLS_CREDENTIAL_CA_CERTIFICATE,
        edgehog_ota_ca_certificate_root, sizeof(edgehog_ota_ca_certificate_root));

    tls_credential_add(CONFIG_EDGEHOG_DEVICE_FILE_TRANSFER_HTTPS_CA_CERT_TAG,
        TLS_CREDENTIAL_CA_CERTIFICATE, edgehog_ft_ca_certificate_root,
        sizeof(edgehog_ft_ca_certificate_root));

    // Enable mbedtls logging
    mbedtls_debug_set_threshold(1);

#endif

    LOG_INF("Spawning a new thread to poll the eth interface and check connectivity.");
    k_thread_create(&eth_thread_data, eth_thread_stack_area,
        K_THREAD_STACK_SIZEOF(eth_thread_stack_area), eth_thread_entry_point, NULL, NULL, NULL,
        CONFIG_E2E_DEVICE_THREAD_PRIORITY, 0, K_NO_WAIT);

    system_time_init();

    LOG_INF("Running the end to end test.");
    run_end_to_end_test();

    atomic_set_bit(&eth_thread_flags, ETH_THREAD_TERMINATION_FLAG);
    CHECK_HALT(k_thread_join(&eth_thread_data, K_FOREVER) != 0,
        "Failed in waiting for the eth polling thread to terminate.");

    LOG_INF("Returning from the end to end test.");

#if defined(CONFIG_ARCH_POSIX)
    // Required to terminate when on posix
    nsi_exit(0);
#endif
    return 0;
}

/************************************************
 *         Static functions definitions         *
 ***********************************************/

static void system_time_init()
{
#ifdef CONFIG_SNTP
    int ret = 0;
    struct sntp_time now;
    struct timespec tspec;

    LOG_INF("Attempting to sync time via SNTP from %s...", CONFIG_NET_CONFIG_SNTP_INIT_SERVER);

    ret = sntp_simple(
        CONFIG_NET_CONFIG_SNTP_INIT_SERVER, CONFIG_NET_CONFIG_SNTP_INIT_TIMEOUT, &now);
    if (ret == 0) {
        tspec.tv_sec = (time_t) now.seconds;
        // NOLINTBEGIN(bugprone-narrowing-conversions, hicpp-signed-bitwise,
        // readability-magic-numbers)
        tspec.tv_nsec = ((uint64_t) now.fraction * (1000lu * 1000lu * 1000lu)) >> 32;
        // NOLINTEND(bugprone-narrowing-conversions, hicpp-signed-bitwise,
        // readability-magic-numbers)
        sys_clock_settime(SYS_CLOCK_REALTIME, &tspec);

        LOG_INF("SNTP sync successful! System time set to: %lld seconds", (long long) tspec.tv_sec);
    } else {
        LOG_ERR("Failed to get time from SNTP server, error: %d", ret);
    }
#endif
}

static void eth_thread_entry_point(void *unused1, void *unused2, void *unused3)
{
    ARG_UNUSED(unused1);
    ARG_UNUSED(unused2);
    ARG_UNUSED(unused3);

    LOG_INF("Started the eth polling thread");

    while (!atomic_test_bit(&eth_thread_flags, ETH_THREAD_TERMINATION_FLAG)) {
        k_timepoint_t timepoint = sys_timepoint_calc(K_MSEC(CONFIG_E2E_ETH_POLL_PERIOD_MS));

        if (eth_poll() != 0) {
            LOG_ERR("Failed polling Ethernet."); // NOLINT
        }

        k_sleep(sys_timepoint_timeout(timepoint));
    }
}
