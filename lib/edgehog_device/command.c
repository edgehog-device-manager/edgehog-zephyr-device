/*
 * (C) Copyright 2024, SECO Mind Srl
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "command.h"

#include <zephyr/kernel.h>
#include <zephyr/sys/reboot.h>

#include "log.h"
EDGEHOG_LOG_MODULE_REGISTER(command, CONFIG_EDGEHOG_DEVICE_COMMAND_LOG_LEVEL);

edgehog_result_t edgehog_command_event(astarte_device_datastream_individual_event_t *event_request)
{
    astarte_data_t command_data = event_request->data;

    if (strcmp(command_data.data.string, "Reboot") == 0) {
        EDGEHOG_LOG_INF("Device restart in 1 second");
        k_sleep(K_SECONDS(1));
        EDGEHOG_LOG_INF("Device restart now");
        sys_reboot(SYS_REBOOT_WARM);
    } else {
        EDGEHOG_LOG_ERR(
            "Unable to handle command event, command %s unsupported", command_data.data.string);
        return EDGEHOG_RESULT_COMMAND_INVALID_REQUEST;
    }
}
