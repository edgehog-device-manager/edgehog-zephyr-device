/*
 * (C) Copyright 2024, SECO Mind Srl
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "system_status.h"

#include "edgehog_private.h"
#include "hardware_info.h"

#include <time.h>

#include <zephyr/kernel.h>

#include <astarte_device_sdk/device.h>
#include <astarte_device_sdk/result.h>

#include "generated_interfaces.h"
#include "log.h"
EDGEHOG_LOG_MODULE_REGISTER(system_status, CONFIG_EDGEHOG_DEVICE_SYSTEM_STATUS_LOG_LEVEL);

/************************************************
 *        Defines, constants and typedef        *
 ***********************************************/

typedef struct
{
    /** @brief Size of allocated thread's stack. */
    size_t stack_size;
    /** @brief Size of allocated thread's stack free. */
    size_t stack_size_free;
    /** @brief Number of allocated/running threads. */
    int count;
} thread_info_t;

/* This callback counts the number of thread and sums stack size and free stack for each threads. */
static void k_thread_stack_sum_cb(const struct k_thread *thread, void *user_data)
{
#if defined(CONFIG_INIT_STACKS) && defined(CONFIG_THREAD_STACK_INFO)
    size_t unused = thread->stack_info.size;

    thread_info_t *thread_info = (thread_info_t *) user_data;
    if (k_thread_stack_space_get(thread, &unused) == 0) {
        thread_info->count = thread_info->count + 1;
        thread_info->stack_size_free = thread_info->stack_size_free + unused;
        thread_info->stack_size = thread_info->stack_size + thread->stack_info.size;
    }
#endif
}

void publish_system_status(edgehog_device_handle_t edgehog_device)
{

    thread_info_t thread_info = { 0 };
    k_thread_foreach_unlocked(k_thread_stack_sum_cb, &thread_info);

    size_t memory_size = 0;
    size_t avail_memory = 0;

    if (hardware_info_get_memory_size(&memory_size)) {
        avail_memory = memory_size - (thread_info.stack_size - thread_info.stack_size_free);
    } else {
        avail_memory = thread_info.stack_size_free;
    }

    astarte_object_entry_t object_entries[] = {
        { .path = "availMemoryBytes",
            .individual = astarte_individual_from_longinteger(avail_memory) },
        { .path = "bootId", .individual = astarte_individual_from_string(edgehog_device->boot_id) },
        { .path = "taskCount", .individual = astarte_individual_from_integer(thread_info.count) },
        { .path = "uptimeMillis",
            .individual = astarte_individual_from_longinteger(k_uptime_get()) },
    };

    const int64_t timestamp = (int64_t) time(NULL);

    astarte_result_t res = astarte_device_stream_aggregated(edgehog_device->astarte_device,
        io_edgehog_devicemanager_SystemStatus.name, "/systemStatus", object_entries,
        ARRAY_SIZE(object_entries), &timestamp);
    if (res != ASTARTE_RESULT_OK) {
        EDGEHOG_LOG_ERR("Unable to send system_status"); // NOLINT
    }
}
