/*
 * (C) Copyright 2024, SECO Mind Srl
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef E2E_CA_CERTIFICATES_H
#define E2E_CA_CERTIFICATES_H

// astarte_root_certificate.inc is generated at build time by certificate.cmake
static const unsigned char astarte_ca_certificate_root[] = {
#include "astarte_root_certificate.inc"
    0x00
};

// GTS Root R1
static const unsigned char edgehog_ota_ca_certificate_root[] = {
#include "edgehog_ota_root_certificate.inc"
    0x00
};

// Auto signed certificate for FT server
static const unsigned char edgehog_ft_ca_certificate_root[] = {
#include "edgehog_ft_root_certificate.inc"
    0x00
};

#endif /* E2E_CA_CERTIFICATES_H */
