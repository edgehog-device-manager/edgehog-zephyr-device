/*
 * (C) Copyright 2024, SECO Mind Srl
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LOG_H
#define LOG_H

#include <zephyr/logging/log.h>

#define EDGEHOG_LOG_MODULE_REGISTER(...) LOG_MODULE_REGISTER(__VA_ARGS__) // NOLINT

#define EDGEHOG_LOG_MODULE_DECLARE(...) LOG_MODULE_DECLARE(__VA_ARGS__) // NOLINT

#define EDGEHOG_LOG_DBG(...) LOG_DBG(__VA_ARGS__) // NOLINT

#define EDGEHOG_LOG_INF(...) LOG_INF(__VA_ARGS__) // NOLINT

#define EDGEHOG_LOG_WRN(...) LOG_WRN(__VA_ARGS__) // NOLINT

#define EDGEHOG_LOG_ERR(...) LOG_ERR(__VA_ARGS__) // NOLINT

#define EDGEHOG_LOG_HEXDUMP_DBG(...) LOG_HEXDUMP_DBG(__VA_ARGS__) // NOLINT

#endif // LOG_H
