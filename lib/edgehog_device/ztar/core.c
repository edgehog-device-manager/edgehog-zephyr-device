/*
 * (C) Copyright 2026, SECO Mind Srl
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ztar/core.h"

#include <zephyr/kernel.h>

/************************************************
 *       Checks over configuration values       *
 ***********************************************/

BUILD_ASSERT(sizeof(ztar_header_t) == ZTAR_BLOCK_SIZE, "ZTAR Incompatible header struct packing.");
