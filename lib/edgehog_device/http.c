/*
 * (C) Copyright 2024, SECO Mind Srl
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "http.h"

#include <zephyr/kernel.h>
#include <zephyr/net/http/client.h>
#include <zephyr/net/http/parser.h>
#include <zephyr/net/http/status.h>
#include <zephyr/net/socket.h>
#include <zephyr/version.h>

#ifndef CONFIG_EDGEHOG_DEVICE_DEVELOP_USE_NON_TLS_HTTP
#include <zephyr/net/tls_credentials.h>
#endif

#include <stdio.h>
#include <string.h>

#include "log.h"
EDGEHOG_LOG_MODULE_REGISTER(edgehog_http, CONFIG_EDGEHOG_DEVICE_HTTP_LOG_LEVEL);

/************************************************
 *        Defines, constants and typedef        *
 ***********************************************/

/** @brief Context struct for a generic HTTP request callback */
struct request_cbk_ctx
{
    /** @brief Result to store the success or failure of the request */
    edgehog_result_t result;
    /** @brief Callback for a the payload event of an HTTP request. */
    edgehog_http_payload_cbk_t payload_cbk;
    /** @brief Callback for a the response event of an HTTP request. */
    edgehog_http_response_cbk_t response_cbk;
    /** @brief User data passed to callback functions. */
    void *user_data;
};

/** @brief Data struct holding internal parameters for a generic HTTP request. */
struct request_data
{
    /** @brief The HTTP method to use (e.g., HTTP_GET, HTTP_PUT). */
    enum http_method method;
    /** @brief The target URL for the request. */
    const char *url;
    /** @brief NULL terminated list of headers for the request. */
    const char **header_fields;
    /** @brief Timeout to use for the HTTP operations in ms. */
    int32_t timeout_ms;
    /** @brief Internal Zephyr callback for processing payload uploads. */
    http_payload_cb_t payload_cbk;
    /** @brief Internal Zephyr callback for processing HTTP responses. */
    http_response_cb_t response_cbk;
    /** @brief Context passed to the HTTP client containing user callbacks and state. */
    struct request_cbk_ctx cbk_ctx;
};

#define PORT_STR_LEN 6
#define HTTPS_STR "https"
#define HTTPS_STR_LEN sizeof(HTTPS_STR)

/** @brief Buffer size for formatting chunk length in HTTP chunked transfer encoding. */
#define HTTP_CHUNKED_PAYLOAD_CHUNK_LENGTH_BUFFER_SIZE 32

/************************************************
 *         Static functions declaration         *
 ***********************************************/

/**
 * @brief Create a new TCP socket and connect it to a server.
 * @note The returned socket should be closed once its use has terminated.
 *
 * @param[in] host domain name, a string representation of an IPv4.
 * @param[in] port service port, a string representation of HTTP service port.
 * @return -1 upon failure, a file descriptor for the new socket otherwise.
 */
static int create_and_connect_socket(const char *host, const char *port);

/**
 * @brief Configures, initiates, and manages an HTTP request (GET or PUT).
 *
 * @param[in] data Pointer to the internal request configuration structure.
 * @return EDGEHOG_RESULT_OK if successful, otherwise an error code.
 */
static edgehog_result_t perform_request(struct request_data *data);

/**
 * @brief Helper function to reliably send all bytes of a buffer over a socket.
 *
 * @param sock The connected socket descriptor.
 * @param buf Pointer to the data to send.
 * @param len Number of bytes to send.
 * @return The total number of bytes sent, or -1 on error.
 */
static int send_buffer_fully(int sock, const uint8_t *buf, size_t len);

/************************************************
 *       Callbacks definition/declaration       *
 ***********************************************/

/**
 * @brief Internal Zephyr callback invoked when an HTTP response chunk is received.
 *
 * @param rsp Pointer to the HTTP response structure.
 * @param final_data Enum indicating if this is the final chunk of data.
 * @param user_data Pointer to the user-provided context.
 *
 * @return 0 on success, or a negative value on error or if aborted by the user.
 */
static int get_response_cbk(
    struct http_response *rsp, enum http_final_call final_data, void *user_data)
{
    if (!user_data) {
        EDGEHOG_LOG_ERR("Unable to read user data context");
        return -1;
    }

