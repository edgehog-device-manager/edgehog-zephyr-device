# (C) Copyright 2024, SECO Mind Srl
#
# SPDX-License-Identifier: Apache-2.0

zephyr_library()

FILE(GLOB lib_sources *.c)
zephyr_library_sources(${lib_sources})
