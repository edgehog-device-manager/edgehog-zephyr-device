/*
 * (C) Copyright 2024, SECO Mind Srl
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SYSTEM_TIME_H
#define SYSTEM_TIME_H

/**
 * @file system_time.h
 * @brief System time utility functions
 */

#include "edgehog_device/result.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Gets the current system time in milliseconds.
 *
 * @param[out] timestamp_ms The current time value in ms.
 *
 * @return EDGEHOG_RESULT_OK if successful, otherwise an error code.
 */
edgehog_result_t system_time_current_ms(int64_t *timestamp_ms);

#ifdef __cplusplus
}
#endif

#endif /* SYSTEM_TIME_H */
