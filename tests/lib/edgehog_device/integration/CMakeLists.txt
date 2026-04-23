# (C) Copyright 2024, SECO Mind Srl
#
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(edgehog_device_integration)

target_include_directories(app PRIVATE
        ${ZEPHYR_BASE}/../edgehog-zephyr-device/lib/edgehog_device/include
)

FILE(GLOB test_sources src/*.c)
target_sources(app PRIVATE ${test_sources})