    struct request_cbk_ctx *ctx = (struct request_cbk_ctx *) user_data;

    // Evaluate the status code if it has been parsed
    if ((rsp->http_status_code < HTTP_200_OK)
        || (rsp->http_status_code >= HTTP_300_MULTIPLE_CHOICES)) {
        EDGEHOG_LOG_ERR(
            "HTTP request failed, response code: %s (%d)", rsp->http_status, rsp->http_status_code);
        ctx->result = EDGEHOG_RESULT_HTTP_REQUEST_ERROR;
        return -1;
    }

    edgehog_http_response_chunk_t http_response_chunk = { 0 };
    if (rsp->body_found) {
        http_response_chunk.chunk_start_addr = rsp->body_frag_start;
        http_response_chunk.chunk_size = rsp->body_frag_len;
    } else {
        EDGEHOG_LOG_DBG("Empty body found in HTTP response");
    }
    http_response_chunk.response_size = rsp->content_length;
    http_response_chunk.last_chunk = final_data == HTTP_DATA_FINAL;

    ctx->result = ctx->response_cbk(&http_response_chunk, ctx->user_data);
    if (ctx->result != EDGEHOG_RESULT_OK) {
        EDGEHOG_LOG_ERR("HTTP response user callback error: %d", ctx->result);
        return -1;
    }
    return 0;
}

static int put_response_cbk(
    struct http_response * /*rsp*/, enum http_final_call /*final_data*/, void * /*user_data*/)
{
    // Zephyr requires a response callback.
    return 0;
}

/**
 * @brief Internal Zephyr callback invoked to fetch and send HTTP payload chunks during an upload.
 *
 * @param sock The connected socket descriptor.
 * @param req Pointer to the HTTP request structure.
 * @param user_data Pointer to the user-provided context.
 *
 * @return The total number of bytes sent, or a negative value on error.
 */
static int put_payload_cbk(int sock, struct http_request * /*req*/, void *user_data)
{
    if (!user_data) {
        EDGEHOG_LOG_ERR("Unable to read user data context");
        return -1;
    }

    struct request_cbk_ctx *ctx = (struct request_cbk_ctx *) user_data;
    int total_sent_bytes = 0;
    edgehog_http_payload_chunk_t http_payload_chunk = { 0 };

    while (!http_payload_chunk.last_chunk) {
        // Clear the previous chunk information
        memset(&http_payload_chunk, 0, sizeof(http_payload_chunk));

        // Get the next chunk to upload from the user callback
        ctx->result = ctx->payload_cbk(&http_payload_chunk, ctx->user_data);
        if (ctx->result != EDGEHOG_RESULT_OK) {
            EDGEHOG_LOG_ERR("HTTP payload user callback error: %d", ctx->result);
            return -1;
        }

        // Format and send the chunk to the server
        if (http_payload_chunk.chunk_size > 0) {
            int sent_bytes = 0;
            char buffer[HTTP_CHUNKED_PAYLOAD_CHUNK_LENGTH_BUFFER_SIZE] = { 0 };
            const char crlf[] = "\r\n";

            // Format chunk header
            int snprintf_rc = snprintf(
                buffer, ARRAY_SIZE(buffer), "%zx%s", http_payload_chunk.chunk_size, crlf);
            if (snprintf_rc < 0 || snprintf_rc >= ARRAY_SIZE(buffer)) {
                EDGEHOG_LOG_ERR("Failed to format chunk header: %d", snprintf_rc);
                ctx->result = EDGEHOG_RESULT_HTTP_REQUEST_ERROR;
                return -1;
            }

            // Send header
            sent_bytes = send_buffer_fully(sock, buffer, snprintf_rc);
            if (sent_bytes < 0) {
                EDGEHOG_LOG_ERR("Failed to send chunk header: %d", sent_bytes);
                ctx->result = EDGEHOG_RESULT_HTTP_REQUEST_ERROR;
                return -1;
            }
            total_sent_bytes += sent_bytes;
            // Send payload
            sent_bytes = send_buffer_fully(
                sock, http_payload_chunk.chunk_start_addr, http_payload_chunk.chunk_size);
            if (sent_bytes < 0) {
                EDGEHOG_LOG_ERR("Failed to send chunk payload: %d", sent_bytes);
                ctx->result = EDGEHOG_RESULT_HTTP_REQUEST_ERROR;
                return -1;
            }
            total_sent_bytes += sent_bytes;
            // Send trailing CRLF
            sent_bytes = send_buffer_fully(sock, crlf, sizeof(crlf) - 1);
            if (sent_bytes < 0) {
                EDGEHOG_LOG_ERR("Failed to send trailing CRLF: %d", sent_bytes);
                ctx->result = EDGEHOG_RESULT_HTTP_REQUEST_ERROR;
                return -1;
            }
            total_sent_bytes += sent_bytes;
            EDGEHOG_LOG_DBG("Sent chunk of size %zu bytes", http_payload_chunk.chunk_size);
        }
    }

    // Send the terminating sequence
    const char terminating_sequence[] = "0\r\n\r\n";
    if (send_buffer_fully(sock, terminating_sequence, sizeof(terminating_sequence) - 1) < 0) {
        ctx->result = EDGEHOG_RESULT_HTTP_REQUEST_ERROR;
        return -1;
    }
    total_sent_bytes += sizeof(terminating_sequence) - 1;
    EDGEHOG_LOG_DBG("Sent chunks terminating sequence");

    return total_sent_bytes;
}

