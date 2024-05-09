/*
 * (C) Copyright 2024, SECO Mind Srl
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef COMMAND_H
#define COMMAND_H

/**
 * @file command.h
 * @brief Edgehog command events handler.
 */

#include "edgehog_device/device.h"
#include "edgehog_device/result.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief receive Edgehog device commands.
 *
 * @details This function receives a command request from Astarte.
 *
 * @param event_request A valid Astarte datastream individual event.
 *
 * @return EDGEHOG_RESULT_OK if the command event is handled successfully, an edgehog_result_t
 * otherwise.
 */

edgehog_result_t edgehog_command_event(astarte_device_datastream_individual_event_t *event_request);

#ifdef __cplusplus
}
#endif

#endif /* COMMAND_H */
