/*
 * (C) Copyright 2025, SECO Mind Srl
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef WIFI_H
#define WIFI_H

#include <zephyr/net/wifi.h>

/**
 * @brief Initialize the wifi driver
 */
void app_wifi_init(void);

/**
 * @brief Connect the wifi to an SSID
 */
int app_wifi_connect(const char *ssid, enum wifi_security_type sec, const char *psk);

#endif /* WIFI_H */