/************************************************
 *         Global functions definitions         *
 ***********************************************/

edgehog_result_t edgehog_http_get(edgehog_http_get_data_t *data)
{
    struct request_data req_data = {
        .method = HTTP_GET,
        .url = data->url,
        .header_fields = data->header_fields,
        .timeout_ms = data->timeout_ms,
        .payload_cbk = NULL,
        .response_cbk = get_response_cbk,
        .cbk_ctx =
            {
                .result = EDGEHOG_RESULT_OK,
                .payload_cbk = NULL,
                .response_cbk = data->response_cbk,
                .user_data = data->user_data,
            },
    };
    return perform_request(&req_data);
}

edgehog_result_t edgehog_http_put(edgehog_http_put_data_t *data)
{
    size_t user_header_fields_number = 0;
    char **header_fields = NULL;

    // Search for any "Transfer-Encoding: chunked" header in the provided headers, or
    // "Content-Length: " header.
    if (data->header_fields) {
        for (size_t i = 0; data->header_fields[i] != NULL; i++) {
            const char *header = data->header_fields[i];

            // Check for "Transfer-Encoding:" or "Content-Length:" headers (case-insensitive)
            // If found, return immediately with an error to prevent conflicting upload directives.
            char transfer_encoding[] = "Transfer-Encoding:";
            size_t transfer_encoding_len = sizeof(transfer_encoding) - 1;
            char content_length[] = "Content-Length:";
            size_t content_length_len = sizeof(content_length) - 1;
            if (strncmp(header, transfer_encoding, transfer_encoding_len) == 0
                || strncmp(header, content_length, content_length_len) == 0) {

                EDGEHOG_LOG_ERR("Conflicting header provided by user: %s", header);
                return EDGEHOG_RESULT_HTTP_REQUEST_INVALID_HEADERS;
            }

            user_header_fields_number++;
        }
    }

    // +2 for the "Transfer-Encoding: chunked" header and the NULL terminator
    size_t header_fields_size = user_header_fields_number + 2;
    header_fields = (char **) k_malloc(header_fields_size * sizeof(char *));
    if (header_fields == NULL) {
        EDGEHOG_LOG_ERR("Failed to allocate memory for header fields");
        return EDGEHOG_RESULT_OUT_OF_MEMORY;
    }
    memset((void *) header_fields, 0, header_fields_size * sizeof(char *));

    // Copy user provided headers to the new header fields array
    for (size_t i = 0; i < user_header_fields_number; i++) {
        header_fields[i] = (char *) data->header_fields[i];
    }
    // Add the "Transfer-Encoding: chunked" header
    header_fields[user_header_fields_number] = "Transfer-Encoding: chunked\r\n";
    // The last element of the array is already set to NULL by memset

    struct request_data req_data = {
        .method = HTTP_PUT,
        .url = data->url,
        .header_fields = (const char **) header_fields,
        .timeout_ms = data->timeout_ms,
        .payload_cbk = put_payload_cbk,
        .response_cbk = put_response_cbk,
        .cbk_ctx =
            {
                .result = EDGEHOG_RESULT_OK,
                .payload_cbk = data->payload_cbk,
                .response_cbk = NULL,
                .user_data = data->user_data,
            },
    };

    edgehog_result_t result = perform_request(&req_data);
    k_free((void *) header_fields);
    return result;
}

