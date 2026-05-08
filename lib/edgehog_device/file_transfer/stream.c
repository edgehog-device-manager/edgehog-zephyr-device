/*
 * (C) Copyright 2026, SECO Mind Srl
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "file_transfer/stream.h"

#include "edgehog_device/file_transfer.h"

#include <stdlib.h>
#include <string.h>
#include <zephyr/kernel.h>

#include "log.h"

EDGEHOG_LOG_MODULE_REGISTER(file_transfer_stream, CONFIG_EDGEHOG_DEVICE_FILE_TRANSFER_LOG_LEVEL);

/************************************************
 *        Defines, constants and typedef        *
 ***********************************************/

/* The timeout for pipe operations. This assumes a reasonable delay for pipe operations. */
#define PIPE_TIMEOUT_MS 2000
/* The timeout for event operations. This assumes a reasonable delay for event operations. */
#define EVENT_TIMEOUT_MS 2000
/* Define an internal buffer size for chunks read from the pipe during uploads */
#define READ_BUFFER_SIZE 1024
/* Buffer size for the dynamically allocated stream pipe */
#define STREAM_PIPE_BUFFER_SIZE 1024
/* Simple macro for 100% */
#define ONE_HUNDRED_PERCENT 100

/** @brief Context structure for stream write operations. */
typedef struct
{
    /** @brief Zephyr pipe used for writing stream data. */
    struct k_pipe pipe;
    /** @brief Internal buffer backing the write pipe. */
    uint8_t __aligned(4) pipe_buffer[STREAM_PIPE_BUFFER_SIZE];
    /** @brief Event used to signal the end of the file/stream. */
    struct k_event eof_event;
    /** @brief Expected total size of the file being written. */
    size_t total_size;
    /** @brief Number of bytes successfully transferred so far. */
    size_t transferred_size;
} write_ctx_t;

/** @brief Context structure for stream read operations. */
typedef struct
{
    /** @brief Zephyr pipe used for reading stream data. */
    struct k_pipe pipe;
    /** @brief Internal buffer backing the read pipe. */
    uint8_t pipe_buffer[STREAM_PIPE_BUFFER_SIZE];
    /** @brief Event used to signal the end of the file/stream. */
    struct k_event eof_event;
    /** @brief Buffer used to read chunks of data from the pipe. */
    uint8_t read_buffer[READ_BUFFER_SIZE];
} read_ctx_t;

/************************************************
 *         Static functions declarations        *
 ***********************************************/

static edgehog_result_t write_init(void **ctx, edgehog_ft_cbks_t *cbks, char *identifier, char *url,
    size_t expected_file_size, char *destination);
static edgehog_result_t write_append(void *ctx, const uint8_t *chunk_data, size_t chunk_size);
static edgehog_result_t write_complete(void *ctx);
static void write_abort(void *ctx);

static edgehog_result_t read_init(
    void **ctx, edgehog_ft_cbks_t *cbks, char *identifier, char *source);
static edgehog_result_t read_chunk(
    void *ctx, size_t max_length, uint8_t **chunk_data, size_t *chunk_size, bool *last_chunk);
static edgehog_result_t read_complete(void *ctx);
static void read_abort(void *ctx);

/************************************************
 *         Global variables definitions         *
 ***********************************************/

const edgehog_ft_file_write_cbks_t edgehog_ft_stream_write_cbks = { .file_init = write_init,
    .file_append_chunk = write_append,
    .file_complete = write_complete,
    .file_abort = write_abort };
const edgehog_ft_file_read_cbks_t edgehog_ft_stream_read_cbks = { .file_init = read_init,
    .file_read_chunk = read_chunk,
    .file_complete = read_complete,
    .file_abort = read_abort };

/************************************************
 *         Static functions definitions         *
 ***********************************************/

