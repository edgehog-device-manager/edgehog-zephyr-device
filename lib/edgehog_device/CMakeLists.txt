# (C) Copyright 2024, SECO Mind Srl
#
# SPDX-License-Identifier: Apache-2.0

zephyr_library()

# TODO: remove this as it's a workaround to make available the mbedTLS crypto headers in zephyr 4.3
zephyr_library_link_libraries(mbedTLS)

zephyr_library_include_directories(include)

string(TIMESTAMP BUILD_DATE_TIME UTC)
# This define is used in the BaseImage interface
add_compile_definitions(CMAKE_BUILD_DATE_TIME="${BUILD_DATE_TIME}")

FILE(GLOB lib_sources *.c)
zephyr_library_sources(${lib_sources})

if(CONFIG_EDGEHOG_DEVICE_FILE_TRANSFER)
    FILE(GLOB ft_sources file_transfer/*.c)

    # Remove the compression source file if the config is not enabled
    if(NOT CONFIG_EDGEHOG_DEVICE_FILE_TRANSFER_COMPRESSION)
        list(REMOVE_ITEM ft_sources "${CMAKE_CURRENT_SOURCE_DIR}/file_transfer/compression.c")
        list(REMOVE_ITEM ft_sources "${CMAKE_CURRENT_SOURCE_DIR}/file_transfer/decompression.c")
    endif()

    zephyr_library_sources(${ft_sources})
endif()
