# (C) Copyright 2024, SECO Mind Srl
#
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
project(library-documentation LANGUAGES)

set(DOXYGEN_CMAKE_TARGET_NAME library_documentation)

#-------------------------------------------------------------------------------
# Project specific defaults

if(NOT DEFINED DOXYGEN_LIBRARY_NAME)
    set(DOXYGEN_LIBRARY_NAME "Edgehog device API for Zephyr")
endif()

if(NOT DEFINED DOXYGEN_LIBRARY_VERSION)
    set(DOXYGEN_LIBRARY_VERSION "0.9.0")
endif()

if(NOT DEFINED DOXYGEN_LIBRARY_BRIEF)
    set(DOXYGEN_LIBRARY_BRIEF "Edgehog device SDK for Zephyr RTOS")
endif()

set(DOXYGEN_LIBRARY_DIR "edgehog_device")

if(NOT DEFINED DOXYGEN_PREDEFINED)
    set(DOXYGEN_PREDEFINED "CONFIG_EDGEHOG_DEVICE_ZBUS_OTA_EVENT CONFIG_WIFI")
endif()

#-------------------------------------------------------------------------------
# Doxygen Inputs

# Define common sources used by both standard and extended documentation
set(COMMON_SOURCES
    "${DOXYGEN_LIBRARY_PATH}/README.md"
    "${DOXYGEN_LIBRARY_PATH}/doc/architecture"
    "${DOXYGEN_LIBRARY_PATH}/doc/_doxygen/groups.dox"
    "${DOXYGEN_LIBRARY_PATH}/include/"
)

if("${EXTENDED_DOCS}" STREQUAL "yes")
  message(STATUS "Generating extended documentation.")
  set(DOXYFILE_IN ${CMAKE_CURRENT_LIST_DIR}/library.extended.doxyfile.in)

  # Combine common sources with the extended paths
  set(DOXYGEN_INPUT_SOURCES
      ${COMMON_SOURCES}
      "${DOXYGEN_LIBRARY_PATH}/lib/${DOXYGEN_LIBRARY_DIR}/include/"
      "${DOXYGEN_LIBRARY_PATH}/lib/${DOXYGEN_LIBRARY_DIR}/"
  )
else()
  set(DOXYFILE_IN ${CMAKE_CURRENT_LIST_DIR}/library.doxyfile.in)

  # Use only common sources for standard documentation
  set(DOXYGEN_INPUT_SOURCES ${COMMON_SOURCES})
endif()

# Convert the CMake list into a space-separated string for the Doxyfile
string(REPLACE ";" " " DOXYGEN_INPUT_SOURCES "${DOXYGEN_INPUT_SOURCES}")

#-------------------------------------------------------------------------------
# Options

message(STATUS "Library name: ${DOXYGEN_LIBRARY_NAME}")
message(STATUS "Library version: ${DOXYGEN_LIBRARY_VERSION}")
message(STATUS "Library brief: ${DOXYGEN_LIBRARY_BRIEF}")
message(STATUS "Library path: ${DOXYGEN_LIBRARY_PATH}")
message(STATUS "Predefined definitions: ${DOXYGEN_PREDEFINED}")

#-------------------------------------------------------------------------------
# Dependencies

find_package(Doxygen REQUIRED dot)

#-------------------------------------------------------------------------------
# Doxygen

set(DOXY_OUT ${CMAKE_CURRENT_BINARY_DIR}/doxygen)

if("${EXTENDED_DOCS}" STREQUAL "yes")
  message(STATUS "Generating extended documentation.")
  set(DOXYFILE_IN ${CMAKE_CURRENT_LIST_DIR}/library.extended.doxyfile.in)
else()
  set(DOXYFILE_IN ${CMAKE_CURRENT_LIST_DIR}/library.doxyfile.in)
endif()

set(DOXYFILE_OUT ${CMAKE_CURRENT_BINARY_DIR}/zephyr.doxyfile)

configure_file(${DOXYFILE_IN} ${DOXYFILE_OUT} @ONLY)

add_custom_target(
  ${DOXYGEN_CMAKE_TARGET_NAME}
  COMMAND
    ${DOXYGEN_EXECUTABLE} ${DOXYFILE_OUT}
  COMMENT "Running Doxygen..."
)

set_target_properties(
  ${DOXYGEN_CMAKE_TARGET_NAME}
  PROPERTIES
    ADDITIONAL_CLEAN_FILES "${DOXY_OUT}"
)
