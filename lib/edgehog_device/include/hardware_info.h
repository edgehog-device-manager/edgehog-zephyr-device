/*
 * (C) Copyright 2024, SECO Mind Srl
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef HARDWARE_INFO_H
#define HARDWARE_INFO_H

/**
 * @file hardware_info.h
 * @brief Hardware info API
 */

#include "edgehog_device/device.h"

extern const astarte_interface_t hardware_info_interface;

#ifdef __cplusplus
extern "C" {
#endif

void publish_hardware_info(edgehog_device_handle_t edgehog_device);

#ifdef __cplusplus
}
#endif

#endif // HARDWARE_INFO_H
