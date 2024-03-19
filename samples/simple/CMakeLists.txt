# (C) Copyright 2024, SECO Mind Srl
#
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(simple)

FILE(GLOB app_sources src/*.c)

if(NOT CONFIG_WIFI)
    LIST(REMOVE_ITEM app_sources ${CMAKE_CURRENT_LIST_DIR}/src/wifi.c)
endif()

target_include_directories(app PRIVATE src/include)
target_sources(app PRIVATE ${app_sources})
