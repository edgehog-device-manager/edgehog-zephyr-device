/*
 * (C) Copyright 2024, SECO Mind Srl
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "edgehog_device/edgehog_device.h"

#include <zephyr/logging/log.h>

#include <stdlib.h>

LOG_MODULE_REGISTER(edgehog_device); // NOLINT

struct edgehog_device_t
{
    void *fake_field;
};

edgehog_device_handle_t edgehog_device_new()
{
    edgehog_device_handle_t edgehog_device = calloc(1, sizeof(struct edgehog_device_t));
    if (!edgehog_device) {
        LOG_ERR("Out of memory %s: %d", __FILE__, __LINE__); // NOLINT
        return NULL;
    }
    return edgehog_device;
}

void edgehog_device_destroy(edgehog_device_handle_t edgehog_device)
{
    free(edgehog_device);
}
