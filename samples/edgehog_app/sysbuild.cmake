
# (C) Copyright 2024, SECO Mind Srl
#
# SPDX-License-Identifier: Apache-2.0

if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/sysbuild/boards/${BOARD}.overlay")
  list(APPEND mcuboot_DTC_OVERLAY_FILE "${CMAKE_CURRENT_LIST_DIR}/sysbuild/boards/${BOARD}.overlay")
  list(REMOVE_DUPLICATES mcuboot_DTC_OVERLAY_FILE)
  set(mcuboot_DTC_OVERLAY_FILE "${mcuboot_DTC_OVERLAY_FILE}" CACHE INTERNAL "")
endif()
