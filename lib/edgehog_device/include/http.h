/*
 * (C) Copyright 2024, SECO Mind Srl
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef HTTP_H
#define HTTP_H

/**
 * @file http.h
 * @brief Low level connectivity functions
 */

#include "edgehog_device/result.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/** @brief Chunk of response from the server. */
typedef struct
{
    /** @brief Start address of the chunk contained in the response buffer. */
    uint8_t *chunk_start_addr;
    /** @brief Size of the chunk contained in the response buffer. */
    size_t chunk_size;
    /** @brief Size of the response. */
    size_t response_size;
    /** @brief Identify the last chunk of the response. */
    bool last_chunk;
} edgehog_http_response_chunk_t;

/** @brief Chunk of payload to send to the server. */
typedef struct
{
    /** @brief Start address of the chunk to send. */
    uint8_t *chunk_start_addr;
    /** @brief Size of the chunk to send. */
    size_t chunk_size;
    /** @brief Identify the last chunk of the send. */
    bool last_chunk;
} edgehog_http_payload_chunk_t;

/**
 * @typedef edgehog_http_response_cbk_t
 * @brief Callback used when chunk of response is received from the server.
 *
 * @param response_chunk Response chunk information.
 * @param user_data User specified data.
 * @return EDGEHOG_RESULT_OK if successful, otherwise an error code.
 */
typedef edgehog_result_t (*edgehog_http_response_cbk_t)(
    edgehog_http_response_chunk_t *response_chunk, void *user_data);

/**
 * @typedef edgehog_http_payload_cbk_t
 * @brief Callback used when a chunk of payload is requested to be sent to the server.
 *
 * @param payload_chunk Payload chunk information to be populated by the user.
 * @param user_data User specified data provided in the *edgehog_http_put_data_t* struct.
 *
 * @return EDGEHOG_RESULT_OK if successful, otherwise an error code.
 */
typedef edgehog_result_t (*edgehog_http_payload_cbk_t)(
    edgehog_http_payload_chunk_t *payload_chunk, void *user_data);

/** @brief Data struct for a HTTP GET instance. */
typedef struct
{
    /** @brief URL to use for the HTTP GET request. */
    const char *url;
    /** @brief NULL terminated list of headers for the request. */
    const char **header_fields;
    /** @brief Timeout to use for the HTTP operations in ms. */
    int32_t timeout_ms;
    /** @brief Callback for a chunk response event. */
    edgehog_http_response_cbk_t response_cbk;
    /** @brief User data passed to the callback function. */
    void *user_data;
} edgehog_http_get_data_t;

/** @brief Data struct for an HTTP PUT instance. */
typedef struct
{
    /** @brief URL to use for the HTTP PUT request. */
    const char *url;
    /** @brief NULL terminated list of headers for the request. */
    const char **header_fields;
    /** @brief Timeout to use for the HTTP operations in ms. */
    int32_t timeout_ms;
    /** @brief Callback for a chunk payload event. */
    edgehog_http_payload_cbk_t payload_cbk;
    /** @brief User data passed to the callback function. */
    void *user_data;
} edgehog_http_put_data_t;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Perform an HTTP GET file request.
 *
 * @param[in] data Pointer to the HTTP GET request data.
 * @return EDGEHOG_RESULT_OK if successful, otherwise an error code.
 */
edgehog_result_t edgehog_http_get(edgehog_http_get_data_t *data);

/**
 * @brief Perform an HTTP PUT request.
 *
 * @param[in] data Pointer to the HTTP PUT request data.
 * @return EDGEHOG_RESULT_OK if successful, otherwise an error code.
 */
edgehog_result_t edgehog_http_put(edgehog_http_put_data_t *data);

#ifdef __cplusplus
}
#endif

#endif /* HTTP_H */
