/**
 * @file generated_interfaces.c
 * @brief Contains automatically generated interfaces.
 *
 * @warning Do not modify this file manually.
 */

// clang-format off

#include "generated_interfaces.h"

// Interface names should resemble as closely as possible their respective .json file names.
// NOLINTBEGIN(readability-identifier-naming)

/** @brief Automatically generated mapping definition. */
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

/** @brief Automatically generated interface definition. */
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

/** @brief Automatically generated mapping definition. */
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

/** @brief Automatically generated interface definition. */
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

/** @brief Automatically generated mapping definition. */
static const astarte_mapping_t io_edgehog_devicemanager_CellularConnectionProperties_mappings[3] = {

    {
        .endpoint = "/%{id}/apn",
        .type = ASTARTE_MAPPING_TYPE_STRING,
        .reliability = ASTARTE_MAPPING_RELIABILITY_UNIQUE,
        .explicit_timestamp = false,
        .allow_unset = true,
    },
    {
        .endpoint = "/%{id}/imei",
        .type = ASTARTE_MAPPING_TYPE_STRING,
        .reliability = ASTARTE_MAPPING_RELIABILITY_UNIQUE,
        .explicit_timestamp = false,
        .allow_unset = true,
    },
    {
        .endpoint = "/%{id}/imsi",
        .type = ASTARTE_MAPPING_TYPE_STRING,
        .reliability = ASTARTE_MAPPING_RELIABILITY_UNIQUE,
        .explicit_timestamp = false,
        .allow_unset = true,
    },
};

/** @brief Automatically generated interface definition. */
const astarte_interface_t io_edgehog_devicemanager_CellularConnectionProperties = {
    .name = "io.edgehog.devicemanager.CellularConnectionProperties",
    .major_version = 0,
    .minor_version = 1,
    .type = ASTARTE_INTERFACE_TYPE_PROPERTIES,
    .ownership = ASTARTE_INTERFACE_OWNERSHIP_DEVICE,
    .aggregation = ASTARTE_INTERFACE_AGGREGATION_INDIVIDUAL,
    .mappings = io_edgehog_devicemanager_CellularConnectionProperties_mappings,
    .mappings_length = 3U,
};

/** @brief Automatically generated mapping definition. */
static const astarte_mapping_t io_edgehog_devicemanager_CellularConnectionStatus_mappings[8] = {

    {
        .endpoint = "/%{id}/carrier",
        .type = ASTARTE_MAPPING_TYPE_STRING,
        .reliability = ASTARTE_MAPPING_RELIABILITY_UNRELIABLE,
        .explicit_timestamp = true,
        .allow_unset = false,
    },
    {
        .endpoint = "/%{id}/cellId",
        .type = ASTARTE_MAPPING_TYPE_LONGINTEGER,
        .reliability = ASTARTE_MAPPING_RELIABILITY_UNRELIABLE,
        .explicit_timestamp = true,
        .allow_unset = false,
    },
    {
        .endpoint = "/%{id}/mobileCountryCode",
        .type = ASTARTE_MAPPING_TYPE_INTEGER,
        .reliability = ASTARTE_MAPPING_RELIABILITY_UNRELIABLE,
        .explicit_timestamp = true,
        .allow_unset = false,
    },
    {
        .endpoint = "/%{id}/mobileNetworkCode",
        .type = ASTARTE_MAPPING_TYPE_INTEGER,
        .reliability = ASTARTE_MAPPING_RELIABILITY_UNRELIABLE,
        .explicit_timestamp = true,
        .allow_unset = false,
    },
    {
        .endpoint = "/%{id}/localAreaCode",
        .type = ASTARTE_MAPPING_TYPE_INTEGER,
        .reliability = ASTARTE_MAPPING_RELIABILITY_UNRELIABLE,
        .explicit_timestamp = true,
        .allow_unset = false,
    },
    {
        .endpoint = "/%{id}/registrationStatus",
        .type = ASTARTE_MAPPING_TYPE_STRING,
        .reliability = ASTARTE_MAPPING_RELIABILITY_UNRELIABLE,
        .explicit_timestamp = true,
        .allow_unset = false,
    },
    {
        .endpoint = "/%{id}/rssi",
        .type = ASTARTE_MAPPING_TYPE_DOUBLE,
        .reliability = ASTARTE_MAPPING_RELIABILITY_UNRELIABLE,
        .explicit_timestamp = true,
        .allow_unset = false,
    },
    {
        .endpoint = "/%{id}/technology",
        .type = ASTARTE_MAPPING_TYPE_STRING,
        .reliability = ASTARTE_MAPPING_RELIABILITY_UNRELIABLE,
        .explicit_timestamp = true,
        .allow_unset = false,
    },
};

/** @brief Automatically generated interface definition. */
const astarte_interface_t io_edgehog_devicemanager_CellularConnectionStatus = {
    .name = "io.edgehog.devicemanager.CellularConnectionStatus",
    .major_version = 0,
    .minor_version = 1,
    .type = ASTARTE_INTERFACE_TYPE_DATASTREAM,
    .ownership = ASTARTE_INTERFACE_OWNERSHIP_DEVICE,
    .aggregation = ASTARTE_INTERFACE_AGGREGATION_OBJECT,
    .mappings = io_edgehog_devicemanager_CellularConnectionStatus_mappings,
    .mappings_length = 8U,
};

/** @brief Automatically generated mapping definition. */
static const astarte_mapping_t io_edgehog_devicemanager_Commands_mappings[1] = {

    {
        .endpoint = "/request",
        .type = ASTARTE_MAPPING_TYPE_STRING,
        .reliability = ASTARTE_MAPPING_RELIABILITY_UNIQUE,
        .explicit_timestamp = true,
        .allow_unset = false,
    },
};

/** @brief Automatically generated interface definition. */
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

/** @brief Automatically generated mapping definition. */
static const astarte_mapping_t io_edgehog_devicemanager_ForwarderSessionRequest_mappings[4] = {

    {
        .endpoint = "/request/session_token",
        .type = ASTARTE_MAPPING_TYPE_STRING,
        .reliability = ASTARTE_MAPPING_RELIABILITY_UNRELIABLE,
        .explicit_timestamp = false,
        .allow_unset = false,
    },
    {
        .endpoint = "/request/port",
        .type = ASTARTE_MAPPING_TYPE_INTEGER,
        .reliability = ASTARTE_MAPPING_RELIABILITY_UNRELIABLE,
        .explicit_timestamp = false,
        .allow_unset = false,
    },
    {
        .endpoint = "/request/host",
        .type = ASTARTE_MAPPING_TYPE_STRING,
        .reliability = ASTARTE_MAPPING_RELIABILITY_UNRELIABLE,
        .explicit_timestamp = false,
        .allow_unset = false,
    },
    {
        .endpoint = "/request/secure",
        .type = ASTARTE_MAPPING_TYPE_BOOLEAN,
        .reliability = ASTARTE_MAPPING_RELIABILITY_UNRELIABLE,
        .explicit_timestamp = false,
        .allow_unset = false,
    },
};

/** @brief Automatically generated interface definition. */
const astarte_interface_t io_edgehog_devicemanager_ForwarderSessionRequest = {
    .name = "io.edgehog.devicemanager.ForwarderSessionRequest",
    .major_version = 0,
    .minor_version = 1,
    .type = ASTARTE_INTERFACE_TYPE_DATASTREAM,
    .ownership = ASTARTE_INTERFACE_OWNERSHIP_SERVER,
    .aggregation = ASTARTE_INTERFACE_AGGREGATION_OBJECT,
    .mappings = io_edgehog_devicemanager_ForwarderSessionRequest_mappings,
    .mappings_length = 4U,
};

/** @brief Automatically generated mapping definition. */
static const astarte_mapping_t io_edgehog_devicemanager_ForwarderSessionState_mappings[1] = {

    {
        .endpoint = "/%{session_token}/status",
        .type = ASTARTE_MAPPING_TYPE_STRING,
        .reliability = ASTARTE_MAPPING_RELIABILITY_UNIQUE,
        .explicit_timestamp = false,
        .allow_unset = true,
    },
};

/** @brief Automatically generated interface definition. */
const astarte_interface_t io_edgehog_devicemanager_ForwarderSessionState = {
    .name = "io.edgehog.devicemanager.ForwarderSessionState",
    .major_version = 0,
    .minor_version = 1,
    .type = ASTARTE_INTERFACE_TYPE_PROPERTIES,
    .ownership = ASTARTE_INTERFACE_OWNERSHIP_DEVICE,
    .aggregation = ASTARTE_INTERFACE_AGGREGATION_INDIVIDUAL,
    .mappings = io_edgehog_devicemanager_ForwarderSessionState_mappings,
    .mappings_length = 1U,
};

