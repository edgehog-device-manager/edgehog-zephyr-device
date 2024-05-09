/*
 * (C) Copyright 2024, SECO Mind Srl
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef RUNTIME_INFO_H
#define RUNTIME_INFO_H

/**
 * @file runtime_info.h
 * @brief Runtime info API.
 */

#include "edgehog_device/device.h"

#ifdef __cplusplus
extern "C" {
#endif

void publish_runtime_info(edgehog_device_handle_t edgehog_device);

#ifdef __cplusplus
}
#endif

#endif // RUNTIME_INFO_H
