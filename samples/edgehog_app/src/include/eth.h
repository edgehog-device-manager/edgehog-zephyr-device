/*
 * (C) Copyright 2024, SECO Mind Srl
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ETH_H
#define ETH_H

#include <stdbool.h>
#include <stddef.h>

/**
 * @brief Blocking function to connect the device to internet.
 *
 * @note This is not actually initializing the ethernet module which is intialized by default.
 *
 * @return 0 upon success, -1 otherwise.
 */
int eth_connect(void);

/**
 * @brief Poll the Ethernet driver, ensuring a connection is present.
 *
 * @note This function will block when connectivity is not present.
 */
void eth_poll(void);

#endif /* ETH_H */