static edgehog_result_t write_init(void **ctx, edgehog_ft_cbks_t *cbks, char * /*identifier*/,
    char * /*url*/, size_t expected_file_size, char *destination)
{
    write_ctx_t *wctx = k_malloc(sizeof(write_ctx_t));
    if (!wctx) {
        EDGEHOG_LOG_ERR("Out of memory %s: %d", __FILE__, __LINE__);
        return EDGEHOG_RESULT_OUT_OF_MEMORY;
    }

    k_pipe_init(&wctx->pipe, wctx->pipe_buffer, STREAM_PIPE_BUFFER_SIZE);
    k_event_init(&wctx->eof_event);

    // Check if callbacks are valid
    if (!cbks || !cbks->on_stream_transfer_start) {
        EDGEHOG_LOG_ERR("Invalid callbacks provided for stream write");
        k_free(wctx);
        return EDGEHOG_RESULT_INVALID_PARAM;
    }

    // TODO: evaluate if destination might need some parsing or validation.

    edgehog_ft_stream_t stream = { .pipe = &wctx->pipe, .event = &wctx->eof_event };

    // Trigger the callback to notify the app and provide resources
    bool accepted = cbks->on_stream_transfer_start(
        destination, EDGEHOG_FT_TYPE_SERVER_TO_DEVICE, expected_file_size, &stream);
    if (!accepted) {
        EDGEHOG_LOG_ERR("File transfer rejected for: %s", destination);
        k_free(wctx);
        return EDGEHOG_RESULT_INVALID_PARAM;
    }

    wctx->total_size = expected_file_size;
    wctx->transferred_size = 0;

    *ctx = wctx;
    return EDGEHOG_RESULT_OK;
}

static edgehog_result_t write_append(void *ctx, const uint8_t *chunk_data, size_t chunk_size)
{
    write_ctx_t *wctx = (write_ctx_t *) ctx;
    int64_t start_time = k_uptime_get();
    size_t total_written = 0;

    // Loop until the entire chunk is written
    while (total_written < chunk_size) {

        // Check if the application signaled an error before we attempt to write
        uint32_t events = k_event_test(&wctx->eof_event, EDGEHOG_FT_STREAM_ERROR_EVENT_FLAG);
        if (events & EDGEHOG_FT_STREAM_ERROR_EVENT_FLAG) {
            EDGEHOG_LOG_ERR("Application signaled an error during stream write");
            return EDGEHOG_RESULT_INTERNAL_ERROR;
        }

        // Try to write data without blocking
        size_t remaining = chunk_size - total_written;
        int ret = k_pipe_write(&wctx->pipe, chunk_data + total_written, remaining, K_NO_WAIT);
        if (ret > 0) {
            total_written += (size_t) ret;
            // Reset timeout timer since we made progress
            start_time = k_uptime_get();
        } else if (ret < 0 && ret != -EAGAIN) {
            EDGEHOG_LOG_ERR("Error writing to pipe: %d", ret);
            return EDGEHOG_RESULT_INTERNAL_ERROR;
        }

        // Timeout check
        if ((k_uptime_get() - start_time) >= PIPE_TIMEOUT_MS) {
            EDGEHOG_LOG_ERR("Timeout writing to pipe - user application is too slow to read");
            return EDGEHOG_RESULT_INTERNAL_ERROR;
        }

        // Yield briefly if we still have data to write
        if (total_written < chunk_size) {
            k_sleep(K_MSEC(10));
        }
    }

    wctx->transferred_size += total_written;
    return EDGEHOG_RESULT_OK;
}

static edgehog_result_t write_complete(void *ctx)
{
    EDGEHOG_LOG_DBG("File write has been completed.");
    write_ctx_t *wctx = (write_ctx_t *) ctx;

    // Post the EOF flag to the event object
    k_event_post(&wctx->eof_event, EDGEHOG_FT_STREAM_EOF_EVENT_FLAG);

    // Synchronize teardown: wait for the app thread to acknowledge finishing reading
    k_event_wait(
        &wctx->eof_event, EDGEHOG_FT_STREAM_ACK_EVENT_FLAG, false, K_MSEC(EVENT_TIMEOUT_MS));

    k_free(ctx);
    return EDGEHOG_RESULT_OK;
}

static void write_abort(void *ctx)
{
    EDGEHOG_LOG_DBG("File write has been aborted.");
    write_ctx_t *wctx = (write_ctx_t *) ctx;

    k_event_post(&wctx->eof_event, EDGEHOG_FT_STREAM_EOF_EVENT_FLAG);
    k_event_wait(
        &wctx->eof_event, EDGEHOG_FT_STREAM_ACK_EVENT_FLAG, false, K_MSEC(EVENT_TIMEOUT_MS));

    k_free(ctx);
}

