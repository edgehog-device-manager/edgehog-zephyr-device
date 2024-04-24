/*
 * (C) Copyright 2024, SECO Mind Srl
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef UTIL_H
#define UTIL_H

/**
 * @file util.h
 * @brief Utility functions
 */

#include <astarte_device_sdk/interface.h>

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

bool check_empty_string_property(
    const astarte_interface_t *interface, const char *property, const char *value);

#ifdef __cplusplus
}
#endif

#endif // UTIL_H

