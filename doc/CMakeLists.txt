# (C) Copyright 2024, SECO Mind Srl
#
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
project(Edgehog-Device-Doc LANGUAGES)

set(MIN_WEST_VERSION 1.0.0)

set(Edgehog_Device_VERSION 0.9.0)

message(STATUS "Zephyr base: ${LIBRARY_PATH}")
message(STATUS "Documentation tag: ${DOC_TAG}")

#-------------------------------------------------------------------------------
# Options

set(DOC_TAG "development" CACHE STRING "Documentation tag")

#-------------------------------------------------------------------------------
# Dependencies

find_package(Doxygen REQUIRED dot)

#-------------------------------------------------------------------------------
# Environment & Paths

set(DOCS_CFG_DIR ${CMAKE_CURRENT_LIST_DIR})
set(DOCS_BUILD_DIR ${CMAKE_CURRENT_BINARY_DIR})
set(DOCS_SRC_DIR ${CMAKE_CURRENT_BINARY_DIR}/src)
set(DOCS_HTML_DIR ${CMAKE_CURRENT_BINARY_DIR}/html)

if(WIN32)
  set(SEP $<SEMICOLON>)
else()
  set(SEP :)
endif()

#-------------------------------------------------------------------------------
# Doxygen (standalone)

set(DOXY_OUT ${CMAKE_CURRENT_BINARY_DIR}/doxygen)

if("${EXTENDED_DOCS}" STREQUAL "yes")
message(STATUS "Generating extended documentation.")
set(DOXYFILE_IN ${CMAKE_CURRENT_LIST_DIR}/edgehog-zephyr-device.extended.doxyfile.in)
else()
set(DOXYFILE_IN ${CMAKE_CURRENT_LIST_DIR}/edgehog-zephyr-device.doxyfile.in)
endif()

set(DOXYFILE_OUT ${CMAKE_CURRENT_BINARY_DIR}/zephyr.doxyfile)
set(EDGEHOG_DEVICE_VERSION "${Edgehog_Device_VERSION}")

configure_file(${DOXYFILE_IN} ${DOXYFILE_OUT} @ONLY)

add_custom_target(
  doxygen
  COMMAND
    ${DOXYGEN_EXECUTABLE} ${DOXYFILE_OUT}
  COMMENT "Running Doxygen..."
)

set_target_properties(
  doxygen
  PROPERTIES
    ADDITIONAL_CLEAN_FILES "${DOXY_OUT}"
)
