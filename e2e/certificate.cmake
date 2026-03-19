# (C) Copyright 2024, SECO Mind Srl
#
# SPDX-License-Identifier: Apache-2.0

function(generate_certificate_inc certificate_path inc_file_name)

  if(NOT certificate_path)
    message(WARNING "Missing 'certificate_path', no certificate include file will be generated")
    return()
  endif()

  if(NOT inc_file_name)
    message(WARNING "Missing 'inc_file_name', no certificate include file will be generated")
    return()
  endif()

  get_filename_component(certificate_file "${CMAKE_SOURCE_DIR}/${certificate_path}" ABSOLUTE)

  if(NOT EXISTS "${certificate_file}")
      message(FATAL_ERROR "Certificate file '${certificate_file}' does not exist")
      return()
  endif()

  generate_inc_file_for_target(
      app
      "${certificate_file}"
      "${ZEPHYR_BINARY_DIR}/include/generated/${inc_file_name}.inc"
  )
endfunction()
