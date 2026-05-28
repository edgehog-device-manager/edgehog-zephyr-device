/*
 * (C) Copyright 2026, SECO Mind Srl
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef NETWORK_PROPERTIES_H
#define NETWORK_PROPERTIES_H

/**
 * @file network_properties.h
 * @brief Storage usageS API
 */

#include "edgehog_device/device.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Publish a network interfaces information.
 *
 * @param[in] edgehog_device A valid Edgehog device handle.
 * @return edgehog_result_t ASTARTE_RESULT_OK if no error occurrs, an error otherwise.
 */
edgehog_result_t publish_network_properties(edgehog_device_handle_t edgehog_device);

#ifdef __cplusplus
}
#endif

#endif // NETWORK_PROPERTIES_H
