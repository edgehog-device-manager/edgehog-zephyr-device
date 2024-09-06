/*
 * (C) Copyright 2024, SECO Mind Srl
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef UUID_H
#define UUID_H

/**
 * @file uuid.h
 * @brief Utility functions for the generation and parsing of Universal Unique Identifier.
 */

#include <stdint.h>

#include "edgehog_device/result.h"

/** @brief Number of bytes in the binary representation of a UUID. */
#define UUID_SIZE 16

/** @brief Length of the UUID canonical string representation. */
#define UUID_STR_LEN 36

/** @brief Binary representation of a UUID. */
typedef uint8_t uuid_t[UUID_SIZE];

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Generate a UUIDv4 string.
 *
 * @details This function computes a random UUID using hardware RNG.
 *
 * @param[out] out The UUID where the result will be written.
 *
 * @return EDGEHOG_RESULT_OK on success, an error code otherwise.
 */
edgehog_result_t uuid_generate_v4_string(char out[static UUID_STR_LEN + 1]);

#ifdef __cplusplus
}
#endif

#endif // UUID_H
