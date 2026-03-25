/*
 * (C) Copyright 2024, SECO Mind Srl
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runner.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <zephyr/fatal.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/shell/shell.h>
#include <zephyr/shell/shell_uart.h>
#include <zephyr/sys/bitarray.h>
#include <zephyr/sys/util.h>

#include "device_handler.h"
#include "shell_handlers.h"
#include "shell_macros.h"

#include "http.h"

LOG_MODULE_REGISTER(runner, CONFIG_RUNNER_LOG_LEVEL);

/************************************************
 *         Global functions definition          *
 ***********************************************/

edgehog_result_t my_download_callback(
    bool *abort_flag, http_download_chunk_t *download_chunk, void *user_data)
{
    LOG_WRN("%.*s", (int) download_chunk->chunk_size, download_chunk->chunk_start_addr);

    if (download_chunk->last_chunk) {
        LOG_WRN("Download complete!");
    }

    return EDGEHOG_RESULT_OK;
}

void run_end_to_end_test()
{
    LOG_INF("End to end test runner");

#if CONFIG_E2E_LOG_ONLY
    LOG_WRN("Running with device callbacks in log only mode");
    LOG_WRN("Data received will NOT be checked against expected data");
#endif

    LOG_INF("Starting the device");
    setup_device();

    // Wait for the device connection
    LOG_INF("Waiting for the device to be connected");
    wait_for_device_connection();

    // Wait for a second to allow the Edgehog device to perform the initial publish
    k_sleep(K_SECONDS(1));

    // We are ready to send and receive data
    const struct shell *uart_shell = shell_backend_uart_get_ptr();
    shell_start(uart_shell);
    k_sleep(K_MSEC(100));

    // Pytest detects the readyness of the shell through this string
    shell_print(uart_shell, SHELL_IS_READY);

    http_download_t download_ctx = { .download_cbk = my_download_callback, .user_data = NULL };
    const char *url = "https://192.0.2.2:8443/test_data.txt";
    const char *headers[] = { NULL };
    int32_t timeout_ms = 5000;
    edgehog_result_t result = edgehog_http_download(url, headers, timeout_ms, &download_ctx);
    if (result == EDGEHOG_RESULT_OK) {
        LOG_WRN("HTTP GET request succeeded.");
    } else {
        LOG_WRN("HTTP GET request failed with error code: %d", result);
    }

    // Wait untill a shell command disconnects the device
    wait_for_device_disconnection();

    // Pytest detects the a termination of the test through this string
    shell_print(uart_shell, SHELL_IS_CLOSING);
    shell_stop(uart_shell);

    // Free the device and epxected data structures after `shell_stop`
    free_device();

#if CONFIG_E2E_LOG_ONLY
    LOG_WRN("Test has been run with device callbacks in log only mode");
    LOG_WRN("Data received didn't get checked against expected data");
#endif
}
