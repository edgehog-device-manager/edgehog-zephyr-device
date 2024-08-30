/*
 * (C) Copyright 2024, SECO Mind Srl
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "system_time.h"

#include <zephyr/posix/time.h>

edgehog_result_t system_time_current_ms(int64_t *timestamp_ms)
{
    if (!timestamp_ms) {
        return EDGEHOG_RESULT_INVALID_PARAM;
    }

    struct timespec tspec;
    int res = clock_gettime(CLOCK_REALTIME, &tspec);

    if (res != 0) {
        return EDGEHOG_RESULT_INTERNAL_ERROR;
    }

    *timestamp_ms = (int64_t) tspec.tv_sec * MSEC_PER_SEC + (tspec.tv_nsec / NSEC_PER_MSEC);

    return EDGEHOG_RESULT_OK;
}