/** @brief Automatically generated mapping definition. */
static const astarte_mapping_t io_edgehog_devicemanager_Geolocation_mappings[7] = {

    {
        .endpoint = "/%{id}/latitude",
        .type = ASTARTE_MAPPING_TYPE_DOUBLE,
        .reliability = ASTARTE_MAPPING_RELIABILITY_UNRELIABLE,
        .explicit_timestamp = true,
        .allow_unset = false,
    },
    {
        .endpoint = "/%{id}/longitude",
        .type = ASTARTE_MAPPING_TYPE_DOUBLE,
        .reliability = ASTARTE_MAPPING_RELIABILITY_UNRELIABLE,
        .explicit_timestamp = true,
        .allow_unset = false,
    },
    {
        .endpoint = "/%{id}/altitude",
        .type = ASTARTE_MAPPING_TYPE_DOUBLE,
        .reliability = ASTARTE_MAPPING_RELIABILITY_UNRELIABLE,
        .explicit_timestamp = true,
        .allow_unset = false,
    },
    {
        .endpoint = "/%{id}/accuracy",
        .type = ASTARTE_MAPPING_TYPE_DOUBLE,
        .reliability = ASTARTE_MAPPING_RELIABILITY_UNRELIABLE,
        .explicit_timestamp = true,
        .allow_unset = false,
    },
    {
        .endpoint = "/%{id}/altitudeAccuracy",
        .type = ASTARTE_MAPPING_TYPE_DOUBLE,
        .reliability = ASTARTE_MAPPING_RELIABILITY_UNRELIABLE,
        .explicit_timestamp = true,
        .allow_unset = false,
    },
    {
        .endpoint = "/%{id}/heading",
        .type = ASTARTE_MAPPING_TYPE_DOUBLE,
        .reliability = ASTARTE_MAPPING_RELIABILITY_UNRELIABLE,
        .explicit_timestamp = true,
        .allow_unset = false,
    },
    {
        .endpoint = "/%{id}/speed",
        .type = ASTARTE_MAPPING_TYPE_DOUBLE,
        .reliability = ASTARTE_MAPPING_RELIABILITY_UNRELIABLE,
        .explicit_timestamp = true,
        .allow_unset = false,
    },
};

/** @brief Automatically generated interface definition. */
const astarte_interface_t io_edgehog_devicemanager_Geolocation = {
    .name = "io.edgehog.devicemanager.Geolocation",
    .major_version = 0,
    .minor_version = 1,
    .type = ASTARTE_INTERFACE_TYPE_DATASTREAM,
    .ownership = ASTARTE_INTERFACE_OWNERSHIP_DEVICE,
    .aggregation = ASTARTE_INTERFACE_AGGREGATION_OBJECT,
    .mappings = io_edgehog_devicemanager_Geolocation_mappings,
    .mappings_length = 7U,
};

/** @brief Automatically generated mapping definition. */
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

/** @brief Automatically generated interface definition. */
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

/** @brief Automatically generated mapping definition. */
static const astarte_mapping_t io_edgehog_devicemanager_LedBehavior_mappings[1] = {

    {
        .endpoint = "/%{led_id}/behavior",
        .type = ASTARTE_MAPPING_TYPE_STRING,
        .reliability = ASTARTE_MAPPING_RELIABILITY_UNIQUE,
        .explicit_timestamp = false,
        .allow_unset = false,
    },
};

/** @brief Automatically generated interface definition. */
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

/** @brief Automatically generated mapping definition. */
static const astarte_mapping_t io_edgehog_devicemanager_NetworkInterfaceProperties_mappings[2] = {

    {
        .endpoint = "/%{iface_name}/macAddress",
        .type = ASTARTE_MAPPING_TYPE_STRING,
        .reliability = ASTARTE_MAPPING_RELIABILITY_UNIQUE,
        .explicit_timestamp = false,
        .allow_unset = true,
    },
    {
        .endpoint = "/%{iface_name}/technologyType",
        .type = ASTARTE_MAPPING_TYPE_STRING,
        .reliability = ASTARTE_MAPPING_RELIABILITY_UNIQUE,
        .explicit_timestamp = false,
        .allow_unset = true,
    },
};

/** @brief Automatically generated interface definition. */
const astarte_interface_t io_edgehog_devicemanager_NetworkInterfaceProperties = {
    .name = "io.edgehog.devicemanager.NetworkInterfaceProperties",
    .major_version = 0,
    .minor_version = 1,
    .type = ASTARTE_INTERFACE_TYPE_PROPERTIES,
    .ownership = ASTARTE_INTERFACE_OWNERSHIP_DEVICE,
    .aggregation = ASTARTE_INTERFACE_AGGREGATION_INDIVIDUAL,
    .mappings = io_edgehog_devicemanager_NetworkInterfaceProperties_mappings,
    .mappings_length = 2U,
};

/** @brief Automatically generated mapping definition. */
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

/** @brief Automatically generated interface definition. */
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

/** @brief Automatically generated mapping definition. */
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

/** @brief Automatically generated interface definition. */
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

/** @brief Automatically generated mapping definition. */
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

/** @brief Automatically generated interface definition. */
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

/** @brief Automatically generated mapping definition. */
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

/** @brief Automatically generated interface definition. */
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

/** @brief Automatically generated mapping definition. */
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

/** @brief Automatically generated interface definition. */
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

/** @brief Automatically generated mapping definition. */
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

/** @brief Automatically generated interface definition. */
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

/** @brief Automatically generated mapping definition. */
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

/** @brief Automatically generated interface definition. */
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

/** @brief Automatically generated mapping definition. */
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

/** @brief Automatically generated interface definition. */
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

/** @brief Automatically generated mapping definition. */
static const astarte_mapping_t io_edgehog_devicemanager_apps_AvailableContainers_mappings[1] = {

    {
        .endpoint = "/%{container_id}/status",
        .type = ASTARTE_MAPPING_TYPE_STRING,
        .reliability = ASTARTE_MAPPING_RELIABILITY_UNIQUE,
        .explicit_timestamp = false,
        .allow_unset = true,
    },
};

/** @brief Automatically generated interface definition. */
const astarte_interface_t io_edgehog_devicemanager_apps_AvailableContainers = {
    .name = "io.edgehog.devicemanager.apps.AvailableContainers",
    .major_version = 0,
    .minor_version = 1,
    .type = ASTARTE_INTERFACE_TYPE_PROPERTIES,
    .ownership = ASTARTE_INTERFACE_OWNERSHIP_DEVICE,
    .aggregation = ASTARTE_INTERFACE_AGGREGATION_INDIVIDUAL,
    .mappings = io_edgehog_devicemanager_apps_AvailableContainers_mappings,
    .mappings_length = 1U,
};

/** @brief Automatically generated mapping definition. */
static const astarte_mapping_t io_edgehog_devicemanager_apps_AvailableDeployments_mappings[1] = {

    {
        .endpoint = "/%{deployment_id}/status",
        .type = ASTARTE_MAPPING_TYPE_STRING,
        .reliability = ASTARTE_MAPPING_RELIABILITY_UNIQUE,
        .explicit_timestamp = false,
        .allow_unset = true,
    },
};

/** @brief Automatically generated interface definition. */
const astarte_interface_t io_edgehog_devicemanager_apps_AvailableDeployments = {
    .name = "io.edgehog.devicemanager.apps.AvailableDeployments",
    .major_version = 0,
    .minor_version = 1,
    .type = ASTARTE_INTERFACE_TYPE_PROPERTIES,
    .ownership = ASTARTE_INTERFACE_OWNERSHIP_DEVICE,
    .aggregation = ASTARTE_INTERFACE_AGGREGATION_INDIVIDUAL,
    .mappings = io_edgehog_devicemanager_apps_AvailableDeployments_mappings,
    .mappings_length = 1U,
};

/** @brief Automatically generated mapping definition. */
static const astarte_mapping_t io_edgehog_devicemanager_apps_AvailableDeviceMappings_mappings[1] = {

    {
        .endpoint = "/%{device_mapping_id}/present",
        .type = ASTARTE_MAPPING_TYPE_BOOLEAN,
        .reliability = ASTARTE_MAPPING_RELIABILITY_UNIQUE,
        .explicit_timestamp = false,
        .allow_unset = true,
    },
};

/** @brief Automatically generated interface definition. */
const astarte_interface_t io_edgehog_devicemanager_apps_AvailableDeviceMappings = {
    .name = "io.edgehog.devicemanager.apps.AvailableDeviceMappings",
    .major_version = 0,
    .minor_version = 1,
    .type = ASTARTE_INTERFACE_TYPE_PROPERTIES,
    .ownership = ASTARTE_INTERFACE_OWNERSHIP_DEVICE,
    .aggregation = ASTARTE_INTERFACE_AGGREGATION_INDIVIDUAL,
    .mappings = io_edgehog_devicemanager_apps_AvailableDeviceMappings_mappings,
    .mappings_length = 1U,
};

/** @brief Automatically generated mapping definition. */
static const astarte_mapping_t io_edgehog_devicemanager_apps_AvailableImages_mappings[1] = {

    {
        .endpoint = "/%{image_id}/pulled",
        .type = ASTARTE_MAPPING_TYPE_BOOLEAN,
        .reliability = ASTARTE_MAPPING_RELIABILITY_UNIQUE,
        .explicit_timestamp = false,
        .allow_unset = true,
    },
};

/** @brief Automatically generated interface definition. */
const astarte_interface_t io_edgehog_devicemanager_apps_AvailableImages = {
    .name = "io.edgehog.devicemanager.apps.AvailableImages",
    .major_version = 0,
    .minor_version = 1,
    .type = ASTARTE_INTERFACE_TYPE_PROPERTIES,
    .ownership = ASTARTE_INTERFACE_OWNERSHIP_DEVICE,
    .aggregation = ASTARTE_INTERFACE_AGGREGATION_INDIVIDUAL,
    .mappings = io_edgehog_devicemanager_apps_AvailableImages_mappings,
    .mappings_length = 1U,
};

