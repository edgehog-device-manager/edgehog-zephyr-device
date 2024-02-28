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

/**
 * @typedef http_download_payload_cbk_t
 * @brief Chunk of download from the server.
 */
typedef struct
{
    /** @brief Start address of the chunk contained in the download buffer. */
    uint8_t *chunk_start_addr;
    /** @brief Size of the chunk contained in the download buffer. */
    size_t chunk_size;
    /** @brief Size of the download. */
    size_t download_size;
    /** @brief Identify the last chunk of the download. */
    bool last_chunk;
} http_download_chunk_t;

/**
 * @typedef http_download_payload_cbk_t
 * @brief Callback used when chunk of download is received from the server.
 *
 * @param http_download Download information.
 * @param download_chunk Download chunk information.
 * @param user_data User specified data specified in *http_download_t* struct.
 *
 * @return EDGEHOG_RESULT_OK if successful, otherwise an error code.
 */
typedef edgehog_result_t (*http_download_payload_cbk_t)(
    int sock_id, http_download_chunk_t *download_chunk, void *user_data);

typedef struct
{
    /** @brief Callback for a chunk download event. */
    http_download_payload_cbk_t download_cbk;
    /** @brief Socket id of the connection. */
    int sock_id;
    /** @brief Edgehog result of download_cbk execution*/
    edgehog_result_t edegehog_result;
    /** @brief User data passed to http_download_cb callback function. */
    void *user_data;
} http_download_t;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Perform a dowload file request.
 *
 * @param[in] url URL to use for the Download request.
 * @param[in] header_fields NULL terminated list of headers for the request.
 * @param[in] timeout_ms Timeout to use for the HTTP operations in ms.
 * @param[in] http_download Http Download specified data that is used for receiveing an download.
 * chunk.
 * @return EDGEHOG_RESULT_OK if successful, otherwise an error code.
 */
edgehog_result_t edgehog_http_download(const char *url, const char **header_fields,
    int32_t timeout_ms, http_download_t *http_download);

/**
 * @brief Abort a specific dowload file request.
 *
 * @param[in] sock_id Socket id of the connection.
 * chunk.
 */
void edgehog_http_download_abort(int sock_id);

#ifdef __cplusplus
}
#endif

#endif /* HTTP_H */
