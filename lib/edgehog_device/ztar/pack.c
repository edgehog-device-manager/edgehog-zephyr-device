/*
 * (C) Copyright 2026, SECO Mind Srl
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ztar/pack.h"

#include <zephyr/sys/util.h>

#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "log.h"

EDGEHOG_LOG_MODULE_REGISTER(ztar_pack, CONFIG_EDGEHOG_DEVICE_ZTAR_LOG_LEVEL);

/************************************************
 *        Defines, constants and typedef        *
 ***********************************************/

// Default mode we will set for all the produced tar
#define FILE_MODE_DEFAULT 0644
// Checksum first termination (NULL)
#define CHKSUM_TERMINATOR_NULL_IDX 6
// Checksum second termination (space)
#define CHKSUM_TERMINATOR_SPACE_IDX 7

/************************************************
 *         Static functions declaration         *
 ***********************************************/

// Handles getting the next file metadata for packing
static ztar_result_t pack_handle_next_file(ztar_pack_t *stream);
// Handles formatting and outputting the tar header
static ztar_result_t pack_handle_header(
    ztar_pack_t *stream, uint8_t *out, size_t remaining, size_t *written);
// Handles reading and outputting the file data payload
static ztar_result_t pack_handle_data(
    ztar_pack_t *stream, uint8_t *out, size_t remaining, size_t *written);
// Handles writing zero-padding to reach 512-byte boundaries
static ztar_result_t pack_handle_padding(
    ztar_pack_t *stream, uint8_t *out, size_t remaining, size_t *written);
// Handles writing the end-of-archive zero blocks
static ztar_result_t pack_handle_trailer(
    ztar_pack_t *stream, uint8_t *out, size_t remaining, size_t *written);
// Helper function for packing octals safely into headers
static int pack_format_octal(uint64_t value, char *out, size_t out_size);

/************************************************
 *         Global functions definition          *
 ***********************************************/

ztar_result_t ztar_pack_init(ztar_pack_t *stream, ztar_pack_callbacks_t callbacks, void *user_data)
{
    if (!stream) {
        EDGEHOG_LOG_ERR("Called ztar_pack_init with null pointer");
        return ZTAR_RESULT_INVALID_ARGS;
    }
    if (!callbacks.get_next_file || !callbacks.read_file_data) {
        EDGEHOG_LOG_ERR("Missing callback for packing context");
        return ZTAR_RESULT_INVALID_ARGS;
    }
    EDGEHOG_LOG_DBG("Initializing ztar packing stream");

    memset(stream, 0, sizeof(ztar_pack_t));
    stream->initialized = true;
    stream->state = ZTAR_PACK_STATE_NEXT_FILE;
    stream->callbacks = callbacks;
    stream->user_data = user_data;

    return ZTAR_RESULT_OK;
}

bool ztar_pack_is_initialized(const ztar_pack_t *stream)
{
    if (!stream) {
        EDGEHOG_LOG_ERR("Called ztar_pack_is_initialized with null pointer");
        return false;
    }
    return stream->initialized;
}

ztar_result_t ztar_pack_read_stream(
    ztar_pack_t *stream, uint8_t *out_buffer, size_t out_size, size_t *bytes_written)
{
    if (!stream || !out_buffer || !bytes_written) {
        return ZTAR_RESULT_INVALID_ARGS;
    }
    if (!stream->initialized) {
        return ZTAR_RESULT_INVALID_ARGS;
    }
    if (out_size < sizeof(ztar_header_t)) {
        return ZTAR_RESULT_INSUFFICIENT_BUFF;
    }

    *bytes_written = 0;
    ztar_result_t res = ZTAR_RESULT_OK;

    // Loop to fill the buffer as much as possible
    while (*bytes_written < out_size && stream->state != ZTAR_PACK_STATE_DONE) {
        size_t remaining_out = out_size - *bytes_written;
        uint8_t *current_out = out_buffer + *bytes_written;
        size_t step_written = 0;

        switch (stream->state) {
            case ZTAR_PACK_STATE_NEXT_FILE:
                res = pack_handle_next_file(stream);
                break;
            case ZTAR_PACK_STATE_HEADER:
                // If we don't have at least sizeof(ztar_header_t) bytes left in the buffer, pause
                // and wait for the next call so we don't have to manage split headers across buffer
                // boundaries.
                if (remaining_out < sizeof(ztar_header_t)) {
                    return ZTAR_RESULT_OK;
                }
                res = pack_handle_header(stream, current_out, remaining_out, &step_written);
                break;
            case ZTAR_PACK_STATE_DATA:
                res = pack_handle_data(stream, current_out, remaining_out, &step_written);
                break;
            case ZTAR_PACK_STATE_PADDING:
                res = pack_handle_padding(stream, current_out, remaining_out, &step_written);
                break;
            case ZTAR_PACK_STATE_TRAILER:
                res = pack_handle_trailer(stream, current_out, remaining_out, &step_written);
                break;
            case ZTAR_PACK_STATE_DONE:
                return ZTAR_RESULT_ARCHIVE_EXAHUSTED;
            default:
                return ZTAR_RESULT_INVALID_ARCHIVE;
        }

        if (res != ZTAR_RESULT_OK) {
            return res;
        }

        *bytes_written += step_written;
    }

    if (stream->state == ZTAR_PACK_STATE_DONE && *bytes_written == 0) {
        return ZTAR_RESULT_ARCHIVE_EXAHUSTED;
    }

    return ZTAR_RESULT_OK;
}