/** @brief Automatically generated mapping definition. */
static const astarte_mapping_t io_edgehog_devicemanager_apps_AvailableNetworks_mappings[1] = {

    {
        .endpoint = "/%{network_id}/created",
        .type = ASTARTE_MAPPING_TYPE_BOOLEAN,
        .reliability = ASTARTE_MAPPING_RELIABILITY_UNIQUE,
        .explicit_timestamp = false,
        .allow_unset = true,
    },
};

/** @brief Automatically generated interface definition. */
const astarte_interface_t io_edgehog_devicemanager_apps_AvailableNetworks = {
    .name = "io.edgehog.devicemanager.apps.AvailableNetworks",
    .major_version = 0,
    .minor_version = 1,
    .type = ASTARTE_INTERFACE_TYPE_PROPERTIES,
    .ownership = ASTARTE_INTERFACE_OWNERSHIP_DEVICE,
    .aggregation = ASTARTE_INTERFACE_AGGREGATION_INDIVIDUAL,
    .mappings = io_edgehog_devicemanager_apps_AvailableNetworks_mappings,
    .mappings_length = 1U,
};

/** @brief Automatically generated mapping definition. */
static const astarte_mapping_t io_edgehog_devicemanager_apps_AvailableVolumes_mappings[1] = {

    {
        .endpoint = "/%{volume_id}/created",
        .type = ASTARTE_MAPPING_TYPE_BOOLEAN,
        .reliability = ASTARTE_MAPPING_RELIABILITY_UNIQUE,
        .explicit_timestamp = false,
        .allow_unset = true,
    },
};

/** @brief Automatically generated interface definition. */
const astarte_interface_t io_edgehog_devicemanager_apps_AvailableVolumes = {
    .name = "io.edgehog.devicemanager.apps.AvailableVolumes",
    .major_version = 0,
    .minor_version = 1,
    .type = ASTARTE_INTERFACE_TYPE_PROPERTIES,
    .ownership = ASTARTE_INTERFACE_OWNERSHIP_DEVICE,
    .aggregation = ASTARTE_INTERFACE_AGGREGATION_INDIVIDUAL,
    .mappings = io_edgehog_devicemanager_apps_AvailableVolumes_mappings,
    .mappings_length = 1U,
};

/** @brief Automatically generated mapping definition. */
static const astarte_mapping_t io_edgehog_devicemanager_apps_CreateContainerRequest_mappings[28] = {

    {
        .endpoint = "/container/id",
        .type = ASTARTE_MAPPING_TYPE_STRING,
        .reliability = ASTARTE_MAPPING_RELIABILITY_GUARANTEED,
        .explicit_timestamp = false,
        .allow_unset = false,
    },
    {
        .endpoint = "/container/deploymentId",
        .type = ASTARTE_MAPPING_TYPE_STRING,
        .reliability = ASTARTE_MAPPING_RELIABILITY_GUARANTEED,
        .explicit_timestamp = false,
        .allow_unset = false,
    },
    {
        .endpoint = "/container/imageId",
        .type = ASTARTE_MAPPING_TYPE_STRING,
        .reliability = ASTARTE_MAPPING_RELIABILITY_GUARANTEED,
        .explicit_timestamp = false,
        .allow_unset = false,
    },
    {
        .endpoint = "/container/networkIds",
        .type = ASTARTE_MAPPING_TYPE_STRINGARRAY,
        .reliability = ASTARTE_MAPPING_RELIABILITY_GUARANTEED,
        .explicit_timestamp = false,
        .allow_unset = false,
    },
    {
        .endpoint = "/container/volumeIds",
        .type = ASTARTE_MAPPING_TYPE_STRINGARRAY,
        .reliability = ASTARTE_MAPPING_RELIABILITY_GUARANTEED,
        .explicit_timestamp = false,
        .allow_unset = false,
    },
    {
        .endpoint = "/container/deviceMappingIds",
        .type = ASTARTE_MAPPING_TYPE_STRINGARRAY,
        .reliability = ASTARTE_MAPPING_RELIABILITY_GUARANTEED,
        .explicit_timestamp = false,
        .allow_unset = false,
    },
    {
        .endpoint = "/container/hostname",
        .type = ASTARTE_MAPPING_TYPE_STRING,
        .reliability = ASTARTE_MAPPING_RELIABILITY_GUARANTEED,
        .explicit_timestamp = false,
        .allow_unset = false,
    },
    {
        .endpoint = "/container/restartPolicy",
        .type = ASTARTE_MAPPING_TYPE_STRING,
        .reliability = ASTARTE_MAPPING_RELIABILITY_GUARANTEED,
        .explicit_timestamp = false,
        .allow_unset = false,
    },
    {
        .endpoint = "/container/env",
        .type = ASTARTE_MAPPING_TYPE_STRINGARRAY,
        .reliability = ASTARTE_MAPPING_RELIABILITY_GUARANTEED,
        .explicit_timestamp = false,
        .allow_unset = false,
    },
    {
        .endpoint = "/container/binds",
        .type = ASTARTE_MAPPING_TYPE_STRINGARRAY,
        .reliability = ASTARTE_MAPPING_RELIABILITY_GUARANTEED,
        .explicit_timestamp = false,
        .allow_unset = false,
    },
    {
        .endpoint = "/container/networkMode",
        .type = ASTARTE_MAPPING_TYPE_STRING,
        .reliability = ASTARTE_MAPPING_RELIABILITY_GUARANTEED,
        .explicit_timestamp = false,
        .allow_unset = false,
    },
    {
        .endpoint = "/container/portBindings",
        .type = ASTARTE_MAPPING_TYPE_STRINGARRAY,
        .reliability = ASTARTE_MAPPING_RELIABILITY_GUARANTEED,
        .explicit_timestamp = false,
        .allow_unset = false,
    },
    {
        .endpoint = "/container/extraHosts",
        .type = ASTARTE_MAPPING_TYPE_STRINGARRAY,
        .reliability = ASTARTE_MAPPING_RELIABILITY_GUARANTEED,
        .explicit_timestamp = false,
        .allow_unset = false,
    },
    {
        .endpoint = "/container/capAdd",
        .type = ASTARTE_MAPPING_TYPE_STRINGARRAY,
        .reliability = ASTARTE_MAPPING_RELIABILITY_GUARANTEED,
        .explicit_timestamp = false,
        .allow_unset = false,
    },
    {
        .endpoint = "/container/capDrop",
        .type = ASTARTE_MAPPING_TYPE_STRINGARRAY,
        .reliability = ASTARTE_MAPPING_RELIABILITY_GUARANTEED,
        .explicit_timestamp = false,
        .allow_unset = false,
    },
    {
        .endpoint = "/container/cpuPeriod",
        .type = ASTARTE_MAPPING_TYPE_LONGINTEGER,
        .reliability = ASTARTE_MAPPING_RELIABILITY_GUARANTEED,
        .explicit_timestamp = false,
        .allow_unset = false,
    },
    {
        .endpoint = "/container/cpuQuota",
        .type = ASTARTE_MAPPING_TYPE_LONGINTEGER,
        .reliability = ASTARTE_MAPPING_RELIABILITY_GUARANTEED,
        .explicit_timestamp = false,
        .allow_unset = false,
    },
    {
        .endpoint = "/container/cpuRealtimePeriod",
        .type = ASTARTE_MAPPING_TYPE_LONGINTEGER,
        .reliability = ASTARTE_MAPPING_RELIABILITY_GUARANTEED,
        .explicit_timestamp = false,
        .allow_unset = false,
    },
    {
        .endpoint = "/container/cpuRealtimeRuntime",
        .type = ASTARTE_MAPPING_TYPE_LONGINTEGER,
        .reliability = ASTARTE_MAPPING_RELIABILITY_GUARANTEED,
        .explicit_timestamp = false,
        .allow_unset = false,
    },
    {
        .endpoint = "/container/memory",
        .type = ASTARTE_MAPPING_TYPE_LONGINTEGER,
        .reliability = ASTARTE_MAPPING_RELIABILITY_GUARANTEED,
        .explicit_timestamp = false,
        .allow_unset = false,
    },
    {
        .endpoint = "/container/memoryReservation",
        .type = ASTARTE_MAPPING_TYPE_LONGINTEGER,
        .reliability = ASTARTE_MAPPING_RELIABILITY_GUARANTEED,
        .explicit_timestamp = false,
        .allow_unset = false,
    },
    {
        .endpoint = "/container/memorySwap",
        .type = ASTARTE_MAPPING_TYPE_LONGINTEGER,
        .reliability = ASTARTE_MAPPING_RELIABILITY_GUARANTEED,
        .explicit_timestamp = false,
        .allow_unset = false,
    },
    {
        .endpoint = "/container/memorySwappiness",
        .type = ASTARTE_MAPPING_TYPE_INTEGER,
        .reliability = ASTARTE_MAPPING_RELIABILITY_GUARANTEED,
        .explicit_timestamp = false,
        .allow_unset = false,
    },
    {
        .endpoint = "/container/volumeDriver",
        .type = ASTARTE_MAPPING_TYPE_STRING,
        .reliability = ASTARTE_MAPPING_RELIABILITY_GUARANTEED,
        .explicit_timestamp = false,
        .allow_unset = false,
    },
    {
        .endpoint = "/container/storageOpt",
        .type = ASTARTE_MAPPING_TYPE_STRINGARRAY,
        .reliability = ASTARTE_MAPPING_RELIABILITY_GUARANTEED,
        .explicit_timestamp = false,
        .allow_unset = false,
    },
    {
        .endpoint = "/container/readOnlyRootfs",
        .type = ASTARTE_MAPPING_TYPE_BOOLEAN,
        .reliability = ASTARTE_MAPPING_RELIABILITY_GUARANTEED,
        .explicit_timestamp = false,
        .allow_unset = false,
    },
    {
        .endpoint = "/container/tmpfs",
        .type = ASTARTE_MAPPING_TYPE_STRINGARRAY,
        .reliability = ASTARTE_MAPPING_RELIABILITY_GUARANTEED,
        .explicit_timestamp = false,
        .allow_unset = false,
    },
    {
        .endpoint = "/container/privileged",
        .type = ASTARTE_MAPPING_TYPE_BOOLEAN,
        .reliability = ASTARTE_MAPPING_RELIABILITY_GUARANTEED,
        .explicit_timestamp = false,
        .allow_unset = false,
    },
};

