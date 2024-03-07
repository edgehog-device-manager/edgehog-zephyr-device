/*
 * (C) Copyright 2024, SECO Mind Srl
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef EDGEHOG_DEVICE_EDGEHOG_PRIVATE_H
#define EDGEHOG_DEVICE_EDGEHOG_PRIVATE_H

/**
 * @file edgehog_private.h
 * @brief Private Edgehog Device APIs and fields
 */

#include <astarte_device_sdk/device.h>

struct edgehog_device_t
{
    astarte_device_handle_t astarte_device;
};

#endif // EDGEHOG_DEVICE_EDGEHOG_PRIVATE_H