/************************************************
 *         Static functions definitions         *
 ***********************************************/

static int create_and_connect_socket(const char *hostname, const char *port)
{
    struct zsock_addrinfo hints = { 0 };
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    struct zsock_addrinfo *host_addrinfo = NULL;
    int getaddrinfo_rc = zsock_getaddrinfo(hostname, port, &hints, &host_addrinfo);
    if (getaddrinfo_rc != 0) {
        EDGEHOG_LOG_ERR("Unable to resolve address %s", zsock_gai_strerror(getaddrinfo_rc));
        if (getaddrinfo_rc == DNS_EAI_SYSTEM) {
            EDGEHOG_LOG_ERR("Errno: (%d) %s", errno, strerror(errno));
        }
        return -1;
    }

#ifdef CONFIG_EDGEHOG_DEVICE_DEVELOP_USE_NON_TLS_HTTP
    int proto = IPPROTO_TCP;
#else
    int proto = IPPROTO_TLS_1_2;
#endif

    int sock = -1;
    struct zsock_addrinfo *curr_addr = NULL;

    // Iterate through the linked list of resolved addresses
    for (curr_addr = host_addrinfo; curr_addr != NULL; curr_addr = curr_addr->ai_next) {
        sock = zsock_socket(curr_addr->ai_family, curr_addr->ai_socktype, proto);
        if (sock == -1) {
            EDGEHOG_LOG_DBG("Socket creation failed, trying next address...");
            continue;
        }

#ifndef CONFIG_EDGEHOG_DEVICE_DEVELOP_USE_NON_TLS_HTTP
        // TODO: Evaluate what happens if one of the two tags is not found,
        // currently we are assuming both will be always present if TLS is enabled.
        sec_tag_t sec_tag_opt[] = {
            CONFIG_EDGEHOG_DEVICE_OTA_HTTPS_CA_CERT_TAG,
            CONFIG_EDGEHOG_DEVICE_FILE_TRANSFER_HTTPS_CA_CERT_TAG,
        };
        int sockopt_rc
            = zsock_setsockopt(sock, SOL_TLS, TLS_SEC_TAG_LIST, sec_tag_opt, sizeof(sec_tag_opt));
        if (sockopt_rc == -1) {
            EDGEHOG_LOG_ERR("Socket options error (TLS_SEC_TAG_LIST): %d", sockopt_rc);
            zsock_close(sock);
            sock = -1;
            continue;
        }

        sockopt_rc = zsock_setsockopt(sock, SOL_TLS, TLS_HOSTNAME, hostname, strlen(hostname));
        if (sockopt_rc == -1) {
            EDGEHOG_LOG_ERR("Socket options error (TLS_HOSTNAME): %d", sockopt_rc);
            zsock_close(sock);
            sock = -1;
            continue;
        }
#endif

        int connect_rc = zsock_connect(sock, curr_addr->ai_addr, curr_addr->ai_addrlen);
        if (connect_rc == -1) {
            EDGEHOG_LOG_DBG(
                "Connection failed (%d -  %s), trying next address...", errno, strerror(errno));
            zsock_close(sock);
            sock = -1;
            continue;
        }

        // If we reach here, we have successfully connected
        break;
    }

    // Free the linked list after we are done iterating
    zsock_freeaddrinfo(host_addrinfo);

    // Check if we exhausted the list without a successful connection
    if (sock == -1) {
        EDGEHOG_LOG_ERR("Failed to connect to any resolved address.");
    }

    return sock;
}