/** @brief Automatically generated interface definition. */
const astarte_interface_t io_edgehog_devicemanager_apps_CreateContainerRequest = {
    .name = "io.edgehog.devicemanager.apps.CreateContainerRequest",
    .major_version = 0,
    .minor_version = 1,
    .type = ASTARTE_INTERFACE_TYPE_DATASTREAM,
    .ownership = ASTARTE_INTERFACE_OWNERSHIP_SERVER,
    .aggregation = ASTARTE_INTERFACE_AGGREGATION_OBJECT,
    .mappings = io_edgehog_devicemanager_apps_CreateContainerRequest_mappings,
    .mappings_length = 28U,
};

/** @brief Automatically generated mapping definition. */
static const astarte_mapping_t io_edgehog_devicemanager_apps_CreateDeploymentRequest_mappings[2] = {

    {
        .endpoint = "/deployment/id",
        .type = ASTARTE_MAPPING_TYPE_STRING,
        .reliability = ASTARTE_MAPPING_RELIABILITY_GUARANTEED,
        .explicit_timestamp = false,
        .allow_unset = false,
    },
    {
        .endpoint = "/deployment/containers",
        .type = ASTARTE_MAPPING_TYPE_STRINGARRAY,
        .reliability = ASTARTE_MAPPING_RELIABILITY_GUARANTEED,
        .explicit_timestamp = false,
        .allow_unset = false,
    },
};

/** @brief Automatically generated interface definition. */
const astarte_interface_t io_edgehog_devicemanager_apps_CreateDeploymentRequest = {
    .name = "io.edgehog.devicemanager.apps.CreateDeploymentRequest",
    .major_version = 0,
    .minor_version = 1,
    .type = ASTARTE_INTERFACE_TYPE_DATASTREAM,
    .ownership = ASTARTE_INTERFACE_OWNERSHIP_SERVER,
    .aggregation = ASTARTE_INTERFACE_AGGREGATION_OBJECT,
    .mappings = io_edgehog_devicemanager_apps_CreateDeploymentRequest_mappings,
    .mappings_length = 2U,
};

/** @brief Automatically generated mapping definition. */
static const astarte_mapping_t io_edgehog_devicemanager_apps_CreateDeviceMappingRequest_mappings[5]
    = {

          {
              .endpoint = "/deviceMapping/id",
              .type = ASTARTE_MAPPING_TYPE_STRING,
              .reliability = ASTARTE_MAPPING_RELIABILITY_GUARANTEED,
              .explicit_timestamp = false,
              .allow_unset = false,
          },
          {
              .endpoint = "/deviceMapping/deploymentId",
              .type = ASTARTE_MAPPING_TYPE_STRING,
              .reliability = ASTARTE_MAPPING_RELIABILITY_GUARANTEED,
              .explicit_timestamp = false,
              .allow_unset = false,
          },
          {
              .endpoint = "/deviceMapping/pathOnHost",
              .type = ASTARTE_MAPPING_TYPE_STRING,
              .reliability = ASTARTE_MAPPING_RELIABILITY_GUARANTEED,
              .explicit_timestamp = false,
              .allow_unset = false,
          },
          {
              .endpoint = "/deviceMapping/pathInContainer",
              .type = ASTARTE_MAPPING_TYPE_STRING,
              .reliability = ASTARTE_MAPPING_RELIABILITY_GUARANTEED,
              .explicit_timestamp = false,
              .allow_unset = false,
          },
          {
              .endpoint = "/deviceMapping/cGroupPermissions",
              .type = ASTARTE_MAPPING_TYPE_STRING,
              .reliability = ASTARTE_MAPPING_RELIABILITY_GUARANTEED,
              .explicit_timestamp = false,
              .allow_unset = false,
          },
      };

/** @brief Automatically generated interface definition. */
const astarte_interface_t io_edgehog_devicemanager_apps_CreateDeviceMappingRequest = {
    .name = "io.edgehog.devicemanager.apps.CreateDeviceMappingRequest",
    .major_version = 0,
    .minor_version = 1,
    .type = ASTARTE_INTERFACE_TYPE_DATASTREAM,
    .ownership = ASTARTE_INTERFACE_OWNERSHIP_SERVER,
    .aggregation = ASTARTE_INTERFACE_AGGREGATION_OBJECT,
    .mappings = io_edgehog_devicemanager_apps_CreateDeviceMappingRequest_mappings,
    .mappings_length = 5U,
};

/** @brief Automatically generated mapping definition. */
static const astarte_mapping_t io_edgehog_devicemanager_apps_CreateImageRequest_mappings[4] = {

    {
        .endpoint = "/image/id",
        .type = ASTARTE_MAPPING_TYPE_STRING,
        .reliability = ASTARTE_MAPPING_RELIABILITY_GUARANTEED,
        .explicit_timestamp = false,
        .allow_unset = false,
    },
    {
        .endpoint = "/image/deploymentId",
        .type = ASTARTE_MAPPING_TYPE_STRING,
        .reliability = ASTARTE_MAPPING_RELIABILITY_GUARANTEED,
        .explicit_timestamp = false,
        .allow_unset = false,
    },
    {
        .endpoint = "/image/reference",
        .type = ASTARTE_MAPPING_TYPE_STRING,
        .reliability = ASTARTE_MAPPING_RELIABILITY_GUARANTEED,
        .explicit_timestamp = false,
        .allow_unset = false,
    },
    {
        .endpoint = "/image/registryAuth",
        .type = ASTARTE_MAPPING_TYPE_STRING,
        .reliability = ASTARTE_MAPPING_RELIABILITY_GUARANTEED,
        .explicit_timestamp = false,
        .allow_unset = false,
    },
};

/** @brief Automatically generated interface definition. */
const astarte_interface_t io_edgehog_devicemanager_apps_CreateImageRequest = {
    .name = "io.edgehog.devicemanager.apps.CreateImageRequest",
    .major_version = 0,
    .minor_version = 1,
    .type = ASTARTE_INTERFACE_TYPE_DATASTREAM,
    .ownership = ASTARTE_INTERFACE_OWNERSHIP_SERVER,
    .aggregation = ASTARTE_INTERFACE_AGGREGATION_OBJECT,
    .mappings = io_edgehog_devicemanager_apps_CreateImageRequest_mappings,
    .mappings_length = 4U,
};

/** @brief Automatically generated mapping definition. */
static const astarte_mapping_t io_edgehog_devicemanager_apps_CreateNetworkRequest_mappings[6] = {

    {
        .endpoint = "/network/id",
        .type = ASTARTE_MAPPING_TYPE_STRING,
        .reliability = ASTARTE_MAPPING_RELIABILITY_GUARANTEED,
        .explicit_timestamp = false,
        .allow_unset = false,
    },
    {
        .endpoint = "/network/deploymentId",
        .type = ASTARTE_MAPPING_TYPE_STRING,
        .reliability = ASTARTE_MAPPING_RELIABILITY_GUARANTEED,
        .explicit_timestamp = false,
        .allow_unset = false,
    },
    {
        .endpoint = "/network/driver",
        .type = ASTARTE_MAPPING_TYPE_STRING,
        .reliability = ASTARTE_MAPPING_RELIABILITY_GUARANTEED,
        .explicit_timestamp = false,
        .allow_unset = false,
    },
    {
        .endpoint = "/network/internal",
        .type = ASTARTE_MAPPING_TYPE_BOOLEAN,
        .reliability = ASTARTE_MAPPING_RELIABILITY_GUARANTEED,
        .explicit_timestamp = false,
        .allow_unset = false,
    },
    {
        .endpoint = "/network/enableIpv6",
        .type = ASTARTE_MAPPING_TYPE_BOOLEAN,
        .reliability = ASTARTE_MAPPING_RELIABILITY_GUARANTEED,
        .explicit_timestamp = false,
        .allow_unset = false,
    },
    {
        .endpoint = "/network/options",
        .type = ASTARTE_MAPPING_TYPE_STRINGARRAY,
        .reliability = ASTARTE_MAPPING_RELIABILITY_GUARANTEED,
        .explicit_timestamp = false,
        .allow_unset = false,
    },
};

/** @brief Automatically generated interface definition. */
const astarte_interface_t io_edgehog_devicemanager_apps_CreateNetworkRequest = {
    .name = "io.edgehog.devicemanager.apps.CreateNetworkRequest",
    .major_version = 0,
    .minor_version = 1,
    .type = ASTARTE_INTERFACE_TYPE_DATASTREAM,
    .ownership = ASTARTE_INTERFACE_OWNERSHIP_SERVER,
    .aggregation = ASTARTE_INTERFACE_AGGREGATION_OBJECT,
    .mappings = io_edgehog_devicemanager_apps_CreateNetworkRequest_mappings,
    .mappings_length = 6U,
};

