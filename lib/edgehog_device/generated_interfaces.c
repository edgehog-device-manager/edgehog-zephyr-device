/**
 * @file generated_interfaces.c
 * @brief Contains automatically generated interfaces.
 *
 * @warning Do not modify this file manually.
 */

#include "generated_interfaces.h"

// Interface names should resemble as closely as possible their respective .json file names.
// NOLINTBEGIN(readability-identifier-naming)

static const astarte_mapping_t io_edgehog_devicemanager_BaseImage_mappings[4] = {

    {
        .endpoint = "/fingerprint",
        .type = ASTARTE_MAPPING_TYPE_STRING,
        .reliability = ASTARTE_MAPPING_RELIABILITY_UNIQUE,
        .explicit_timestamp = false,
        .allow_unset = false,
    },
    {
        .endpoint = "/name",
        .type = ASTARTE_MAPPING_TYPE_STRING,
        .reliability = ASTARTE_MAPPING_RELIABILITY_UNIQUE,
        .explicit_timestamp = false,
        .allow_unset = false,
    },
    {
        .endpoint = "/version",
        .type = ASTARTE_MAPPING_TYPE_STRING,
        .reliability = ASTARTE_MAPPING_RELIABILITY_UNIQUE,
        .explicit_timestamp = false,
        .allow_unset = false,
    },
    {
        .endpoint = "/buildId",
        .type = ASTARTE_MAPPING_TYPE_STRING,
        .reliability = ASTARTE_MAPPING_RELIABILITY_UNIQUE,
        .explicit_timestamp = false,
        .allow_unset = false,
    },
};

const astarte_interface_t io_edgehog_devicemanager_BaseImage = {
    .name = "io.edgehog.devicemanager.BaseImage",
    .major_version = 0,
    .minor_version = 1,
    .type = ASTARTE_INTERFACE_TYPE_PROPERTIES,
    .ownership = ASTARTE_INTERFACE_OWNERSHIP_DEVICE,
    .aggregation = ASTARTE_INTERFACE_AGGREGATION_INDIVIDUAL,
    .mappings = io_edgehog_devicemanager_BaseImage_mappings,
    .mappings_length = 4U,
};

static const astarte_mapping_t io_edgehog_devicemanager_BatteryStatus_mappings[3] = {

    {
        .endpoint = "/%{battery_slot}/levelPercentage",
        .type = ASTARTE_MAPPING_TYPE_DOUBLE,
        .reliability = ASTARTE_MAPPING_RELIABILITY_UNRELIABLE,
        .explicit_timestamp = true,
        .allow_unset = false,
    },
    {
        .endpoint = "/%{battery_slot}/levelAbsoluteError",
        .type = ASTARTE_MAPPING_TYPE_DOUBLE,
        .reliability = ASTARTE_MAPPING_RELIABILITY_UNRELIABLE,
        .explicit_timestamp = true,
        .allow_unset = false,
    },
    {
        .endpoint = "/%{battery_slot}/status",
        .type = ASTARTE_MAPPING_TYPE_STRING,
        .reliability = ASTARTE_MAPPING_RELIABILITY_UNRELIABLE,
        .explicit_timestamp = true,
        .allow_unset = false,
    },
};

const astarte_interface_t io_edgehog_devicemanager_BatteryStatus = {
    .name = "io.edgehog.devicemanager.BatteryStatus",
    .major_version = 0,
    .minor_version = 1,
    .type = ASTARTE_INTERFACE_TYPE_DATASTREAM,
    .ownership = ASTARTE_INTERFACE_OWNERSHIP_DEVICE,
    .aggregation = ASTARTE_INTERFACE_AGGREGATION_OBJECT,
    .mappings = io_edgehog_devicemanager_BatteryStatus_mappings,
    .mappings_length = 3U,
};

static const astarte_mapping_t io_edgehog_devicemanager_Commands_mappings[1] = {

    {
        .endpoint = "/request",
        .type = ASTARTE_MAPPING_TYPE_STRING,
        .reliability = ASTARTE_MAPPING_RELIABILITY_UNIQUE,
        .explicit_timestamp = true,
        .allow_unset = false,
    },
};

const astarte_interface_t io_edgehog_devicemanager_Commands = {
    .name = "io.edgehog.devicemanager.Commands",
    .major_version = 0,
    .minor_version = 1,
    .type = ASTARTE_INTERFACE_TYPE_DATASTREAM,
    .ownership = ASTARTE_INTERFACE_OWNERSHIP_SERVER,
    .aggregation = ASTARTE_INTERFACE_AGGREGATION_INDIVIDUAL,
    .mappings = io_edgehog_devicemanager_Commands_mappings,
    .mappings_length = 1U,
};

