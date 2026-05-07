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

#if !defined(CONFIG_EDGEHOG_DEVICE_DEVELOP_USE_NON_TLS_HTTP)
#include <zephyr/net/tls_credentials.h>
#endif

#include <stdio.h>
#include <string.h>

#include "log.h"
EDGEHOG_LOG_MODULE_REGISTER(edgehog_http, CONFIG_EDGEHOG_DEVICE_HTTP_LOG_LEVEL);

/************************************************
 *        Defines, constants and typedef        *
 ***********************************************/

// TODO: Move this in the kconfig
#define CONFIG_EDGEHOG_DEVICE_ADVANCED_HTTP_RCV_BUFFER_SIZE 1024

/** @brief Context struct for any HTTP request */
struct http_req_ctx
{
    /** @brief Result to store the success or failure of the request */
    edgehog_result_t request_result;
    /** @brief Callback for a chunk download event. */
    http_download_payload_cbk_t download_cbk;
    /** @brief Abort flag, user settable. */
    bool abort_flag;
    /** @brief User data passed to http_download_cb callback function. */
    void *user_data;
};

#define PORT_STR_LEN 6
#define HTTPS_STR "https"
#define HTTPS_STR_LEN sizeof(HTTPS_STR)

/************************************************
 *       Callbacks definition/declaration       *
 ***********************************************/

static int http_download_cb(
    struct http_response *rsp, enum http_final_call final_data, void *user_data)
{
    int res = 0;

    if (!user_data) {
        EDGEHOG_LOG_ERR("Unable to read user data context");
        res = -1;
        goto exit;
    }

    struct http_req_ctx *ctx = (struct http_req_ctx *) user_data;

    if (ctx->abort_flag) {
        EDGEHOG_LOG_WRN("Download aborted by user.");
        ctx->request_result = EDGEHOG_RESULT_HTTP_REQUEST_ABORTED;
        return -1;
    }

    // Evaluate the status code if it has been parsed
    if ((rsp->http_status_code < HTTP_200_OK)
        || (rsp->http_status_code >= HTTP_300_MULTIPLE_CHOICES)) {
        EDGEHOG_LOG_ERR(
            "HTTP request failed, response code: %s %d", rsp->http_status, rsp->http_status_code);
        ctx->request_result = EDGEHOG_RESULT_HTTP_REQUEST_ERROR;
        res = -1;
        goto exit;
    }

    // Evaluate if body is found
    if (!rsp->body_found) {
        EDGEHOG_LOG_DBG("Empty body, skipping http download callback");
        goto exit;
    }

    http_download_chunk_t http_download_chunk = { 0 };
    http_download_chunk.chunk_start_addr = rsp->body_frag_start;
    http_download_chunk.chunk_size = rsp->body_frag_len;
    http_download_chunk.download_size = rsp->content_length;
    http_download_chunk.last_chunk = final_data == HTTP_DATA_FINAL;

    ctx->request_result = ctx->download_cbk(&ctx->abort_flag, &http_download_chunk, ctx->user_data);

exit:
    return res;
}

/************************************************
 *         Static functions declaration         *
 ***********************************************/

/**
 * @brief Create a new TCP socket and connect it to a server.
 *
 * @param[in] host domain name, a string representation of an IPv4.
 * @param[in] port service port, a striing representation of HTTP service port.
 *
 * @note The returned socket should be closed once its use has terminated.
 *
 * @return -1 upon failure, a file descriptor for the new socket otherwise.
 */
static int create_and_connect_socket(const char *host, const char *port);

/************************************************
 *         Global functions definitions         *
 ***********************************************/

