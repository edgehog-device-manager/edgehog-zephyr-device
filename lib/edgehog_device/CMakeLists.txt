# (C) Copyright 2024, SECO Mind Srl
#
# SPDX-License-Identifier: Apache-2.0

zephyr_library()
zephyr_library_include_directories(include)

string(TIMESTAMP BUILD_DATE_TIME UTC)
# This define is used in the BaseImage interface
add_compile_definitions(CMAKE_BUILD_DATE_TIME="${BUILD_DATE_TIME}")

FILE(GLOB lib_sources *.c)
zephyr_library_sources(${lib_sources})
