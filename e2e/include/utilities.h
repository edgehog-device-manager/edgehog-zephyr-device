/*
 * (C) Copyright 2024, SECO Mind Srl
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef UTILITIES_H
#define UTILITIES_H

#include <zephyr/fatal.h>
#include <zephyr/logging/log.h>
#include <zephyr/shell/shell.h>

#include "edgehog_device/result.h"

#define CHECK_HALT(expr, ...)                                                                      \
    do {                                                                                           \
        if (unlikely(expr)) {                                                                      \
            __VA_OPT__(LOG_ERR(__VA_ARGS__);)                                                      \
            k_fatal_halt(-1);                                                                      \
        }                                                                                          \
    } while (0)

#define CHECK_EDGEHOG_OK_HALT(expr, ...) CHECK_HALT(expr != EDGEHOG_RESULT_OK, __VA_ARGS__)

#define CHECK_RET_1(expr, ...)                                                                     \
    do {                                                                                           \
        if (unlikely(expr)) {                                                                      \
            __VA_OPT__(LOG_ERR(__VA_ARGS__);)                                                      \
            return 1;                                                                              \
        }                                                                                          \
    } while (0)

#define CHECK_EDGEHOG_OK_RET_1(expr, ...) CHECK_RET_1(expr != EDGEHOG_RESULT_OK, __VA_ARGS__)

#define CHECK_GOTO(expr, label, ...)                                                               \
    do {                                                                                           \
        if (unlikely(expr)) {                                                                      \
            __VA_OPT__(LOG_ERR(__VA_ARGS__);)                                                      \
            goto label;                                                                            \
        }                                                                                          \
    } while (0)

#define CHECK_EDGEHOG_OK_GOTO(expr, label, ...)                                                    \
    CHECK_GOTO(expr != EDGEHOG_RESULT_OK, label, __VA_ARGS__)

#endif /* UTILITIES_H */
