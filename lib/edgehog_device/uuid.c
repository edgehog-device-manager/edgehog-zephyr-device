/*
 * (C) Copyright 2024, SECO Mind Srl
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "uuid.h"

#include <stdio.h>

#include <zephyr/posix/arpa/inet.h>
#include <zephyr/random/random.h>
#include <zephyr/sys/base64.h>

#include "edgehog_device/result.h"
#include "log.h"

EDGEHOG_LOG_MODULE_REGISTER(edgehog_uuid, CONFIG_EDGEHOG_DEVICE_UUID_LOG_LEVEL);

// All the macros below follow the standard for the Universally Unique Identifier as defined
// by the IETF in the RFC9562.
// The full specification can be found here: https://datatracker.ietf.org/doc/rfc9562/

// Position of the hyphens
#define UUID_STR_POSITION_FIRST_HYPHEN (8U)
#define UUID_STR_POSITION_SECOND_HYPHEN (13U)
#define UUID_STR_POSITION_THIRD_HYPHEN (18U)
#define UUID_STR_POSITION_FOURTH_HYPHEN (23U)

// Common positions, offsets and masks for all UUID versions
#define UUID_POSITION_VERSION (6U)
#define UUID_OFFSET_VERSION (4U)
#define UUID_MASK_VERSION (0b11110000U)
#define UUID_POSITION_VARIANT (8U)
#define UUID_OFFSET_VARIANT (6U)
#define UUID_MASK_VARIANT (0b11000000U)

#define UUID_V4_VERSION (4U)
#define UUID_V4_VARIANT (2U)

/**
 * @brief Overwrite the 'ver' and 'var' fields in the input UUID.
 *
 * @param[inout] uuid The UUID to use for the operation.
 * @param[in] version The new version for the UUID. Only the first 4 bits will be used.
 * @param[in] variant The new variant for the UUID. Only the first 2 bits will be used.
 */
static void overwrite_uuid_version_and_variant(uuid_t uuid, uint8_t version, uint8_t variant);
/**
 * @brief Convert a UUID to its canonical (RFC9562) string representation.
 *
 * @param[in] uuid The UUID to convert to string.
 * @param[out] out A pointer to a previously allocated buffer where the result will be written.
 * @param[in] out_size The size of the out buffer. Should be at least 37 bytes.
 * @return EDGEHOG_RESULT_OK on success, an error code otherwise.
 */
static edgehog_result_t uuid_to_string(const uuid_t uuid, char *out, size_t out_size);
/**
 * @brief Generate a UUIDv4.
 *
 * @details This function computes a random UUID using hardware RNG.
 *
 * @param[out] out The UUID where the result will be written.
 */
static void uuid_generate_v4(uuid_t out);

edgehog_result_t uuid_generate_v4_string(char out[static UUID_STR_LEN + 1])
{
    uuid_t v4_uuid = { 0 };
    uuid_generate_v4(v4_uuid);
    return uuid_to_string(v4_uuid, out, UUID_STR_LEN + 1);
}

static void overwrite_uuid_version_and_variant(uuid_t uuid, uint8_t version, uint8_t variant)
{
    // Clear the 'ver' and 'var' fields
    uuid[UUID_POSITION_VERSION] &= ~UUID_MASK_VERSION;
    uuid[UUID_POSITION_VARIANT] &= ~UUID_MASK_VARIANT;
    // Update the 'ver' and 'var' fields
    uuid[UUID_POSITION_VERSION] |= (uint8_t) (version << UUID_OFFSET_VERSION);
    uuid[UUID_POSITION_VARIANT] |= (uint8_t) (variant << UUID_OFFSET_VARIANT);
}

static void uuid_generate_v4(uuid_t out)
{
    // Fill the whole UUID struct with a random number
    sys_rand_get(out, UUID_SIZE);
    // Update version and variant
    overwrite_uuid_version_and_variant(out, UUID_V4_VERSION, UUID_V4_VARIANT);
}

static edgehog_result_t uuid_to_string(const uuid_t uuid, char *out, size_t out_size)
{
    size_t min_out_size = UUID_STR_LEN + 1;
    if (out_size < min_out_size) {
        EDGEHOG_LOG_ERR("Output buffer should be at least %zu bytes long", min_out_size);
        return EDGEHOG_RESULT_INTERNAL_ERROR;
    }

    // NOLINTBEGIN(readability-magic-numbers, cppcoreguidelines-avoid-magic-numbers)
    int res = snprintf(out, min_out_size,
        "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x", uuid[0], uuid[1],
        uuid[2], uuid[3], uuid[4], uuid[5], uuid[6], uuid[7], uuid[8], uuid[9], uuid[10], uuid[11],
        uuid[12], uuid[13], uuid[14], uuid[15]);
    // NOLINTEND(readability-magic-numbers, cppcoreguidelines-avoid-magic-numbers)
    if ((res < 0) || (res >= min_out_size)) {
        EDGEHOG_LOG_ERR("Error converting UUID to string.");
        return EDGEHOG_RESULT_INTERNAL_ERROR;
    }

    return EDGEHOG_RESULT_OK;
}
