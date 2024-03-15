/*
 * (C) Copyright 2024, SECO Mind Srl
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef EDGEHOG_PRIVATE_H
#define EDGEHOG_PRIVATE_H

/**
 * @file edgehog_private.h
 * @brief Private Edgehog Device APIs and fields
 */

#include <astarte_device_sdk/device.h>


#define ASTARTE_UUID_LEN 39

struct edgehog_device_t
{
    astarte_device_handle_t astarte_device;
    char boot_id[ASTARTE_UUID_LEN];
};

#endif // EDGEHOG_PRIVATE_H