/** @brief Automatically generated mapping definition. */
static const astarte_mapping_t io_edgehog_devicemanager_apps_CreateVolumeRequest_mappings[4] = {

    {
        .endpoint = "/volume/id",
        .type = ASTARTE_MAPPING_TYPE_STRING,
        .reliability = ASTARTE_MAPPING_RELIABILITY_GUARANTEED,
        .explicit_timestamp = false,
        .allow_unset = false,
    },
    {
        .endpoint = "/volume/deploymentId",
        .type = ASTARTE_MAPPING_TYPE_STRING,
        .reliability = ASTARTE_MAPPING_RELIABILITY_GUARANTEED,
        .explicit_timestamp = false,
        .allow_unset = false,
    },
    {
        .endpoint = "/volume/driver",
        .type = ASTARTE_MAPPING_TYPE_STRING,
        .reliability = ASTARTE_MAPPING_RELIABILITY_GUARANTEED,
        .explicit_timestamp = false,
        .allow_unset = false,
    },
    {
        .endpoint = "/volume/options",
        .type = ASTARTE_MAPPING_TYPE_STRINGARRAY,
        .reliability = ASTARTE_MAPPING_RELIABILITY_GUARANTEED,
        .explicit_timestamp = false,
        .allow_unset = false,
    },
};

/** @brief Automatically generated interface definition. */
const astarte_interface_t io_edgehog_devicemanager_apps_CreateVolumeRequest = {
    .name = "io.edgehog.devicemanager.apps.CreateVolumeRequest",
    .major_version = 0,
    .minor_version = 1,
    .type = ASTARTE_INTERFACE_TYPE_DATASTREAM,
    .ownership = ASTARTE_INTERFACE_OWNERSHIP_SERVER,
    .aggregation = ASTARTE_INTERFACE_AGGREGATION_OBJECT,
    .mappings = io_edgehog_devicemanager_apps_CreateVolumeRequest_mappings,
    .mappings_length = 4U,
};

/** @brief Automatically generated mapping definition. */
static const astarte_mapping_t io_edgehog_devicemanager_apps_DeploymentCommand_mappings[1] = {

    {
        .endpoint = "/%{deployment_id}/command",
        .type = ASTARTE_MAPPING_TYPE_STRING,
        .reliability = ASTARTE_MAPPING_RELIABILITY_UNRELIABLE,
        .explicit_timestamp = false,
        .allow_unset = false,
    },
};

/** @brief Automatically generated interface definition. */
const astarte_interface_t io_edgehog_devicemanager_apps_DeploymentCommand = {
    .name = "io.edgehog.devicemanager.apps.DeploymentCommand",
    .major_version = 0,
    .minor_version = 1,
    .type = ASTARTE_INTERFACE_TYPE_DATASTREAM,
    .ownership = ASTARTE_INTERFACE_OWNERSHIP_SERVER,
    .aggregation = ASTARTE_INTERFACE_AGGREGATION_INDIVIDUAL,
    .mappings = io_edgehog_devicemanager_apps_DeploymentCommand_mappings,
    .mappings_length = 1U,
};

/** @brief Automatically generated mapping definition. */
static const astarte_mapping_t io_edgehog_devicemanager_apps_DeploymentEvent_mappings[3] = {

    {
        .endpoint = "/%{deployment_id}/status",
        .type = ASTARTE_MAPPING_TYPE_STRING,
        .reliability = ASTARTE_MAPPING_RELIABILITY_UNRELIABLE,
        .explicit_timestamp = true,
        .allow_unset = false,
    },
    {
        .endpoint = "/%{deployment_id}/message",
        .type = ASTARTE_MAPPING_TYPE_STRING,
        .reliability = ASTARTE_MAPPING_RELIABILITY_UNRELIABLE,
        .explicit_timestamp = true,
        .allow_unset = false,
    },
    {
        .endpoint = "/%{deployment_id}/addInfo",
        .type = ASTARTE_MAPPING_TYPE_STRINGARRAY,
        .reliability = ASTARTE_MAPPING_RELIABILITY_UNRELIABLE,
        .explicit_timestamp = true,
        .allow_unset = false,
    },
};

/** @brief Automatically generated interface definition. */
const astarte_interface_t io_edgehog_devicemanager_apps_DeploymentEvent = {
    .name = "io.edgehog.devicemanager.apps.DeploymentEvent",
    .major_version = 0,
    .minor_version = 2,
    .type = ASTARTE_INTERFACE_TYPE_DATASTREAM,
    .ownership = ASTARTE_INTERFACE_OWNERSHIP_DEVICE,
    .aggregation = ASTARTE_INTERFACE_AGGREGATION_OBJECT,
    .mappings = io_edgehog_devicemanager_apps_DeploymentEvent_mappings,
    .mappings_length = 3U,
};

/** @brief Automatically generated mapping definition. */
static const astarte_mapping_t io_edgehog_devicemanager_apps_DeploymentUpdate_mappings[2] = {

    {
        .endpoint = "/deployment/from",
        .type = ASTARTE_MAPPING_TYPE_STRING,
        .reliability = ASTARTE_MAPPING_RELIABILITY_UNRELIABLE,
        .explicit_timestamp = false,
        .allow_unset = false,
    },
    {
        .endpoint = "/deployment/to",
        .type = ASTARTE_MAPPING_TYPE_STRING,
        .reliability = ASTARTE_MAPPING_RELIABILITY_UNRELIABLE,
        .explicit_timestamp = false,
        .allow_unset = false,
    },
};

/** @brief Automatically generated interface definition. */
const astarte_interface_t io_edgehog_devicemanager_apps_DeploymentUpdate = {
    .name = "io.edgehog.devicemanager.apps.DeploymentUpdate",
    .major_version = 0,
    .minor_version = 1,
    .type = ASTARTE_INTERFACE_TYPE_DATASTREAM,
    .ownership = ASTARTE_INTERFACE_OWNERSHIP_SERVER,
    .aggregation = ASTARTE_INTERFACE_AGGREGATION_OBJECT,
    .mappings = io_edgehog_devicemanager_apps_DeploymentUpdate_mappings,
    .mappings_length = 2U,
};

/** @brief Automatically generated mapping definition. */
static const astarte_mapping_t io_edgehog_devicemanager_apps_stats_ContainerBlkio_mappings[5] = {

    {
        .endpoint = "/%{container_id}/name",
        .type = ASTARTE_MAPPING_TYPE_STRING,
        .reliability = ASTARTE_MAPPING_RELIABILITY_UNRELIABLE,
        .explicit_timestamp = true,
        .allow_unset = false,
    },
    {
        .endpoint = "/%{container_id}/major",
        .type = ASTARTE_MAPPING_TYPE_LONGINTEGER,
        .reliability = ASTARTE_MAPPING_RELIABILITY_UNRELIABLE,
        .explicit_timestamp = true,
        .allow_unset = false,
    },
    {
        .endpoint = "/%{container_id}/minor",
        .type = ASTARTE_MAPPING_TYPE_LONGINTEGER,
        .reliability = ASTARTE_MAPPING_RELIABILITY_UNRELIABLE,
        .explicit_timestamp = true,
        .allow_unset = false,
    },
    {
        .endpoint = "/%{container_id}/op",
        .type = ASTARTE_MAPPING_TYPE_STRING,
        .reliability = ASTARTE_MAPPING_RELIABILITY_UNRELIABLE,
        .explicit_timestamp = true,
        .allow_unset = false,
    },
    {
        .endpoint = "/%{container_id}/value",
        .type = ASTARTE_MAPPING_TYPE_LONGINTEGER,
        .reliability = ASTARTE_MAPPING_RELIABILITY_UNRELIABLE,
        .explicit_timestamp = true,
        .allow_unset = false,
    },
};

/** @brief Automatically generated interface definition. */
const astarte_interface_t io_edgehog_devicemanager_apps_stats_ContainerBlkio = {
    .name = "io.edgehog.devicemanager.apps.stats.ContainerBlkio",
    .major_version = 0,
    .minor_version = 1,
    .type = ASTARTE_INTERFACE_TYPE_DATASTREAM,
    .ownership = ASTARTE_INTERFACE_OWNERSHIP_DEVICE,
    .aggregation = ASTARTE_INTERFACE_AGGREGATION_OBJECT,
    .mappings = io_edgehog_devicemanager_apps_stats_ContainerBlkio_mappings,
    .mappings_length = 5U,
};