static const astarte_mapping_t io_edgehog_devicemanager_HardwareInfo_mappings[5] = {

    {
        .endpoint = "/cpu/architecture",
        .type = ASTARTE_MAPPING_TYPE_STRING,
        .reliability = ASTARTE_MAPPING_RELIABILITY_UNIQUE,
        .explicit_timestamp = false,
        .allow_unset = false,
    },
    {
        .endpoint = "/cpu/model",
        .type = ASTARTE_MAPPING_TYPE_STRING,
        .reliability = ASTARTE_MAPPING_RELIABILITY_UNIQUE,
        .explicit_timestamp = false,
        .allow_unset = false,
    },
    {
        .endpoint = "/cpu/modelName",
        .type = ASTARTE_MAPPING_TYPE_STRING,
        .reliability = ASTARTE_MAPPING_RELIABILITY_UNIQUE,
        .explicit_timestamp = false,
        .allow_unset = false,
    },
    {
        .endpoint = "/cpu/vendor",
        .type = ASTARTE_MAPPING_TYPE_STRING,
        .reliability = ASTARTE_MAPPING_RELIABILITY_UNIQUE,
        .explicit_timestamp = false,
        .allow_unset = false,
    },
    {
        .endpoint = "/mem/totalBytes",
        .type = ASTARTE_MAPPING_TYPE_LONGINTEGER,
        .reliability = ASTARTE_MAPPING_RELIABILITY_UNIQUE,
        .explicit_timestamp = false,
        .allow_unset = false,
    },
};

const astarte_interface_t io_edgehog_devicemanager_HardwareInfo = {
    .name = "io.edgehog.devicemanager.HardwareInfo",
    .major_version = 0,
    .minor_version = 1,
    .type = ASTARTE_INTERFACE_TYPE_PROPERTIES,
    .ownership = ASTARTE_INTERFACE_OWNERSHIP_DEVICE,
    .aggregation = ASTARTE_INTERFACE_AGGREGATION_INDIVIDUAL,
    .mappings = io_edgehog_devicemanager_HardwareInfo_mappings,
    .mappings_length = 5U,
};

static const astarte_mapping_t io_edgehog_devicemanager_LedBehavior_mappings[1] = {

    {
        .endpoint = "/%{led_id}/behavior",
        .type = ASTARTE_MAPPING_TYPE_STRING,
        .reliability = ASTARTE_MAPPING_RELIABILITY_UNIQUE,
        .explicit_timestamp = false,
        .allow_unset = false,
    },
};

const astarte_interface_t io_edgehog_devicemanager_LedBehavior = {
    .name = "io.edgehog.devicemanager.LedBehavior",
    .major_version = 0,
    .minor_version = 1,
    .type = ASTARTE_INTERFACE_TYPE_DATASTREAM,
    .ownership = ASTARTE_INTERFACE_OWNERSHIP_SERVER,
    .aggregation = ASTARTE_INTERFACE_AGGREGATION_INDIVIDUAL,
    .mappings = io_edgehog_devicemanager_LedBehavior_mappings,
    .mappings_length = 1U,
};

static const astarte_mapping_t io_edgehog_devicemanager_OSInfo_mappings[2] = {

    {
        .endpoint = "/osName",
        .type = ASTARTE_MAPPING_TYPE_STRING,
        .reliability = ASTARTE_MAPPING_RELIABILITY_UNIQUE,
        .explicit_timestamp = false,
        .allow_unset = false,
    },
    {
        .endpoint = "/osVersion",
        .type = ASTARTE_MAPPING_TYPE_STRING,
        .reliability = ASTARTE_MAPPING_RELIABILITY_UNIQUE,
        .explicit_timestamp = false,
        .allow_unset = false,
    },
};

const astarte_interface_t io_edgehog_devicemanager_OSInfo = {
    .name = "io.edgehog.devicemanager.OSInfo",
    .major_version = 0,
    .minor_version = 1,
    .type = ASTARTE_INTERFACE_TYPE_PROPERTIES,
    .ownership = ASTARTE_INTERFACE_OWNERSHIP_DEVICE,
    .aggregation = ASTARTE_INTERFACE_AGGREGATION_INDIVIDUAL,
    .mappings = io_edgehog_devicemanager_OSInfo_mappings,
    .mappings_length = 2U,
};

