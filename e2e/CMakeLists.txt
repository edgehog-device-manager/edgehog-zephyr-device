# (C) Copyright 2024, SECO Mind Srl
#
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

function(concat_if_exists FILE_PATH STRING_NAME)
    # Check if the file exists
    file(GLOB FILE_TO_ADD ${FILE_PATH})

    # If the file exists, add it to the list
    if(EXISTS ${FILE_TO_ADD})
        set(${STRING_NAME} "${${STRING_NAME}};${FILE_PATH}" PARENT_SCOPE)
    endif()
endfunction()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# add some custom functions from the sample directory
set(SAMPLE_DIR "${CMAKE_CURRENT_LIST_DIR}/../samples/edgehog_app")

# add the sample specific private configuration
concat_if_exists(${CMAKE_SOURCE_DIR}/private.conf EXTRA_CONF_FILE)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(e2e_test)

# add sources and includes from the sample dir
target_sources(app PRIVATE ${SAMPLE_DIR}/src/eth.c)
target_include_directories(app PRIVATE ${SAMPLE_DIR}/src/include)

# Generate the key/certificate pair for the HTTP server if not already present
set(CERT_OUT "${CMAKE_CURRENT_SOURCE_DIR}/${CONFIG_E2E_HTTP_SERVER_CERT_DIR}/192.0.2.2.pem")
set(KEY_OUT "${CMAKE_CURRENT_SOURCE_DIR}/${CONFIG_E2E_HTTP_SERVER_CERT_DIR}/192.0.2.2.key")
set(CERT_CONFIG "${CMAKE_CURRENT_SOURCE_DIR}/${CONFIG_E2E_HTTP_SERVER_CERT_DIR}/cert_config.cnf")

message(STATUS "Generating self-signed certificate and key...")
execute_process(
    COMMAND openssl req -x509 -new -nodes
            -keyout "${KEY_OUT}"
            -out "${CERT_OUT}"
            -days 3650
            -config "${CERT_CONFIG}"
    RESULT_VARIABLE openssl_result
)
if(NOT openssl_result EQUAL "0")
    message(FATAL_ERROR "Failed to generate certificates with OpenSSL")
endif()

# handle generating certificate inc file
include(certificate.cmake)
if(CONFIG_E2E_ASTARTE_TLS_CERTIFICATE_PATH)
    generate_certificate_inc(${CONFIG_E2E_ASTARTE_TLS_CERTIFICATE_PATH} "astarte_root_certificate")
endif()
if(CONFIG_E2E_EDGEHOG_OTA_TLS_CERTIFICATE_PATH)
    generate_certificate_inc(${CONFIG_E2E_EDGEHOG_OTA_TLS_CERTIFICATE_PATH} "edgehog_ota_root_certificate")
endif()
generate_certificate_inc("${CONFIG_E2E_HTTP_SERVER_CERT_DIR}/192.0.2.2.pem" "edgehog_ft_root_certificate")

# add astarte private headers
target_include_directories(app PRIVATE ${CMAKE_SOURCE_DIR}/../lib/edgehog_device/include)

# add sources
FILE(GLOB app_sources src/*.c)
target_sources(app PRIVATE ${app_sources})
target_include_directories(app PRIVATE include)
