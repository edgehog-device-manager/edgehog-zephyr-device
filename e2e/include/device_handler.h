/*
 * (C) Copyright 2025, SECO Mind Srl
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef DEVICE_HANDLER_H
#define DEVICE_HANDLER_H

#include "utilities.h"

void setup_device();
void free_device();
void wait_for_device_connection();
void disconnect_device();
void wait_for_device_disconnection();

#endif /* DEVICE_HANDLER_H */
