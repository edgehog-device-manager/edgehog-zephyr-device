# (C) Copyright 2026, SECO Mind Srl
#
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

project(edgehog_device_unit)
find_package(Zephyr COMPONENTS unittest REQUIRED HINTS $ENV{ZEPHYR_BASE})

target_include_directories(testbinary PRIVATE
    ${ZEPHYR_BASE}/../edgehog-zephyr-device/include
    ${ZEPHYR_BASE}/../edgehog-zephyr-device
    ${ZEPHYR_BASE}/../edgehog-zephyr-device/lib/edgehog_device/include
    ${ZEPHYR_BASE}/../modules/fs/littlefs
)

FILE(GLOB test_sources src/*.c)
target_sources(testbinary PRIVATE ${test_sources})