/** @brief Automatically generated mapping definition. */
static const astarte_mapping_t io_edgehog_devicemanager_apps_stats_ContainerCpu_mappings[18] = {

    {
        .endpoint = "/%{container_id}/cpuUsageTotalUsage",
        .type = ASTARTE_MAPPING_TYPE_LONGINTEGER,
        .reliability = ASTARTE_MAPPING_RELIABILITY_UNRELIABLE,
        .explicit_timestamp = true,
        .allow_unset = false,
    },
    {
        .endpoint = "/%{container_id}/cpuUsagePercpuUsage",
        .type = ASTARTE_MAPPING_TYPE_LONGINTEGERARRAY,
        .reliability = ASTARTE_MAPPING_RELIABILITY_UNRELIABLE,
        .explicit_timestamp = true,
        .allow_unset = false,
    },
    {
        .endpoint = "/%{container_id}/cpuUsageUsageInKernelmode",
        .type = ASTARTE_MAPPING_TYPE_LONGINTEGER,
        .reliability = ASTARTE_MAPPING_RELIABILITY_UNRELIABLE,
        .explicit_timestamp = true,
        .allow_unset = false,
    },
    {
        .endpoint = "/%{container_id}/cpuUsageUsageInUsermode",
        .type = ASTARTE_MAPPING_TYPE_LONGINTEGER,
        .reliability = ASTARTE_MAPPING_RELIABILITY_UNRELIABLE,
        .explicit_timestamp = true,
        .allow_unset = false,
    },
    {
        .endpoint = "/%{container_id}/systemCpuUsage",
        .type = ASTARTE_MAPPING_TYPE_LONGINTEGER,
        .reliability = ASTARTE_MAPPING_RELIABILITY_UNRELIABLE,
        .explicit_timestamp = true,
        .allow_unset = false,
    },
    {
        .endpoint = "/%{container_id}/onlineCpus",
        .type = ASTARTE_MAPPING_TYPE_INTEGER,
        .reliability = ASTARTE_MAPPING_RELIABILITY_UNRELIABLE,
        .explicit_timestamp = true,
        .allow_unset = false,
    },
    {
        .endpoint = "/%{container_id}/throttlingDataPeriods",
        .type = ASTARTE_MAPPING_TYPE_LONGINTEGER,
        .reliability = ASTARTE_MAPPING_RELIABILITY_UNRELIABLE,
        .explicit_timestamp = true,
        .allow_unset = false,
    },
    {
        .endpoint = "/%{container_id}/throttlingDataThrottledPeriods",
        .type = ASTARTE_MAPPING_TYPE_LONGINTEGER,
        .reliability = ASTARTE_MAPPING_RELIABILITY_UNRELIABLE,
        .explicit_timestamp = true,
        .allow_unset = false,
    },
    {
        .endpoint = "/%{container_id}/throttlingDataThrottledTime",
        .type = ASTARTE_MAPPING_TYPE_LONGINTEGER,
        .reliability = ASTARTE_MAPPING_RELIABILITY_UNRELIABLE,
        .explicit_timestamp = true,
        .allow_unset = false,
    },
    {
        .endpoint = "/%{container_id}/preCpuUsageTotalUsage",
        .type = ASTARTE_MAPPING_TYPE_LONGINTEGER,
        .reliability = ASTARTE_MAPPING_RELIABILITY_UNRELIABLE,
        .explicit_timestamp = true,
        .allow_unset = false,
    },
    {
        .endpoint = "/%{container_id}/preCpuUsagePercpuUsage",
        .type = ASTARTE_MAPPING_TYPE_LONGINTEGERARRAY,
        .reliability = ASTARTE_MAPPING_RELIABILITY_UNRELIABLE,
        .explicit_timestamp = true,
        .allow_unset = false,
    },
    {
        .endpoint = "/%{container_id}/preCpuUsageUsageInKernelmode",
        .type = ASTARTE_MAPPING_TYPE_LONGINTEGER,
        .reliability = ASTARTE_MAPPING_RELIABILITY_UNRELIABLE,
        .explicit_timestamp = true,
        .allow_unset = false,
    },
    {
        .endpoint = "/%{container_id}/preCpuUsageUsageInUsermode",
        .type = ASTARTE_MAPPING_TYPE_LONGINTEGER,
        .reliability = ASTARTE_MAPPING_RELIABILITY_UNRELIABLE,
        .explicit_timestamp = true,
        .allow_unset = false,
    },
    {
        .endpoint = "/%{container_id}/preSystemCpuUsage",
        .type = ASTARTE_MAPPING_TYPE_LONGINTEGER,
        .reliability = ASTARTE_MAPPING_RELIABILITY_UNRELIABLE,
        .explicit_timestamp = true,
        .allow_unset = false,
    },
    {
        .endpoint = "/%{container_id}/preOnlineCpus",
        .type = ASTARTE_MAPPING_TYPE_INTEGER,
        .reliability = ASTARTE_MAPPING_RELIABILITY_UNRELIABLE,
        .explicit_timestamp = true,
        .allow_unset = false,
    },
    {
        .endpoint = "/%{container_id}/preThrottlingDataPeriods",
        .type = ASTARTE_MAPPING_TYPE_LONGINTEGER,
        .reliability = ASTARTE_MAPPING_RELIABILITY_UNRELIABLE,
        .explicit_timestamp = true,
        .allow_unset = false,
    },
    {
        .endpoint = "/%{container_id}/preThrottlingDataThrottledPeriods",
        .type = ASTARTE_MAPPING_TYPE_LONGINTEGER,
        .reliability = ASTARTE_MAPPING_RELIABILITY_UNRELIABLE,
        .explicit_timestamp = true,
        .allow_unset = false,
    },
    {
        .endpoint = "/%{container_id}/preThrottlingDataThrottledTime",
        .type = ASTARTE_MAPPING_TYPE_LONGINTEGER,
        .reliability = ASTARTE_MAPPING_RELIABILITY_UNRELIABLE,
        .explicit_timestamp = true,
        .allow_unset = false,
    },
};

/** @brief Automatically generated interface definition. */
const astarte_interface_t io_edgehog_devicemanager_apps_stats_ContainerCpu = {
    .name = "io.edgehog.devicemanager.apps.stats.ContainerCpu",
    .major_version = 0,
    .minor_version = 1,
    .type = ASTARTE_INTERFACE_TYPE_DATASTREAM,
    .ownership = ASTARTE_INTERFACE_OWNERSHIP_DEVICE,
    .aggregation = ASTARTE_INTERFACE_AGGREGATION_OBJECT,
    .mappings = io_edgehog_devicemanager_apps_stats_ContainerCpu_mappings,
    .mappings_length = 18U,
};

/** @brief Automatically generated mapping definition. */
static const astarte_mapping_t io_edgehog_devicemanager_apps_stats_ContainerMemory_mappings[4] = {

    {
        .endpoint = "/%{container_id}/usage",
        .type = ASTARTE_MAPPING_TYPE_LONGINTEGER,
        .reliability = ASTARTE_MAPPING_RELIABILITY_UNRELIABLE,
        .explicit_timestamp = true,
        .allow_unset = false,
    },
    {
        .endpoint = "/%{container_id}/maxUsage",
        .type = ASTARTE_MAPPING_TYPE_LONGINTEGER,
        .reliability = ASTARTE_MAPPING_RELIABILITY_UNRELIABLE,
        .explicit_timestamp = true,
        .allow_unset = false,
    },
    {
        .endpoint = "/%{container_id}/failcnt",
        .type = ASTARTE_MAPPING_TYPE_LONGINTEGER,
        .reliability = ASTARTE_MAPPING_RELIABILITY_UNRELIABLE,
        .explicit_timestamp = true,
        .allow_unset = false,
    },
    {
        .endpoint = "/%{container_id}/limit",
        .type = ASTARTE_MAPPING_TYPE_LONGINTEGER,
        .reliability = ASTARTE_MAPPING_RELIABILITY_UNRELIABLE,
        .explicit_timestamp = true,
        .allow_unset = false,
    },
};

/** @brief Automatically generated interface definition. */
const astarte_interface_t io_edgehog_devicemanager_apps_stats_ContainerMemory = {
    .name = "io.edgehog.devicemanager.apps.stats.ContainerMemory",
    .major_version = 0,
    .minor_version = 1,
    .type = ASTARTE_INTERFACE_TYPE_DATASTREAM,
    .ownership = ASTARTE_INTERFACE_OWNERSHIP_DEVICE,
    .aggregation = ASTARTE_INTERFACE_AGGREGATION_OBJECT,
    .mappings = io_edgehog_devicemanager_apps_stats_ContainerMemory_mappings,
    .mappings_length = 4U,
};

/** @brief Automatically generated mapping definition. */
static const astarte_mapping_t io_edgehog_devicemanager_apps_stats_ContainerMemoryStats_mappings[2]
    = {

          {
              .endpoint = "/%{container_id}/name",
              .type = ASTARTE_MAPPING_TYPE_STRING,
              .reliability = ASTARTE_MAPPING_RELIABILITY_UNRELIABLE,
              .explicit_timestamp = true,
              .allow_unset = false,
          },
          {
              .endpoint = "/%{container_id}/value",
              .type = ASTARTE_MAPPING_TYPE_LONGINTEGER,
              .reliability = ASTARTE_MAPPING_RELIABILITY_UNRELIABLE,
              .explicit_timestamp = true,
              .allow_unset = false,
          },
      };

/** @brief Automatically generated interface definition. */
const astarte_interface_t io_edgehog_devicemanager_apps_stats_ContainerMemoryStats = {
    .name = "io.edgehog.devicemanager.apps.stats.ContainerMemoryStats",
    .major_version = 0,
    .minor_version = 1,
    .type = ASTARTE_INTERFACE_TYPE_DATASTREAM,
    .ownership = ASTARTE_INTERFACE_OWNERSHIP_DEVICE,
    .aggregation = ASTARTE_INTERFACE_AGGREGATION_OBJECT,
    .mappings = io_edgehog_devicemanager_apps_stats_ContainerMemoryStats_mappings,
    .mappings_length = 2U,
};