static const astarte_mapping_t io_edgehog_devicemanager_OTAEvent_mappings[5] = {

    {
        .endpoint = "/event/requestUUID",
        .type = ASTARTE_MAPPING_TYPE_STRING,
        .reliability = ASTARTE_MAPPING_RELIABILITY_UNIQUE,
        .explicit_timestamp = true,
        .allow_unset = false,
    },
    {
        .endpoint = "/event/status",
        .type = ASTARTE_MAPPING_TYPE_STRING,
        .reliability = ASTARTE_MAPPING_RELIABILITY_UNIQUE,
        .explicit_timestamp = true,
        .allow_unset = false,
    },
    {
        .endpoint = "/event/statusProgress",
        .type = ASTARTE_MAPPING_TYPE_INTEGER,
        .reliability = ASTARTE_MAPPING_RELIABILITY_UNIQUE,
        .explicit_timestamp = true,
        .allow_unset = false,
    },
    {
        .endpoint = "/event/statusCode",
        .type = ASTARTE_MAPPING_TYPE_STRING,
        .reliability = ASTARTE_MAPPING_RELIABILITY_UNIQUE,
        .explicit_timestamp = true,
        .allow_unset = false,
    },
    {
        .endpoint = "/event/message",
        .type = ASTARTE_MAPPING_TYPE_STRING,
        .reliability = ASTARTE_MAPPING_RELIABILITY_UNIQUE,
        .explicit_timestamp = true,
        .allow_unset = false,
    },
};

const astarte_interface_t io_edgehog_devicemanager_OTAEvent = {
    .name = "io.edgehog.devicemanager.OTAEvent",
    .major_version = 0,
    .minor_version = 1,
    .type = ASTARTE_INTERFACE_TYPE_DATASTREAM,
    .ownership = ASTARTE_INTERFACE_OWNERSHIP_DEVICE,
    .aggregation = ASTARTE_INTERFACE_AGGREGATION_OBJECT,
    .mappings = io_edgehog_devicemanager_OTAEvent_mappings,
    .mappings_length = 5U,
};

static const astarte_mapping_t io_edgehog_devicemanager_OTARequest_mappings[3] = {

    {
        .endpoint = "/request/operation",
        .type = ASTARTE_MAPPING_TYPE_STRING,
        .reliability = ASTARTE_MAPPING_RELIABILITY_UNRELIABLE,
        .explicit_timestamp = false,
        .allow_unset = false,
    },
    {
        .endpoint = "/request/url",
        .type = ASTARTE_MAPPING_TYPE_STRING,
        .reliability = ASTARTE_MAPPING_RELIABILITY_UNRELIABLE,
        .explicit_timestamp = false,
        .allow_unset = false,
    },
    {
        .endpoint = "/request/uuid",
        .type = ASTARTE_MAPPING_TYPE_STRING,
        .reliability = ASTARTE_MAPPING_RELIABILITY_UNRELIABLE,
        .explicit_timestamp = false,
        .allow_unset = false,
    },
};

const astarte_interface_t io_edgehog_devicemanager_OTARequest = {
    .name = "io.edgehog.devicemanager.OTARequest",
    .major_version = 1,
    .minor_version = 0,
    .type = ASTARTE_INTERFACE_TYPE_DATASTREAM,
    .ownership = ASTARTE_INTERFACE_OWNERSHIP_SERVER,
    .aggregation = ASTARTE_INTERFACE_AGGREGATION_OBJECT,
    .mappings = io_edgehog_devicemanager_OTARequest_mappings,
    .mappings_length = 3U,
};

static const astarte_mapping_t io_edgehog_devicemanager_RuntimeInfo_mappings[4] = {

    {
        .endpoint = "/name",
        .type = ASTARTE_MAPPING_TYPE_STRING,
        .reliability = ASTARTE_MAPPING_RELIABILITY_UNIQUE,
        .explicit_timestamp = false,
        .allow_unset = false,
    },
    {
        .endpoint = "/url",
        .type = ASTARTE_MAPPING_TYPE_STRING,
        .reliability = ASTARTE_MAPPING_RELIABILITY_UNIQUE,
        .explicit_timestamp = false,
        .allow_unset = false,
    },
    {
        .endpoint = "/version",
        .type = ASTARTE_MAPPING_TYPE_STRING,
        .reliability = ASTARTE_MAPPING_RELIABILITY_UNIQUE,
        .explicit_timestamp = false,
        .allow_unset = false,
    },
    {
        .endpoint = "/environment",
        .type = ASTARTE_MAPPING_TYPE_STRING,
        .reliability = ASTARTE_MAPPING_RELIABILITY_UNIQUE,
        .explicit_timestamp = false,
        .allow_unset = false,
    },
};