static edgehog_result_t read_init(
    void **ctx, edgehog_ft_cbks_t *cbks, char * /*identifier*/, char *source)
{
    read_ctx_t *rctx = k_malloc(sizeof(read_ctx_t));
    if (!rctx) {
        return EDGEHOG_RESULT_OUT_OF_MEMORY;
    }

    k_pipe_init(&rctx->pipe, rctx->pipe_buffer, STREAM_PIPE_BUFFER_SIZE);
    k_event_init(&rctx->eof_event);

    if (!cbks || !cbks->on_stream_transfer_start) {
        EDGEHOG_LOG_ERR("Invalid callbacks provided for stream read");
        k_free(rctx);
        return EDGEHOG_RESULT_INVALID_PARAM;
    }

    // TODO: evaluate if source might need some parsing or validation.

    edgehog_ft_stream_t stream = { .pipe = &rctx->pipe, .event = &rctx->eof_event };

    bool accepted
        = cbks->on_stream_transfer_start(source, EDGEHOG_FT_TYPE_DEVICE_TO_SERVER, 0, &stream);

    if (!accepted) {
        EDGEHOG_LOG_ERR("Transfer rejected for: %s", source);
        k_free(rctx);
        return EDGEHOG_RESULT_INVALID_PARAM;
    }

    *ctx = rctx;
    return EDGEHOG_RESULT_OK;
}

static edgehog_result_t read_chunk(
    void *ctx, size_t max_length, uint8_t **chunk_data, size_t *chunk_size, bool *last_chunk)
{
    read_ctx_t *rctx = (read_ctx_t *) ctx;
    int64_t start_time = k_uptime_get();

    // Loop until we read data, encounter a hard error, or detect EOF
    while (true) {
        // Try to read data from the pipe without blocking
        size_t bytes_to_read = MIN(READ_BUFFER_SIZE, max_length);
        int ret = k_pipe_read(&rctx->pipe, rctx->read_buffer, bytes_to_read, K_NO_WAIT);
        if (ret > 0) {
            *chunk_data = rctx->read_buffer;
            *chunk_size = (size_t) ret;
            *last_chunk = false;
            return EDGEHOG_RESULT_OK;
        }

        if (ret < 0 && ret != -EAGAIN) {
            EDGEHOG_LOG_ERR("Error reading from pipe: %d", ret);
            return EDGEHOG_RESULT_INTERNAL_ERROR;
        }

        // The pipe is empty (-EAGAIN). Check if the writer signaled something.
        uint32_t events = k_event_test(&rctx->eof_event,
            EDGEHOG_FT_STREAM_EOF_EVENT_FLAG | EDGEHOG_FT_STREAM_ERROR_EVENT_FLAG);
        if (events & EDGEHOG_FT_STREAM_ERROR_EVENT_FLAG) {
            EDGEHOG_LOG_ERR("Application signaled an error during stream read");
            return EDGEHOG_RESULT_INTERNAL_ERROR;
        }
        if (events & EDGEHOG_FT_STREAM_EOF_EVENT_FLAG) {
            *chunk_data = NULL;
            *chunk_size = 0;
            *last_chunk = true;
            return EDGEHOG_RESULT_OK;
        }

        // Check for timeout to prevent deadlocks
        if ((k_uptime_get() - start_time) >= PIPE_TIMEOUT_MS) {
            EDGEHOG_LOG_ERR("Timeout reading from pipe - user application is too slow to write");
            return EDGEHOG_RESULT_INTERNAL_ERROR;
        }

        // Sleep briefly to prevent busy-looping and starving lower-priority threads.
        k_sleep(K_MSEC(10));
    }

    return EDGEHOG_RESULT_OK;
}

static edgehog_result_t read_complete(void *ctx)
{
    EDGEHOG_LOG_DBG("File read has been completed, freeing context.");
    k_free(ctx);
    return EDGEHOG_RESULT_OK;
}

static void read_abort(void *ctx)
{
    EDGEHOG_LOG_DBG("File read has been aborted.");
    k_free(ctx);
}
