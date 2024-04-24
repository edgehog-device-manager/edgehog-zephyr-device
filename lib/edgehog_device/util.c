/*
 * (C) Copyright 2024, SECO Mind Srl
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "util.h"
#include "log.h"

#include <string.h>

EDGEHOG_LOG_MODULE_REGISTER(util, CONFIG_EDGEHOG_DEVICE_UTIL_LOG_LEVEL);

/************************************************
 * Global functions definition
 ***********************************************/

bool check_empty_string_property(
    const astarte_interface_t *interface, const char *property, const char *value)
{
    if (strcmp(value, "") == 0) {
        EDGEHOG_LOG_DBG("The property '%s' of interface '%s' is empty", property, interface->name);
        return true;
    }

    return false;
}
