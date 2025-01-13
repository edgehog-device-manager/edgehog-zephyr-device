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
 * @brief Handle an Edgehog device command.
 *
 * @details This function handles a command request from Astarte.
 *
 * @param event_request A valid Astarte datastream individual event.
 * @return #EDGEHOG_RESULT_OK on success, an error code otherwise.
 */

edgehog_result_t edgehog_command_event(astarte_device_datastream_individual_event_t *event_request);

#ifdef __cplusplus
}
#endif

#endif /* COMMAND_H */