/** @brief Automatically generated mapping definition. */
static const astarte_mapping_t io_edgehog_devicemanager_apps_stats_ContainerNetworks_mappings[9] = {

    {
        .endpoint = "/%{container_id}/interface",
        .type = ASTARTE_MAPPING_TYPE_STRING,
        .reliability = ASTARTE_MAPPING_RELIABILITY_UNRELIABLE,
        .explicit_timestamp = true,
        .allow_unset = false,
    },
    {
        .endpoint = "/%{container_id}/rxBytes",
        .type = ASTARTE_MAPPING_TYPE_LONGINTEGER,
        .reliability = ASTARTE_MAPPING_RELIABILITY_UNRELIABLE,
        .explicit_timestamp = true,
        .allow_unset = false,
    },
    {
        .endpoint = "/%{container_id}/rxPackets",
        .type = ASTARTE_MAPPING_TYPE_LONGINTEGER,
        .reliability = ASTARTE_MAPPING_RELIABILITY_UNRELIABLE,
        .explicit_timestamp = true,
        .allow_unset = false,
    },
    {
        .endpoint = "/%{container_id}/rxDropped",
        .type = ASTARTE_MAPPING_TYPE_LONGINTEGER,
        .reliability = ASTARTE_MAPPING_RELIABILITY_UNRELIABLE,
        .explicit_timestamp = true,
        .allow_unset = false,
    },
    {
        .endpoint = "/%{container_id}/rxErrors",
        .type = ASTARTE_MAPPING_TYPE_LONGINTEGER,
        .reliability = ASTARTE_MAPPING_RELIABILITY_UNRELIABLE,
        .explicit_timestamp = true,
        .allow_unset = false,
    },
    {
        .endpoint = "/%{container_id}/txBytes",
        .type = ASTARTE_MAPPING_TYPE_LONGINTEGER,
        .reliability = ASTARTE_MAPPING_RELIABILITY_UNRELIABLE,
        .explicit_timestamp = true,
        .allow_unset = false,
    },
    {
        .endpoint = "/%{container_id}/txPackets",
        .type = ASTARTE_MAPPING_TYPE_LONGINTEGER,
        .reliability = ASTARTE_MAPPING_RELIABILITY_UNRELIABLE,
        .explicit_timestamp = true,
        .allow_unset = false,
    },
    {
        .endpoint = "/%{container_id}/txErrors",
        .type = ASTARTE_MAPPING_TYPE_LONGINTEGER,
        .reliability = ASTARTE_MAPPING_RELIABILITY_UNRELIABLE,
        .explicit_timestamp = true,
        .allow_unset = false,
    },
    {
        .endpoint = "/%{container_id}/txDropped",
        .type = ASTARTE_MAPPING_TYPE_LONGINTEGER,
        .reliability = ASTARTE_MAPPING_RELIABILITY_UNRELIABLE,
        .explicit_timestamp = true,
        .allow_unset = false,
    },
};

/** @brief Automatically generated interface definition. */
const astarte_interface_t io_edgehog_devicemanager_apps_stats_ContainerNetworks = {
    .name = "io.edgehog.devicemanager.apps.stats.ContainerNetworks",
    .major_version = 0,
    .minor_version = 1,
    .type = ASTARTE_INTERFACE_TYPE_DATASTREAM,
    .ownership = ASTARTE_INTERFACE_OWNERSHIP_DEVICE,
    .aggregation = ASTARTE_INTERFACE_AGGREGATION_OBJECT,
    .mappings = io_edgehog_devicemanager_apps_stats_ContainerNetworks_mappings,
    .mappings_length = 9U,
};

/** @brief Automatically generated mapping definition. */
static const astarte_mapping_t io_edgehog_devicemanager_apps_stats_ContainerProcesses_mappings[2]
    = {

          {
              .endpoint = "/%{container_id}/current",
              .type = ASTARTE_MAPPING_TYPE_LONGINTEGER,
              .reliability = ASTARTE_MAPPING_RELIABILITY_UNRELIABLE,
              .explicit_timestamp = true,
              .allow_unset = false,
          },
          {
              .endpoint = "/%{container_id}/limit",
              .type = ASTARTE_MAPPING_TYPE_LONGINTEGER,
              .reliability = ASTARTE_MAPPING_RELIABILITY_UNRELIABLE,
              .explicit_timestamp = true,
              .allow_unset = false,
          },
      };

/** @brief Automatically generated interface definition. */
const astarte_interface_t io_edgehog_devicemanager_apps_stats_ContainerProcesses = {
    .name = "io.edgehog.devicemanager.apps.stats.ContainerProcesses",
    .major_version = 0,
    .minor_version = 1,
    .type = ASTARTE_INTERFACE_TYPE_DATASTREAM,
    .ownership = ASTARTE_INTERFACE_OWNERSHIP_DEVICE,
    .aggregation = ASTARTE_INTERFACE_AGGREGATION_OBJECT,
    .mappings = io_edgehog_devicemanager_apps_stats_ContainerProcesses_mappings,
    .mappings_length = 2U,
};

/** @brief Automatically generated mapping definition. */
static const astarte_mapping_t io_edgehog_devicemanager_apps_stats_VolumeUsage_mappings[5] = {

    {
        .endpoint = "/%{volume_id}/driver",
        .type = ASTARTE_MAPPING_TYPE_STRING,
        .reliability = ASTARTE_MAPPING_RELIABILITY_UNRELIABLE,
        .explicit_timestamp = true,
        .allow_unset = false,
    },
    {
        .endpoint = "/%{volume_id}/mountpoint",
        .type = ASTARTE_MAPPING_TYPE_STRING,
        .reliability = ASTARTE_MAPPING_RELIABILITY_UNRELIABLE,
        .explicit_timestamp = true,
        .allow_unset = false,
    },
    {
        .endpoint = "/%{volume_id}/createdAt",
        .type = ASTARTE_MAPPING_TYPE_DATETIME,
        .reliability = ASTARTE_MAPPING_RELIABILITY_UNRELIABLE,
        .explicit_timestamp = true,
        .allow_unset = false,
    },
    {
        .endpoint = "/%{volume_id}/usageDataSize",
        .type = ASTARTE_MAPPING_TYPE_LONGINTEGER,
        .reliability = ASTARTE_MAPPING_RELIABILITY_UNRELIABLE,
        .explicit_timestamp = true,
        .allow_unset = false,
    },
    {
        .endpoint = "/%{volume_id}/usageDataRefCount",
        .type = ASTARTE_MAPPING_TYPE_LONGINTEGER,
        .reliability = ASTARTE_MAPPING_RELIABILITY_UNRELIABLE,
        .explicit_timestamp = true,
        .allow_unset = false,
    },
};

/** @brief Automatically generated interface definition. */
const astarte_interface_t io_edgehog_devicemanager_apps_stats_VolumeUsage = {
    .name = "io.edgehog.devicemanager.apps.stats.VolumeUsage",
    .major_version = 0,
    .minor_version = 1,
    .type = ASTARTE_INTERFACE_TYPE_DATASTREAM,
    .ownership = ASTARTE_INTERFACE_OWNERSHIP_DEVICE,
    .aggregation = ASTARTE_INTERFACE_AGGREGATION_OBJECT,
    .mappings = io_edgehog_devicemanager_apps_stats_VolumeUsage_mappings,
    .mappings_length = 5U,
};

/** @brief Automatically generated mapping definition. */
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

/** @brief Automatically generated interface definition. */
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

/** @brief Automatically generated mapping definition. */
static const astarte_mapping_t io_edgehog_devicemanager_fileTransfer_DeviceToServer_mappings[7] = {

    {
        .endpoint = "/request/id",
        .type = ASTARTE_MAPPING_TYPE_STRING,
        .reliability = ASTARTE_MAPPING_RELIABILITY_GUARANTEED,
        .explicit_timestamp = false,
        .allow_unset = false,
    },
    {
        .endpoint = "/request/url",
        .type = ASTARTE_MAPPING_TYPE_STRING,
        .reliability = ASTARTE_MAPPING_RELIABILITY_GUARANTEED,
        .explicit_timestamp = false,
        .allow_unset = false,
    },
    {
        .endpoint = "/request/httpHeaderKey",
        .type = ASTARTE_MAPPING_TYPE_STRING,
        .reliability = ASTARTE_MAPPING_RELIABILITY_GUARANTEED,
        .explicit_timestamp = false,
        .allow_unset = false,
    },
    {
        .endpoint = "/request/httpHeaderValue",
        .type = ASTARTE_MAPPING_TYPE_STRING,
        .reliability = ASTARTE_MAPPING_RELIABILITY_GUARANTEED,
        .explicit_timestamp = false,
        .allow_unset = false,
    },
    {
        .endpoint = "/request/compression",
        .type = ASTARTE_MAPPING_TYPE_STRING,
        .reliability = ASTARTE_MAPPING_RELIABILITY_GUARANTEED,
        .explicit_timestamp = false,
        .allow_unset = false,
    },
    {
        .endpoint = "/request/progress",
        .type = ASTARTE_MAPPING_TYPE_BOOLEAN,
        .reliability = ASTARTE_MAPPING_RELIABILITY_GUARANTEED,
        .explicit_timestamp = false,
        .allow_unset = false,
    },
    {
        .endpoint = "/request/source",
        .type = ASTARTE_MAPPING_TYPE_STRING,
        .reliability = ASTARTE_MAPPING_RELIABILITY_GUARANTEED,
        .explicit_timestamp = false,
        .allow_unset = false,
    },
};

/** @brief Automatically generated interface definition. */
const astarte_interface_t io_edgehog_devicemanager_fileTransfer_DeviceToServer = {
    .name = "io.edgehog.devicemanager.fileTransfer.DeviceToServer",
    .major_version = 0,
    .minor_version = 1,
    .type = ASTARTE_INTERFACE_TYPE_DATASTREAM,
    .ownership = ASTARTE_INTERFACE_OWNERSHIP_SERVER,
    .aggregation = ASTARTE_INTERFACE_AGGREGATION_OBJECT,
    .mappings = io_edgehog_devicemanager_fileTransfer_DeviceToServer_mappings,
    .mappings_length = 7U,
};

