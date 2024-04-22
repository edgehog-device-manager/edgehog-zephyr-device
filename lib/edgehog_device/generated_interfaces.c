/**
 * @file generated_interfaces.c
 * @brief Contains automatically generated interfaces.
 *
 * @warning Do not modify this file manually.
 */

#include "generated_interfaces.h"

// Interface names should resemble as closely as possible their respective .json file names.
// NOLINTBEGIN(readability-identifier-naming)

static const astarte_mapping_t io_edgehog_devicemanager_HardwareInfo_mappings[5] = {

    {
        .endpoint = "/cpu/architecture",
        .regex_endpoint = "/cpu/architecture",
        .type = ASTARTE_MAPPING_TYPE_STRING,
        .reliability = ASTARTE_MAPPING_RELIABILITY_UNIQUE,
        .explicit_timestamp = false,
        .allow_unset = false,
    },
    {
        .endpoint = "/cpu/model",
        .regex_endpoint = "/cpu/model",
        .type = ASTARTE_MAPPING_TYPE_STRING,
        .reliability = ASTARTE_MAPPING_RELIABILITY_UNIQUE,
        .explicit_timestamp = false,
        .allow_unset = false,
    },
    {
        .endpoint = "/cpu/modelName",
        .regex_endpoint = "/cpu/modelName",
        .type = ASTARTE_MAPPING_TYPE_STRING,
        .reliability = ASTARTE_MAPPING_RELIABILITY_UNIQUE,
        .explicit_timestamp = false,
        .allow_unset = false,
    },
    {
        .endpoint = "/cpu/vendor",
        .regex_endpoint = "/cpu/vendor",
        .type = ASTARTE_MAPPING_TYPE_STRING,
        .reliability = ASTARTE_MAPPING_RELIABILITY_UNIQUE,
        .explicit_timestamp = false,
        .allow_unset = false,
    },
    {
        .endpoint = "/mem/totalBytes",
        .regex_endpoint = "/mem/totalBytes",
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

static const astarte_mapping_t io_edgehog_devicemanager_OSInfo_mappings[2] = {

    {
        .endpoint = "/osName",
        .regex_endpoint = "/osName",
        .type = ASTARTE_MAPPING_TYPE_STRING,
        .reliability = ASTARTE_MAPPING_RELIABILITY_UNIQUE,
        .explicit_timestamp = false,
        .allow_unset = false,
    },
    {
        .endpoint = "/osVersion",
        .regex_endpoint = "/osVersion",
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

// NOLINTEND(readability-identifier-naming)