const astarte_interface_t io_edgehog_devicemanager_RuntimeInfo = {
    .name = "io.edgehog.devicemanager.RuntimeInfo",
    .major_version = 0,
    .minor_version = 1,
    .type = ASTARTE_INTERFACE_TYPE_PROPERTIES,
    .ownership = ASTARTE_INTERFACE_OWNERSHIP_DEVICE,
    .aggregation = ASTARTE_INTERFACE_AGGREGATION_INDIVIDUAL,
    .mappings = io_edgehog_devicemanager_RuntimeInfo_mappings,
    .mappings_length = 4U,
};

static const astarte_mapping_t io_edgehog_devicemanager_StorageUsage_mappings[2] = {

    {
        .endpoint = "/%{label}/totalBytes",
        .type = ASTARTE_MAPPING_TYPE_LONGINTEGER,
        .reliability = ASTARTE_MAPPING_RELIABILITY_UNRELIABLE,
        .explicit_timestamp = true,
        .allow_unset = false,
    },
    {
        .endpoint = "/%{label}/freeBytes",
        .type = ASTARTE_MAPPING_TYPE_LONGINTEGER,
        .reliability = ASTARTE_MAPPING_RELIABILITY_UNRELIABLE,
        .explicit_timestamp = true,
        .allow_unset = false,
    },
};

const astarte_interface_t io_edgehog_devicemanager_StorageUsage = {
    .name = "io.edgehog.devicemanager.StorageUsage",
    .major_version = 0,
    .minor_version = 1,
    .type = ASTARTE_INTERFACE_TYPE_DATASTREAM,
    .ownership = ASTARTE_INTERFACE_OWNERSHIP_DEVICE,
    .aggregation = ASTARTE_INTERFACE_AGGREGATION_OBJECT,
    .mappings = io_edgehog_devicemanager_StorageUsage_mappings,
    .mappings_length = 2U,
};

static const astarte_mapping_t io_edgehog_devicemanager_SystemInfo_mappings[2] = {

    {
        .endpoint = "/serialNumber",
        .type = ASTARTE_MAPPING_TYPE_STRING,
        .reliability = ASTARTE_MAPPING_RELIABILITY_UNIQUE,
        .explicit_timestamp = false,
        .allow_unset = false,
    },
    {
        .endpoint = "/partNumber",
        .type = ASTARTE_MAPPING_TYPE_STRING,
        .reliability = ASTARTE_MAPPING_RELIABILITY_UNIQUE,
        .explicit_timestamp = false,
        .allow_unset = false,
    },
};

const astarte_interface_t io_edgehog_devicemanager_SystemInfo = {
    .name = "io.edgehog.devicemanager.SystemInfo",
    .major_version = 0,
    .minor_version = 1,
    .type = ASTARTE_INTERFACE_TYPE_PROPERTIES,
    .ownership = ASTARTE_INTERFACE_OWNERSHIP_DEVICE,
    .aggregation = ASTARTE_INTERFACE_AGGREGATION_INDIVIDUAL,
    .mappings = io_edgehog_devicemanager_SystemInfo_mappings,
    .mappings_length = 2U,
};

static const astarte_mapping_t io_edgehog_devicemanager_SystemStatus_mappings[4] = {

    {
        .endpoint = "/systemStatus/availMemoryBytes",
        .type = ASTARTE_MAPPING_TYPE_LONGINTEGER,
        .reliability = ASTARTE_MAPPING_RELIABILITY_UNRELIABLE,
        .explicit_timestamp = true,
        .allow_unset = false,
    },
    {
        .endpoint = "/systemStatus/bootId",
        .type = ASTARTE_MAPPING_TYPE_STRING,
        .reliability = ASTARTE_MAPPING_RELIABILITY_UNRELIABLE,
        .explicit_timestamp = true,
        .allow_unset = false,
    },
    {
        .endpoint = "/systemStatus/taskCount",
        .type = ASTARTE_MAPPING_TYPE_INTEGER,
        .reliability = ASTARTE_MAPPING_RELIABILITY_UNRELIABLE,
        .explicit_timestamp = true,
        .allow_unset = false,
    },
    {
        .endpoint = "/systemStatus/uptimeMillis",
        .type = ASTARTE_MAPPING_TYPE_LONGINTEGER,
        .reliability = ASTARTE_MAPPING_RELIABILITY_UNRELIABLE,
        .explicit_timestamp = true,
        .allow_unset = false,
    },
};

