/*
 * (C) Copyright 2024, SECO Mind Srl
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "edgehog_device/battery_status.h"

#include "edgehog_private.h"
#include "generated_interfaces.h"
#include "system_time.h"

#include <astarte_device_sdk/device.h>
#include <astarte_device_sdk/result.h>

#include "log.h"
EDGEHOG_LOG_MODULE_REGISTER(battery_status, CONFIG_EDGEHOG_DEVICE_BATTERY_STATUS_LOG_LEVEL);

/************************************************
 *         Static functions declaration         *
 ***********************************************/

/**
 * @brief Convert edgehog battery state to string.
 *
 * @param edgehog_battery_state Edgehog battery state.
 */
static const char *edgehog_battery_to_code(edgehog_battery_state_t state);

/************************************************
 *         Global functions definitions         *
 ***********************************************/

edgehog_result_t edgehog_battery_status_publish(
    edgehog_device_handle_t edgehog_device, const edgehog_battery_status_t *battery_status)
{
    if (!edgehog_device) {
        EDGEHOG_LOG_ERR("Unable to publish battery status, Edgehog device undefined");
        return EDGEHOG_RESULT_INTERNAL_ERROR;
    }

    const char *battery_state = edgehog_battery_to_code(battery_status->battery_state);
    astarte_object_entry_t object_entries[] = { { .path = "levelPercentage",
                                                    .individual = astarte_individual_from_double(
                                                        battery_status->level_percentage) },
        { .path = "levelAbsoluteError",
            .individual = astarte_individual_from_double(battery_status->level_absolute_error) },
        { .path = "status", .individual = astarte_individual_from_string(battery_state) } };

    int64_t timestamp_ms = 0;
    system_time_current_ms(&timestamp_ms);

    size_t path_size = strlen(battery_status->battery_slot) + 2;
    char path[path_size];
    memset(path, 0, path_size);

    int snprintf_rc = snprintf(path, path_size, "/%s", battery_status->battery_slot);
    if (snprintf_rc != path_size) {
        EDGEHOG_LOG_ERR("Failure in formatting the Astarte path.");
        return EDGEHOG_RESULT_INTERNAL_ERROR;
    }

    astarte_result_t res = astarte_device_stream_aggregated(edgehog_device->astarte_device,
        io_edgehog_devicemanager_BatteryStatus.name, path, object_entries,
        ARRAY_SIZE(object_entries), &timestamp_ms);
    if (res != ASTARTE_RESULT_OK) {
        edgehog_device->astarte_error = res;
        EDGEHOG_LOG_ERR("Unable to send battery status, error: %s.", astarte_result_to_name(res));
        return EDGEHOG_RESULT_ASTARTE_ERROR;
    }

    return EDGEHOG_RESULT_OK;
}

/************************************************
 *         Static functions definitions         *
 ***********************************************/

static const char *edgehog_battery_to_code(edgehog_battery_state_t state)
{
    switch (state) {
        case BATTERY_IDLE:
            return "Idle";
        case BATTERY_CHARGING:
            return "Charging";
        case BATTERY_DISCHARGING:
            return "Discharging";
        case BATTERY_IDLE_OR_CHARGING:
            return "EitherIdleOrCharging";
        case BATTERY_FAILURE:
            return "Failure";
        case BATTERY_REMOVED:
            return "Removed";
        case BATTERY_UNKNOWN:
            return "Unknown";
        default:
            return "";
    }
}
