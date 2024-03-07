/*
 * (C) Copyright 2024, SECO Mind Srl
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "edgehog_device/edgehog_device.h"
#include "edgehog_private.h"

#include <stdlib.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(edgehog_device); // NOLINT

edgehog_device_handle_t edgehog_device_new(edgehog_device_config_t *config)
{
    if (!config) {
        LOG_ERR("Unable to init Edgehog device, no config provided"); // NOLINT
        return NULL;
    }

    if (!config->astarte_device) {
        LOG_ERR("Unable to init Edgehog device, Astarte device was NULL"); // NOLINT
        return NULL;
    }

    edgehog_device_handle_t edgehog_device = calloc(1, sizeof(struct edgehog_device_t));
    if (!edgehog_device) {
        LOG_ERR("Out of memory %s: %d", __FILE__, __LINE__); // NOLINT
        return NULL;
    }

    edgehog_device->astarte_device = config->astarte_device;

    return edgehog_device;
}

void edgehog_device_destroy(edgehog_device_handle_t edgehog_device)
{
    free(edgehog_device);
}