/************************************************
 *         Static functions definition          *
 ***********************************************/

static ztar_result_t pack_handle_next_file(ztar_pack_t *stream)
{
    bool has_next = false;
    int res = stream->callbacks.get_next_file(
        &has_next, stream->current_file_name, &stream->expected_file_size, stream->user_data);
    if (res < 0) {
        EDGEHOG_LOG_ERR("User callback returned error getting next file");
        return ZTAR_RESULT_USER_CBK_ERROR;
    }

    if (has_next) {
        stream->bytes_written_for_file = 0;
        stream->state = ZTAR_PACK_STATE_HEADER;
    } else {
        stream->bytes_processed_in_trailer = 0;
        stream->state = ZTAR_PACK_STATE_TRAILER;
    }
    return ZTAR_RESULT_OK;
}

static ztar_result_t pack_handle_header(
    ztar_pack_t *stream, uint8_t *out, size_t remaining, size_t *written)
{
    (void) remaining;
    *written = 0;

    ztar_header_t header;
    memset(&header, 0, sizeof(ztar_header_t));

    size_t name_len = strlen(stream->current_file_name);
    if (name_len <= ZTAR_HEADER_FIELD_NAME_LEN) {
        strncpy(header.name, stream->current_file_name, sizeof(header.name));
    } else {
        // Find an appropriate '/' in the file name so we can split into prefix and name.
        size_t prefix_len = 0;
        size_t suffix_len = 0;
        const char *split_ptr = strchr(stream->current_file_name, '/');
        while (split_ptr) {
            prefix_len = split_ptr - stream->current_file_name;
            suffix_len = name_len - prefix_len - 1;
            if (prefix_len <= ZTAR_HEADER_FIELD_PREFIX_LEN
                && suffix_len <= ZTAR_HEADER_FIELD_NAME_LEN) {
                break;
            }
            split_ptr = strchr(split_ptr + 1, '/');
        }

        // Early exit when we can't find a suitable split.
        if (!split_ptr) {
            EDGEHOG_LOG_ERR("File name too long and cannot be split into USTAR prefix/name: %s",
                stream->current_file_name);
            return ZTAR_RESULT_INVALID_ARGS;
        }

        // Copy prefix and name into their respective fields in the header
        memcpy(header.prefix, stream->current_file_name, prefix_len);
        strncpy(header.name, split_ptr + 1, sizeof(header.name));
    }

    if (pack_format_octal(FILE_MODE_DEFAULT, header.mode, sizeof(header.mode)) < 0) {
        return ZTAR_RESULT_INTERNAL_ERROR;
    }
    if (pack_format_octal(0, header.uid, sizeof(header.uid)) < 0) {
        return ZTAR_RESULT_INTERNAL_ERROR;
    }
    if (pack_format_octal(0, header.gid, sizeof(header.gid)) < 0) {
        return ZTAR_RESULT_INTERNAL_ERROR;
    }
    if (pack_format_octal(stream->expected_file_size, header.size, sizeof(header.size)) < 0) {
        return ZTAR_RESULT_INTERNAL_ERROR;
    }
    if (pack_format_octal(0, header.mtime, sizeof(header.mtime)) < 0) {
        return ZTAR_RESULT_INTERNAL_ERROR;
    }

    memcpy(header.magic, "ustar\0", ZTAR_HEADER_FIELD_MAGIC_LEN);
    memcpy(header.version, "00", ZTAR_HEADER_FIELD_VERSION_LEN);
    header.typeflag = '0'; // Regular file, no directory support
    memset(header.chksum, ' ', sizeof(header.chksum));

    uint32_t chksum = 0;
    uint8_t *raw_header = (uint8_t *) &header;
    for (size_t i = 0; i < sizeof(ztar_header_t); i++) {
        chksum += raw_header[i];
    }

    int ret = snprintf(header.chksum, sizeof(header.chksum), "%06o", chksum);
    if (ret < 0) {
        return ZTAR_RESULT_INTERNAL_ERROR;
    }
    header.chksum[CHKSUM_TERMINATOR_NULL_IDX] = '\0';
    header.chksum[CHKSUM_TERMINATOR_SPACE_IDX] = ' ';

    memcpy(out, &header, sizeof(ztar_header_t));
    *written = sizeof(ztar_header_t);

    if (stream->expected_file_size == 0) {
        stream->state = ZTAR_PACK_STATE_NEXT_FILE;
    } else {
        stream->state = ZTAR_PACK_STATE_DATA;
    }
    return ZTAR_RESULT_OK;
}