/** @brief Automatically generated mapping definition. */
static const astarte_mapping_t io_edgehog_devicemanager_fileTransfer_Progress_mappings[2] = {

    {
        .endpoint = "/request/id",
        .type = ASTARTE_MAPPING_TYPE_STRING,
        .reliability = ASTARTE_MAPPING_RELIABILITY_UNRELIABLE,
        .explicit_timestamp = false,
        .allow_unset = false,
    },
    {
        .endpoint = "/request/progress",
        .type = ASTARTE_MAPPING_TYPE_INTEGER,
        .reliability = ASTARTE_MAPPING_RELIABILITY_UNRELIABLE,
        .explicit_timestamp = false,
        .allow_unset = false,
    },
};

/** @brief Automatically generated interface definition. */
const astarte_interface_t io_edgehog_devicemanager_fileTransfer_Progress = {
    .name = "io.edgehog.devicemanager.fileTransfer.Progress",
    .major_version = 0,
    .minor_version = 1,
    .type = ASTARTE_INTERFACE_TYPE_DATASTREAM,
    .ownership = ASTARTE_INTERFACE_OWNERSHIP_DEVICE,
    .aggregation = ASTARTE_INTERFACE_AGGREGATION_OBJECT,
    .mappings = io_edgehog_devicemanager_fileTransfer_Progress_mappings,
    .mappings_length = 2U,
};

/** @brief Automatically generated mapping definition. */
static const astarte_mapping_t io_edgehog_devicemanager_fileTransfer_Response_mappings[3] = {

    {
        .endpoint = "/request/id",
        .type = ASTARTE_MAPPING_TYPE_STRING,
        .reliability = ASTARTE_MAPPING_RELIABILITY_GUARANTEED,
        .explicit_timestamp = false,
        .allow_unset = false,
    },
    {
        .endpoint = "/request/code",
        .type = ASTARTE_MAPPING_TYPE_LONGINTEGER,
        .reliability = ASTARTE_MAPPING_RELIABILITY_GUARANTEED,
        .explicit_timestamp = false,
        .allow_unset = false,
    },
    {
        .endpoint = "/request/message",
        .type = ASTARTE_MAPPING_TYPE_STRING,
        .reliability = ASTARTE_MAPPING_RELIABILITY_GUARANTEED,
        .explicit_timestamp = false,
        .allow_unset = false,
    },
};

/** @brief Automatically generated interface definition. */
const astarte_interface_t io_edgehog_devicemanager_fileTransfer_Response = {
    .name = "io.edgehog.devicemanager.fileTransfer.Response",
    .major_version = 0,
    .minor_version = 1,
    .type = ASTARTE_INTERFACE_TYPE_DATASTREAM,
    .ownership = ASTARTE_INTERFACE_OWNERSHIP_DEVICE,
    .aggregation = ASTARTE_INTERFACE_AGGREGATION_OBJECT,
    .mappings = io_edgehog_devicemanager_fileTransfer_Response_mappings,
    .mappings_length = 3U,
};

/** @brief Automatically generated mapping definition. */
static const astarte_mapping_t
    io_edgehog_devicemanager_fileTransfer_posix_ServerToDevice_mappings[14] = {

        {
            .endpoint = "/request/id",
            .type = ASTARTE_MAPPING_TYPE_STRING,
            .reliability = ASTARTE_MAPPING_RELIABILITY_GUARANTEED,
            .explicit_timestamp = false,
            .allow_unset = false,
        },
        {
            .endpoint = "/request/url",
            .type = ASTARTE_MAPPING_TYPE_STRING,
            .reliability = ASTARTE_MAPPING_RELIABILITY_GUARANTEED,
            .explicit_timestamp = false,
            .allow_unset = false,
        },
        {
            .endpoint = "/request/httpHeaderKey",
            .type = ASTARTE_MAPPING_TYPE_STRING,
            .reliability = ASTARTE_MAPPING_RELIABILITY_GUARANTEED,
            .explicit_timestamp = false,
            .allow_unset = false,
        },
        {
            .endpoint = "/request/httpHeaderValue",
            .type = ASTARTE_MAPPING_TYPE_STRING,
            .reliability = ASTARTE_MAPPING_RELIABILITY_GUARANTEED,
            .explicit_timestamp = false,
            .allow_unset = false,
        },
        {
            .endpoint = "/request/compression",
            .type = ASTARTE_MAPPING_TYPE_STRING,
            .reliability = ASTARTE_MAPPING_RELIABILITY_GUARANTEED,
            .explicit_timestamp = false,
            .allow_unset = false,
        },
        {
            .endpoint = "/request/fileSizeBytes",
            .type = ASTARTE_MAPPING_TYPE_LONGINTEGER,
            .reliability = ASTARTE_MAPPING_RELIABILITY_GUARANTEED,
            .explicit_timestamp = false,
            .allow_unset = false,
        },
        {
            .endpoint = "/request/progress",
            .type = ASTARTE_MAPPING_TYPE_BOOLEAN,
            .reliability = ASTARTE_MAPPING_RELIABILITY_GUARANTEED,
            .explicit_timestamp = false,
            .allow_unset = false,
        },
        {
            .endpoint = "/request/digest",
            .type = ASTARTE_MAPPING_TYPE_STRING,
            .reliability = ASTARTE_MAPPING_RELIABILITY_GUARANTEED,
            .explicit_timestamp = false,
            .allow_unset = false,
        },
        {
            .endpoint = "/request/fileName",
            .type = ASTARTE_MAPPING_TYPE_STRING,
            .reliability = ASTARTE_MAPPING_RELIABILITY_GUARANTEED,
            .explicit_timestamp = false,
            .allow_unset = false,
        },
        {
            .endpoint = "/request/ttlSeconds",
            .type = ASTARTE_MAPPING_TYPE_LONGINTEGER,
            .reliability = ASTARTE_MAPPING_RELIABILITY_GUARANTEED,
            .explicit_timestamp = false,
            .allow_unset = false,
        },
        {
            .endpoint = "/request/fileMode",
            .type = ASTARTE_MAPPING_TYPE_LONGINTEGER,
            .reliability = ASTARTE_MAPPING_RELIABILITY_GUARANTEED,
            .explicit_timestamp = false,
            .allow_unset = false,
        },
        {
            .endpoint = "/request/userId",
            .type = ASTARTE_MAPPING_TYPE_LONGINTEGER,
            .reliability = ASTARTE_MAPPING_RELIABILITY_GUARANTEED,
            .explicit_timestamp = false,
            .allow_unset = false,
        },
        {
            .endpoint = "/request/groupId",
            .type = ASTARTE_MAPPING_TYPE_LONGINTEGER,
            .reliability = ASTARTE_MAPPING_RELIABILITY_GUARANTEED,
            .explicit_timestamp = false,
            .allow_unset = false,
        },
        {
            .endpoint = "/request/destination",
            .type = ASTARTE_MAPPING_TYPE_STRING,
            .reliability = ASTARTE_MAPPING_RELIABILITY_GUARANTEED,
            .explicit_timestamp = false,
            .allow_unset = false,
        },
    };

/** @brief Automatically generated interface definition. */
const astarte_interface_t io_edgehog_devicemanager_fileTransfer_posix_ServerToDevice = {
    .name = "io.edgehog.devicemanager.fileTransfer.posix.ServerToDevice",
    .major_version = 0,
    .minor_version = 1,
    .type = ASTARTE_INTERFACE_TYPE_DATASTREAM,
    .ownership = ASTARTE_INTERFACE_OWNERSHIP_SERVER,
    .aggregation = ASTARTE_INTERFACE_AGGREGATION_OBJECT,
    .mappings = io_edgehog_devicemanager_fileTransfer_posix_ServerToDevice_mappings,
    .mappings_length = 14U,
};

/** @brief Automatically generated mapping definition. */
static const astarte_mapping_t io_edgehog_devicemanager_storage_File_mappings[2] = {

    {
        .endpoint = "/%{requestId}/pathOnDevice",
        .type = ASTARTE_MAPPING_TYPE_STRING,
        .reliability = ASTARTE_MAPPING_RELIABILITY_UNIQUE,
        .explicit_timestamp = false,
        .allow_unset = false,
    },
    {
        .endpoint = "/%{requestId}/sizeBytes",
        .type = ASTARTE_MAPPING_TYPE_LONGINTEGER,
        .reliability = ASTARTE_MAPPING_RELIABILITY_UNIQUE,
        .explicit_timestamp = false,
        .allow_unset = false,
    },
};

/** @brief Automatically generated interface definition. */
const astarte_interface_t io_edgehog_devicemanager_storage_File = {
    .name = "io.edgehog.devicemanager.storage.File",
    .major_version = 0,
    .minor_version = 1,
    .type = ASTARTE_INTERFACE_TYPE_PROPERTIES,
    .ownership = ASTARTE_INTERFACE_OWNERSHIP_DEVICE,
    .aggregation = ASTARTE_INTERFACE_AGGREGATION_INDIVIDUAL,
    .mappings = io_edgehog_devicemanager_storage_File_mappings,
    .mappings_length = 2U,
};

// NOLINTEND(readability-identifier-naming)