static edgehog_result_t perform_request(struct request_data *data)
{
    // Create and connect the socket to use
    struct http_parser_url parser;
    http_parser_url_init(&parser);
    int ret = http_parser_parse_url(data->url, strlen(data->url), 0, &parser);

    if (ret < 0) {
        EDGEHOG_LOG_ERR("Invalid http request URL: %s %d", data->url, ret);
        return EDGEHOG_RESULT_PARSE_URL_ERROR;
    }

    uint16_t host_off = parser.field_data[UF_HOST].off;
    uint16_t host_len = parser.field_data[UF_HOST].len;

    char host[host_len + 1];
    ret = snprintf(host, host_len + 1, "%s", data->url + host_off);
    if (ret < 0) {
        EDGEHOG_LOG_ERR("Error extracting hostname from URL");
        return EDGEHOG_RESULT_PARSE_URL_ERROR;
    }

    char port[PORT_STR_LEN] = { 0 };
    if (parser.port == 0) {
        strncpy(port, strncmp("https", data->url, HTTPS_STR_LEN - 1) == 0 ? "443" : "80",
            PORT_STR_LEN - 1);
    } else {
        ret = sprintf(port, "%u", parser.port);
        if (ret < 0) {
            EDGEHOG_LOG_ERR("Error extracting port from URL");
            return EDGEHOG_RESULT_PARSE_URL_ERROR;
        }
    }

    int sock = create_and_connect_socket(host, port);
    if (sock < 0) {
        EDGEHOG_LOG_ERR("socket err %d", sock);
        return EDGEHOG_RESULT_NETWORK_ERROR;
    }

    uint16_t path_off = parser.field_data[UF_PATH].off;
    uint16_t path_len = parser.field_data[UF_PATH].len;

    char path[MAX(path_len + 1, 2)];
    if (path_len == 0) {
        path[0] = '/';
        path[1] = '\0';
    } else {
        ret = snprintf(path, path_len + 1, "%s", data->url + path_off);
        if (ret < 0) {
            EDGEHOG_LOG_ERR("Error extracting path from URL");
            zsock_close(sock);
            return EDGEHOG_RESULT_PARSE_URL_ERROR;
        }
    }

    // Perform the HTTP request and wait for the response
    uint8_t *recv_buf = k_malloc(CONFIG_EDGEHOG_DEVICE_ADVANCED_HTTP_RCV_BUFFER_SIZE);
    if (recv_buf == NULL) {
        EDGEHOG_LOG_ERR("Failed to allocate memory for recv_buf");
        zsock_close(sock);
        return EDGEHOG_RESULT_OUT_OF_MEMORY;
    }
    memset(recv_buf, 0, CONFIG_EDGEHOG_DEVICE_ADVANCED_HTTP_RCV_BUFFER_SIZE);

    struct http_request req = { 0 };
    req.method = data->method;
    req.host = host;
    req.port = port;
    req.url = path;
    req.header_fields = data->header_fields;
    req.protocol = "HTTP/1.1";
    if (data->payload_cbk) {
        req.content_type_value = "application/octet-stream";
        req.payload_cb = data->payload_cbk;
    }
    req.response = data->response_cbk;
    req.recv_buf = recv_buf;
    req.recv_buf_len = CONFIG_EDGEHOG_DEVICE_ADVANCED_HTTP_RCV_BUFFER_SIZE;

    // Pass context struct as the user_data parameter
    int http_rc = http_client_req(sock, &req, data->timeout_ms, &data->cbk_ctx);
    if (http_rc < 0) {
        EDGEHOG_LOG_ERR("HTTP request failed, http error: %d", http_rc);
        zsock_close(sock);
        k_free(recv_buf);
        return EDGEHOG_RESULT_HTTP_REQUEST_ERROR;
    }

    if (data->cbk_ctx.result != EDGEHOG_RESULT_OK) {
        EDGEHOG_LOG_ERR("HTTP request failed, edgehog error: %d", data->cbk_ctx.result);
        zsock_close(sock);
        k_free(recv_buf);
        return data->cbk_ctx.result;
    }

    zsock_close(sock);
    k_free(recv_buf);
    return EDGEHOG_RESULT_OK;
}

static int send_buffer_fully(int sock, const uint8_t *buf, size_t len)
{
    int sent_bytes = 0;
    while (sent_bytes < len) {
        int send_rc = zsock_send(sock, buf + sent_bytes, len - sent_bytes, 0);
        if (send_rc <= 0) {
            EDGEHOG_LOG_ERR("Failed to send socket data: %d", errno);
            return -1;
        }
        sent_bytes += send_rc;
    }

    return sent_bytes;
}
