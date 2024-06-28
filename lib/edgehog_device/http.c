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

#if !defined(CONFIG_EDGEHOG_DEVICE_DEVELOP_DISABLE_OR_IGNORE_TLS)
#include <zephyr/net/tls_credentials.h>
#endif

#include <stdio.h>

#include "log.h"
EDGEHOG_LOG_MODULE_REGISTER(edgehog_http, CONFIG_EDGEHOG_DEVICE_HTTP_LOG_LEVEL);

/************************************************
 *        Defines, constants and typedef        *
 ***********************************************/

#define PORT_STR_LEN 6
#define HTTPS_STR "https"
#define HTTPS_STR_LEN sizeof(HTTPS_STR)
#define HTTP_RECV_BUF_SIZE 4096
static uint8_t http_recv_buf[HTTP_RECV_BUF_SIZE];

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
 *       Callbacks definition/declaration       *
 ***********************************************/

static void http_download_cb(
    struct http_response *rsp, enum http_final_call final_data, void *user_data)
{
    if (!user_data) {
        EDGEHOG_LOG_ERR("Unable to read user data context");
        return;
    }

    http_download_t *http_download = (http_download_t *) user_data;

    if (rsp->http_status_code >= HTTP_400_BAD_REQUEST) {
        EDGEHOG_LOG_ERR("Unable to handle ota request, http status code %d -> %s",
            rsp->http_status_code, rsp->http_status);
        http_download->edegehog_result = EDGEHOG_RESULT_NETWORK_ERROR;
        return;
    }

    if (!rsp->body_found) {
        EDGEHOG_LOG_ERR("Unable to parse body, It is empty");
        return;
    }

    http_download_chunk_t http_download_chunk = { 0 };
    http_download_chunk.download_size = rsp->content_length;
    http_download_chunk.chunk_start_addr = rsp->body_frag_start;
    http_download_chunk.chunk_size = rsp->body_frag_len;
    http_download_chunk.last_chunk = final_data == HTTP_DATA_FINAL;

    http_download->edegehog_result = http_download->download_cbk(
        http_download->sock_id, &http_download_chunk, http_download->user_data);
}

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
        EDGEHOG_LOG_ERR("Invalid firmware url: %s %d", url, ret);
        return EDGEHOG_RESULT_NETWORK_ERROR;
    }

    uint16_t host_off = parser.field_data[UF_HOST].off;
    uint16_t host_len = parser.field_data[UF_HOST].len;

    char host[host_len + 1];
    ret = snprintf(host, host_len + 1, "%s", url + host_off);
    if (ret < 0) {
        EDGEHOG_LOG_ERR("Error extracting hostanme from url");
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

    http_download->sock_id = create_and_connect_socket(host, port);
    if (http_download->sock_id < 0) {
        EDGEHOG_LOG_ERR("socket err %d \n", http_download->sock_id);
        return EDGEHOG_RESULT_NETWORK_ERROR;
    }

    uint16_t path_off = parser.field_data[UF_PATH].off;
    uint16_t path_len = parser.field_data[UF_PATH].len;

    char path[path_len + 1];
    ret = snprintf(path, path_len + 1, "%s", url + path_off);
    if (ret < 0) {
        EDGEHOG_LOG_ERR("Error extracting path from url");
        return EDGEHOG_RESULT_INTERNAL_ERROR;
    }

    struct http_request req = { 0 };
    memset(&http_recv_buf, 0, sizeof(http_recv_buf));

    req.method = HTTP_GET;
    req.host = host;
    req.port = port;
    req.url = path;
    req.content_type_value = "application/octet-stream";
    req.header_fields = header_fields;
    req.protocol = "HTTP/1.1";
    req.response = http_download_cb;
    req.recv_buf = http_recv_buf;
    req.recv_buf_len = sizeof(http_recv_buf);

    int http_rc = http_client_req(http_download->sock_id, &req, timeout_ms, http_download);

    edgehog_result_t result = EDGEHOG_RESULT_OK;

    if (http_rc < 0) {
        EDGEHOG_LOG_ERR("HTTP request failed: %d", http_rc);
        EDGEHOG_LOG_WRN("Receive buffer content:\n%s", http_recv_buf);
        result = EDGEHOG_RESULT_NETWORK_ERROR;
    }

    if (http_download->edegehog_result != EDGEHOG_RESULT_OK) {
        result = http_download->edegehog_result;
    }

    // Close the socket
    zsock_close(http_download->sock_id);

    return result;
}

void edgehog_http_download_abort(int sock_id)
{
    zsock_close(sock_id);
}

/************************************************
 *         Static functions definitions         *
 ***********************************************/

static int create_and_connect_socket(const char *hostname, const char *port)
{
    static struct zsock_addrinfo hints = { 0 };
    struct zsock_addrinfo *host_addrinfo = NULL;

    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    int getaddrinfo_rc = zsock_getaddrinfo(hostname, port, &hints, &host_addrinfo);
    if (getaddrinfo_rc != 0) {
        EDGEHOG_LOG_ERR("Unable to resolve address %s", zsock_gai_strerror(getaddrinfo_rc));
        if (getaddrinfo_rc == DNS_EAI_SYSTEM) {
            EDGEHOG_LOG_ERR("Errno: %s", strerror(errno));
        }
        return -1;
    }

#if defined(CONFIG_EDGEHOG_DEVICE_DEVELOP_DISABLE_OR_IGNORE_TLS)
    int proto = IPPROTO_TCP;
#else
    int proto = IPPROTO_TLS_1_2;
#endif
    int sock = zsock_socket(host_addrinfo->ai_family, host_addrinfo->ai_socktype, proto);
    if (sock == -1) {
        EDGEHOG_LOG_ERR("Socket creation error: %d %s:%s", sock, hostname, port);
        EDGEHOG_LOG_ERR("Errno: %s", strerror(errno));
        zsock_freeaddrinfo(host_addrinfo);
        return -1;
    }

#if !defined(CONFIG_EDGEHOG_DEVICE_DEVELOP_DISABLE_OR_IGNORE_TLS)
    sec_tag_t sec_tag_opt[] = {
        CONFIG_EDGEHOG_DEVICE_CA_CERT_OTA_TAG,
    };
    int sockopt_rc
        = zsock_setsockopt(sock, SOL_TLS, TLS_SEC_TAG_LIST, sec_tag_opt, sizeof(sec_tag_opt));
    if (sockopt_rc == -1) {
        EDGEHOG_LOG_ERR("Socket options error: %d %s:%s", sock, hostname, port);
        EDGEHOG_LOG_ERR("Errno: %s", strerror(errno));
        goto fail;
    }

    sockopt_rc = zsock_setsockopt(sock, SOL_TLS, TLS_HOSTNAME, hostname, sizeof(hostname));
    if (sockopt_rc == -1) {
        EDGEHOG_LOG_ERR("Socket options error: %d", sockopt_rc);
        goto fail;
    }
#endif

    int connect_rc = zsock_connect(sock, host_addrinfo->ai_addr, host_addrinfo->ai_addrlen);
    if (connect_rc == -1) {
        EDGEHOG_LOG_ERR("Connection error: %d", connect_rc);
        EDGEHOG_LOG_ERR("Errno: %s\n", strerror(errno));
        goto fail;
    }

    zsock_freeaddrinfo(host_addrinfo);

    return sock;

fail:
    zsock_close(sock);
    zsock_freeaddrinfo(host_addrinfo);
    return -1;
}
