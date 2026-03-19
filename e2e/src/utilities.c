/*
 * (C) Copyright 2024, SECO Mind Srl
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "utilities.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <zephyr/fatal.h>
#include <zephyr/logging/log.h>
#include <zephyr/shell/shell_uart.h>
#include <zephyr/sys/bitarray.h>
#include <zephyr/sys/timeutil.h>
#include <zephyr/sys/util.h>

/************************************************
 *   Constants, static variables and defines    *
 ***********************************************/

LOG_MODULE_REGISTER(utilities, CONFIG_UTILITIES_LOG_LEVEL);

/************************************************
 *         Static functions declaration         *
 ***********************************************/

/************************************************
 *         Global functions definition          *
 ***********************************************/

/************************************************
 *         Static functions definitions         *
 ***********************************************/
