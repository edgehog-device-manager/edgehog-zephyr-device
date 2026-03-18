/*
 * (C) Copyright 2026, SECO Mind Srl
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "shell_handlers.h"

#include <stdlib.h>
#include <string.h>

#include <zephyr/logging/log.h>
#include <zephyr/shell/shell.h>
#include <zephyr/sys/base64.h>

#include "device_handler.h"
#include "utilities.h"

LOG_MODULE_REGISTER(shell_handlers, CONFIG_SHELL_HANDLERS_LOG_LEVEL);

/************************************************
 *   Constants, static variables and defines    *
 ***********************************************/

#define DISCONNECT_CMD dvcshellcmd_disconnect
#define DISCONNECT_HELP "Disconnect the device and end the executable"

/************************************************
 *         Static functions declaration         *
 ***********************************************/

static int cmd_disconnect(const struct shell *sh, size_t argc, char **argv);

/************************************************
 *          Shell commands declaration          *
 ***********************************************/

SHELL_CMD_REGISTER(DISCONNECT_CMD, NULL, DISCONNECT_HELP, cmd_disconnect);

/************************************************
 *         Static functions definitions         *
 ***********************************************/

static int cmd_disconnect(const struct shell *sh, size_t argc, char **argv)
{
    ARG_UNUSED(sh);
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);

    LOG_INF("Disconnect command handler");
    LOG_INF("Stopping and joining the astarte device polling thread.");
    disconnect_device();
    return 0;
}