edgehog_result_t edgehog_http_download(
    const char *url, const char **header_fields, int32_t timeout_ms, http_download_t *http_download)
{
    // Create and connect the socket to use
    struct http_parser_url parser;
    http_parser_url_init(&parser);
    int ret = http_parser_parse_url(url, strlen(url), 0, &parser);

    if (ret < 0) {
        EDGEHOG_LOG_ERR("Invalid http request url: %s %d", url, ret);
        return EDGEHOG_RESULT_INVALID_PARAM;
    }

    uint16_t host_off = parser.field_data[UF_HOST].off;
    uint16_t host_len = parser.field_data[UF_HOST].len;

    char host[host_len + 1];
    ret = snprintf(host, host_len + 1, "%s", url + host_off);
    if (ret < 0) {
        EDGEHOG_LOG_ERR("Error extracting hostname from url");
        return EDGEHOG_RESULT_INTERNAL_ERROR;
    }

    char port[PORT_STR_LEN] = { 0 };
    if (parser.port == 0) {
        strncpy(
            port, strncmp("https", url, HTTPS_STR_LEN - 1) == 0 ? "443" : "80", PORT_STR_LEN - 1);
    } else {
        ret = sprintf(port, "%u", parser.port);
        if (ret < 0) {
            EDGEHOG_LOG_ERR("Error extracting port from url");
            return EDGEHOG_RESULT_INTERNAL_ERROR;
        }
    }

    int sock_id = create_and_connect_socket(host, port);
    if (sock_id < 0) {
        EDGEHOG_LOG_ERR("socket err %d \n", sock_id);
        return EDGEHOG_RESULT_NETWORK_ERROR;
    }

    uint16_t path_off = parser.field_data[UF_PATH].off;
    uint16_t path_len = parser.field_data[UF_PATH].len;

    char path[MAX(path_len + 1, 2)];
    if (path_len == 0) {
        path[0] = '/';
        path[1] = '\0';
    } else {
        ret = snprintf(path, path_len + 1, "%s", url + path_off);
        if (ret < 0) {
            EDGEHOG_LOG_ERR("Error extracting path from url");
            zsock_close(sock_id);
            return EDGEHOG_RESULT_INTERNAL_ERROR;
        }
    }

    struct http_req_ctx ctx = { .request_result = EDGEHOG_RESULT_OK,
        .download_cbk = http_download->download_cbk,
        .abort_flag = false,
        .user_data = http_download->user_data };

    struct http_request req = { 0 };
    // TODO: This buffer is still allocated on the stack. Consider moving this to the heap to avoid
    // the stack overflow risk.
    uint8_t recv_buf[CONFIG_EDGEHOG_DEVICE_ADVANCED_HTTP_RCV_BUFFER_SIZE];
    memset(recv_buf, 0, sizeof(recv_buf));

    req.method = HTTP_GET;
    req.host = host;
    req.port = port;
    req.url = path;
    req.header_fields = header_fields;
    req.protocol = "HTTP/1.1";
    req.response = http_download_cb;
    req.content_type_value = "application/octet-stream";
    req.recv_buf = recv_buf;
    req.recv_buf_len = sizeof(recv_buf);

    int http_rc = http_client_req(sock_id, &req, timeout_ms, &ctx);

    if (http_rc < 0) {
        EDGEHOG_LOG_ERR("HTTP request failed, http error: %d", http_rc);
        zsock_close(sock_id);
        return EDGEHOG_RESULT_HTTP_REQUEST_ERROR;
    }

    if (ctx.request_result != EDGEHOG_RESULT_OK) {
        EDGEHOG_LOG_ERR("HTTP request failed, edgehog error: %d", ctx.request_result);
        zsock_close(sock_id);
        return ctx.request_result;
    }

    zsock_close(sock_id);
    return EDGEHOG_RESULT_OK;
}

void edgehog_http_download_abort(bool *abort_flag)
{
    *abort_flag = true;
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

#if defined(CONFIG_EDGEHOG_DEVICE_DEVELOP_USE_NON_TLS_HTTP)
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

#if !defined(CONFIG_EDGEHOG_DEVICE_DEVELOP_USE_NON_TLS_HTTP)
        // TODO: Evaluate what happens if one of the two tags is not found,
        // currently we are assuming both will be always present if TLS is enabled.
        sec_tag_t sec_tag_opt[] = {
            CONFIG_EDGEHOG_DEVICE_OTA_HTTPS_CA_CERT_TAG,
            CONFIG_EDGEHOG_DEVICE_FT_HTTPS_CA_CERT_TAG,
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