const astarte_interface_t io_edgehog_devicemanager_SystemStatus = {
    .name = "io.edgehog.devicemanager.SystemStatus",
    .major_version = 0,
    .minor_version = 1,
    .type = ASTARTE_INTERFACE_TYPE_DATASTREAM,
    .ownership = ASTARTE_INTERFACE_OWNERSHIP_DEVICE,
    .aggregation = ASTARTE_INTERFACE_AGGREGATION_OBJECT,
    .mappings = io_edgehog_devicemanager_SystemStatus_mappings,
    .mappings_length = 4U,
};

static const astarte_mapping_t io_edgehog_devicemanager_WiFiScanResults_mappings[5] = {

    {
        .endpoint = "/ap/channel",
        .type = ASTARTE_MAPPING_TYPE_INTEGER,
        .reliability = ASTARTE_MAPPING_RELIABILITY_UNRELIABLE,
        .explicit_timestamp = true,
        .allow_unset = false,
    },
    {
        .endpoint = "/ap/connected",
        .type = ASTARTE_MAPPING_TYPE_BOOLEAN,
        .reliability = ASTARTE_MAPPING_RELIABILITY_UNRELIABLE,
        .explicit_timestamp = true,
        .allow_unset = false,
    },
    {
        .endpoint = "/ap/essid",
        .type = ASTARTE_MAPPING_TYPE_STRING,
        .reliability = ASTARTE_MAPPING_RELIABILITY_UNRELIABLE,
        .explicit_timestamp = true,
        .allow_unset = false,
    },
    {
        .endpoint = "/ap/macAddress",
        .type = ASTARTE_MAPPING_TYPE_STRING,
        .reliability = ASTARTE_MAPPING_RELIABILITY_UNRELIABLE,
        .explicit_timestamp = true,
        .allow_unset = false,
    },
    {
        .endpoint = "/ap/rssi",
        .type = ASTARTE_MAPPING_TYPE_INTEGER,
        .reliability = ASTARTE_MAPPING_RELIABILITY_UNRELIABLE,
        .explicit_timestamp = true,
        .allow_unset = false,
    },
};

const astarte_interface_t io_edgehog_devicemanager_WiFiScanResults = {
    .name = "io.edgehog.devicemanager.WiFiScanResults",
    .major_version = 0,
    .minor_version = 2,
    .type = ASTARTE_INTERFACE_TYPE_DATASTREAM,
    .ownership = ASTARTE_INTERFACE_OWNERSHIP_DEVICE,
    .aggregation = ASTARTE_INTERFACE_AGGREGATION_OBJECT,
    .mappings = io_edgehog_devicemanager_WiFiScanResults_mappings,
    .mappings_length = 5U,
};

static const astarte_mapping_t io_edgehog_devicemanager_config_Telemetry_mappings[2] = {

    {
        .endpoint = "/request/%{interface_name}/enable",
        .type = ASTARTE_MAPPING_TYPE_BOOLEAN,
        .reliability = ASTARTE_MAPPING_RELIABILITY_UNIQUE,
        .explicit_timestamp = false,
        .allow_unset = true,
    },
    {
        .endpoint = "/request/%{interface_name}/periodSeconds",
        .type = ASTARTE_MAPPING_TYPE_LONGINTEGER,
        .reliability = ASTARTE_MAPPING_RELIABILITY_UNIQUE,
        .explicit_timestamp = false,
        .allow_unset = true,
    },
};

const astarte_interface_t io_edgehog_devicemanager_config_Telemetry = {
    .name = "io.edgehog.devicemanager.config.Telemetry",
    .major_version = 0,
    .minor_version = 1,
    .type = ASTARTE_INTERFACE_TYPE_PROPERTIES,
    .ownership = ASTARTE_INTERFACE_OWNERSHIP_SERVER,
    .aggregation = ASTARTE_INTERFACE_AGGREGATION_INDIVIDUAL,
    .mappings = io_edgehog_devicemanager_config_Telemetry_mappings,
    .mappings_length = 2U,
};

// NOLINTEND(readability-identifier-naming)