static ztar_result_t pack_handle_data(
    ztar_pack_t *stream, uint8_t *out, size_t remaining, size_t *written)
{
    *written = 0;
    size_t bytes_to_read
        = MIN(remaining, stream->expected_file_size - stream->bytes_written_for_file);

    size_t bytes_read = 0;
    int read_res
        = stream->callbacks.read_file_data(out, bytes_to_read, stream->user_data, &bytes_read);
    if (read_res < 0) {
        EDGEHOG_LOG_ERR("User callback returned error reading file data");
        return ZTAR_RESULT_USER_CBK_ERROR;
    }

    // Check if user callback failed to provide requested data prematurely
    if (bytes_read == 0 && bytes_to_read > 0) {
        EDGEHOG_LOG_ERR("User callback read 0 bytes but %zu were requested", bytes_to_read);
        return ZTAR_RESULT_TRUNCATED_FILE;
    }

    stream->bytes_written_for_file += bytes_read;
    *written = bytes_read;

    if (stream->bytes_written_for_file == stream->expected_file_size) {
        size_t required_padding = ZTAR_REQUIRED_FILE_PADDING(stream->expected_file_size);
        if (required_padding > 0) {
            stream->bytes_processed_in_padding = 0;
            stream->state = ZTAR_PACK_STATE_PADDING;
        } else {
            stream->state = ZTAR_PACK_STATE_NEXT_FILE;
        }
    }
    return ZTAR_RESULT_OK;
}

static ztar_result_t pack_handle_padding(
    ztar_pack_t *stream, uint8_t *out, size_t remaining, size_t *written)
{
    size_t total_padding = ZTAR_REQUIRED_FILE_PADDING(stream->expected_file_size);
    size_t padding_remaining = total_padding - stream->bytes_processed_in_padding;
    size_t write_padding = MIN(remaining, padding_remaining);

    memset(out, 0, write_padding);
    stream->bytes_processed_in_padding += write_padding;
    *written = write_padding;

    if (stream->bytes_processed_in_padding == total_padding) {
        stream->state = ZTAR_PACK_STATE_NEXT_FILE;
    }
    return ZTAR_RESULT_OK;
}

static ztar_result_t pack_handle_trailer(
    ztar_pack_t *stream, uint8_t *out, size_t remaining, size_t *written)
{
    size_t trailer_remaining = ZTAR_TRAILER_SIZE - stream->bytes_processed_in_trailer;
    size_t write_trailer = MIN(remaining, trailer_remaining);

    memset(out, 0, write_trailer);
    stream->bytes_processed_in_trailer += write_trailer;
    *written = write_trailer;

    if (stream->bytes_processed_in_trailer == ZTAR_TRAILER_SIZE) {
        stream->state = ZTAR_PACK_STATE_DONE;
    }
    return ZTAR_RESULT_OK;
}

static int pack_format_octal(uint64_t value, char *out, size_t out_size)
{
    if (!out || out_size == 0) {
        return -1;
    }

    // This will produce a zero-padded octal string.
    // %0* : Pad with zeros '0', width provided by the first argument '*'
    // PRIo64 : Cross-platform macro for formatting uint64_t as octal
    int written = snprintf(out, out_size, "%0*" PRIo64, (int) (out_size - 1), value);
    if (written < 0 || (size_t) written >= out_size) {
        return -1;
    }
    return 0;
}
