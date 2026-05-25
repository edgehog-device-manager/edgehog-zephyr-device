/*
 * (C) Copyright 2026, SECO Mind Srl
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ztar/unpack.h"

#include <zephyr/sys/util.h>

#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "log.h"

EDGEHOG_LOG_MODULE_REGISTER(ztar_unpack, CONFIG_EDGEHOG_DEVICE_ZTAR_LOG_LEVEL);

/************************************************
 *        Defines, constants and typedef        *
 ***********************************************/

// Octal base used in USTAR to store numerical values
#define OCTAL_BASE 8

/************************************************
 *         Static functions declaration         *
 ***********************************************/

// Process a complete or partial header from the stream.
static ztar_result_t stream_process_header(
    ztar_unpack_t *stream, const uint8_t *data, size_t size, size_t *bytes_consumed);
// Process a complete or partial file data block from the stream.
static ztar_result_t stream_process_data(
    ztar_unpack_t *stream, const uint8_t *data, size_t size, size_t *bytes_consumed);
// Process a complete or partial padding block from the stream.
static ztar_result_t stream_process_padding(
    ztar_unpack_t *stream, const uint8_t *data, size_t size, size_t *bytes_consumed);
// Process a complete or partial trailer block from the stream.
static ztar_result_t stream_process_trailer(
    ztar_unpack_t *stream, const uint8_t *data, size_t size, size_t *bytes_consumed);
// Validates the magic string, version, and the header checksum.
static ztar_result_t validate_header(const ztar_header_t *header);
// Validates that the 6-byte magic indicator is strictly POSIX USTAR.
static ztar_result_t validate_magic(const ztar_header_t *header);
// Validates that the 2-byte version string is strictly POSIX USTAR.
static ztar_result_t validate_version(const ztar_header_t *header);
// Gets the header checksum.
static ztar_result_t get_chksum(const ztar_header_t *header, uint32_t *chksum);

/************************************************
 *         Global functions definition          *
 ***********************************************/

ztar_result_t ztar_unpack_init(
    ztar_unpack_t *stream, ztar_unpack_callbacks_t callbacks, void *user_data)
{
    if (!stream) {
        EDGEHOG_LOG_ERR("Called ztar_unpack_init with null stream pointer");
        return ZTAR_RESULT_INVALID_ARGS;
    }
    if (!callbacks.on_file_start || !callbacks.on_file_data || !callbacks.on_file_end) {
        EDGEHOG_LOG_ERR("Missing callback for unpacking context");
        return ZTAR_RESULT_INVALID_ARGS;
    }
    EDGEHOG_LOG_DBG("Initializing ztar unpacking stream");

    memset(stream, 0, sizeof(ztar_unpack_t));
    stream->initialized = true;
    stream->state = ZTAR_UNPACK_STATE_HEADER;
    stream->callbacks = callbacks;
    stream->user_data = user_data;

    return ZTAR_RESULT_OK;
}

bool ztar_unpack_is_initialized(const ztar_unpack_t *stream)
{
    if (!stream) {
        EDGEHOG_LOG_ERR("Called ztar_unpack_is_initialized with null stream pointer");
        return false;
    }
    return stream->initialized;
}

ztar_result_t ztar_unpack_process(ztar_unpack_t *stream, const uint8_t *data, size_t size)
{
    ztar_result_t zres = ZTAR_RESULT_OK;
    if (!stream || !data) {
        EDGEHOG_LOG_ERR("Called ztar_unpack_process with null stream pointer or data pointer");
        return ZTAR_RESULT_INVALID_ARGS;
    }
    EDGEHOG_LOG_DBG("Processing ztar stream data of size %zu", size);

    // Loop until we've consumed all input data
    size_t offset = 0;
    while (offset < size) {
        size_t bytes_consumed = 0;
        switch (stream->state) {
            case ZTAR_UNPACK_STATE_HEADER:
                zres = stream_process_header(stream, data + offset, size - offset, &bytes_consumed);
                break;
            case ZTAR_UNPACK_STATE_DATA:
                zres = stream_process_data(stream, data + offset, size - offset, &bytes_consumed);
                break;
            case ZTAR_UNPACK_STATE_PADDING:
                zres
                    = stream_process_padding(stream, data + offset, size - offset, &bytes_consumed);
                break;
            case ZTAR_UNPACK_STATE_TRAILER:
                zres
                    = stream_process_trailer(stream, data + offset, size - offset, &bytes_consumed);
                break;
            default:
                zres = ZTAR_RESULT_INVALID_ARCHIVE;
                break;
        }
        if (zres == ZTAR_RESULT_ARCHIVE_EXAHUSTED) {
            EDGEHOG_LOG_DBG("Completed processing ztar stream");
            return zres;
        }
        if (zres != ZTAR_RESULT_OK) {
            EDGEHOG_LOG_ERR("State machine error processing ztar stream: %d", zres);
            return zres;
        }
        offset += bytes_consumed;
    }

    return ZTAR_RESULT_OK;
}

ztar_result_t ztar_unpack_get_file_name(
    const ztar_header_t *header, char buffer[static ZTAR_FILE_NAME_BUFF_SIZE])
{
    if (!header || !buffer) {
        EDGEHOG_LOG_ERR(
            "Called ztar_unpack_get_file_name with null header pointer or buffer pointer");
        return ZTAR_RESULT_INVALID_ARGS;
    }
    EDGEHOG_LOG_DBG("Getting file name for header");

    int ret = 0;
    if (header->prefix[0] != '\0') {
        ret = snprintf(
            buffer, ZTAR_FILE_NAME_BUFF_SIZE, "%.155s/%.100s", header->prefix, header->name);
    } else {
        ret = snprintf(buffer, ZTAR_FILE_NAME_BUFF_SIZE, "%.100s", header->name);
    }

    if (ret < 0) {
        return ZTAR_RESULT_INTERNAL_ERROR;
    }

    return ZTAR_RESULT_OK;
}

ztar_result_t ztar_unpack_get_file_size(const ztar_header_t *header, size_t *file_size)
{
    if (!header || !file_size) {
        EDGEHOG_LOG_ERR(
            "Called ztar_unpack_get_file_size with null header pointer or file_size pointer");
        return ZTAR_RESULT_INVALID_ARGS;
    }

    // Size buf is null terminated
    char size_buf[ZTAR_HEADER_FIELD_SIZE_LEN + 1] = { 0 };
    memcpy(size_buf, header->size, ZTAR_HEADER_FIELD_SIZE_LEN);

    // Size is an octal string
    char *endptr = NULL;
    unsigned long long parsed_size = strtoull(size_buf, &endptr, OCTAL_BASE);

    // Validate that the conversion consumed at least one digit
    if (endptr == size_buf) {
        return ZTAR_RESULT_INVALID_ARCHIVE;
    }

    *file_size = (size_t) parsed_size;
    return ZTAR_RESULT_OK;
}

ztar_result_t ztar_unpack_get_file_type(const ztar_header_t *header, ztar_filetype_t *file_type)
{
    if (!header || !file_type) {
        EDGEHOG_LOG_ERR(
            "Called ztar_unpack_get_file_type with null header pointer or file_type pointer");
        return ZTAR_RESULT_INVALID_ARGS;
    }

    // In standard tar, both '0' and '\0' denote a regular file
    if (header->typeflag == '0' || header->typeflag == '\0') {
        *file_type = ZTAR_REGULAR_FILE;
    }
    // '5' denotes a directory
    else if (header->typeflag == '5') {
        *file_type = ZTAR_DIRECTORY;
    }
    // Anything else (including PAX 'x'/'g' headers) is unsupported
    else {
        EDGEHOG_LOG_ERR("Found unsupported file type with typeflag '%c'", header->typeflag);
        *file_type = ZTAR_UNSUPPORTED;
    }

    return ZTAR_RESULT_OK;
}

/************************************************
 *         Static functions definition          *
 ***********************************************/

static ztar_result_t stream_process_header(
    ztar_unpack_t *stream, const uint8_t *data, size_t size, size_t *bytes_consumed)
{
    // Calculate how many bytes we still need to complete the header and copy as much as we can
    size_t header_bytes_needed = sizeof(ztar_header_t) - stream->bytes_processed_in_header;
    size_t bytes_to_copy = MIN(size, header_bytes_needed);
    memcpy((uint8_t *) &stream->current_header + stream->bytes_processed_in_header, data,
        bytes_to_copy);
    stream->bytes_processed_in_header += bytes_to_copy;

    // If we've completed the header, validate it and trigger the file start callback
    if (stream->bytes_processed_in_header == sizeof(ztar_header_t)) {
        // Check if this is the end of the archive
        uint8_t *raw_header = (uint8_t *) &stream->current_header;
        if ((raw_header[0] == 0)
            && (memcmp(raw_header, raw_header + 1, sizeof(ztar_header_t) - 1) == 0)) {
            stream->bytes_processed_in_trailer = ZTAR_BLOCK_SIZE;
            stream->state = ZTAR_UNPACK_STATE_TRAILER;
        } else {
            ztar_result_t zres = validate_header(&stream->current_header);
            if (zres != ZTAR_RESULT_OK) {
                return zres;
            }
            stream->bytes_processed_in_file = 0;
            stream->state = ZTAR_UNPACK_STATE_DATA;

            // Invoke the file start callback with the parsed header
            if (stream->callbacks.on_file_start) {
                int cbk_res
                    = stream->callbacks.on_file_start(&stream->current_header, stream->user_data);
                if (cbk_res != 0) {
                    EDGEHOG_LOG_ERR("File start callback returned error code %d", cbk_res);
                    return ZTAR_RESULT_USER_CBK_ERROR;
                }
            }
        }
    }

    // Output how many bytes we've consumed from the input data
    *bytes_consumed = bytes_to_copy;
    return ZTAR_RESULT_OK;
}

static ztar_result_t stream_process_data(
    ztar_unpack_t *stream, const uint8_t *data, size_t size, size_t *bytes_consumed)
{
    ztar_result_t zres = ZTAR_RESULT_OK;

    size_t file_size = 0;
    zres = ztar_unpack_get_file_size(&stream->current_header, &file_size);
    if (zres != ZTAR_RESULT_OK) {
        return zres;
    }

    // Calculate how many bytes we still need to complete the file and parse as much as we can
    size_t file_bytes_needed = file_size - stream->bytes_processed_in_file;
    size_t bytes_to_process = MIN(size, file_bytes_needed);

    // Invoke the file data callback with the parsed chunk
    if ((stream->callbacks.on_file_data) && (bytes_to_process != 0)) {
        int cbk_res = stream->callbacks.on_file_data(
            &stream->current_header, data, bytes_to_process, stream->user_data);
        if (cbk_res != 0) {
            EDGEHOG_LOG_ERR("File data callback returned error code %d", cbk_res);
            return ZTAR_RESULT_USER_CBK_ERROR;
        }
    }

    // Update how many bytes we've processed for the current file
    stream->bytes_processed_in_file += bytes_to_process;

    // If we've completed the file, trigger the file end callback and transition to the next state
    if (stream->bytes_processed_in_file == file_size) {
        if (stream->callbacks.on_file_end) {
            int cbk_res = stream->callbacks.on_file_end(&stream->current_header, stream->user_data);
            if (cbk_res != 0) {
                EDGEHOG_LOG_ERR("File end callback returned error code %d", cbk_res);
                return ZTAR_RESULT_USER_CBK_ERROR;
            }
        }

        // Calculate padding to next boundary
        size_t required_padding = ZTAR_REQUIRED_FILE_PADDING(file_size);
        if (required_padding == 0) {
            stream->bytes_processed_in_header = 0;
            stream->state = ZTAR_UNPACK_STATE_HEADER;
        } else {
            stream->bytes_processed_in_padding = 0;
            stream->state = ZTAR_UNPACK_STATE_PADDING;
        }
    }

    *bytes_consumed = bytes_to_process;
    return ZTAR_RESULT_OK;
}

static ztar_result_t stream_process_padding(
    ztar_unpack_t *stream, const uint8_t *data, size_t size, size_t *bytes_consumed)
{
    ztar_result_t zres = ZTAR_RESULT_OK;

    size_t file_size = 0;
    zres = ztar_unpack_get_file_size(&stream->current_header, &file_size);
    if (zres != ZTAR_RESULT_OK) {
        return zres;
    }

    // Calculate how many bytes we still need to complete the padding and parse as much as we can
    size_t required_padding = ZTAR_REQUIRED_FILE_PADDING(file_size);
    size_t file_bytes_needed = required_padding - stream->bytes_processed_in_padding;
    size_t bytes_to_process = MIN(size, file_bytes_needed);

    // Padding bytes must be zero
    if ((data[0] != 0) || (memcmp(data, data + 1, bytes_to_process - 1) != 0)) {
        return ZTAR_RESULT_INVALID_ARCHIVE;
    }

    // Update how many bytes we've processed for the current padding
    stream->bytes_processed_in_padding += bytes_to_process;

    // If we've completed the padding, transition to the next state
    if (stream->bytes_processed_in_padding == required_padding) {
        stream->bytes_processed_in_header = 0;
        stream->state = ZTAR_UNPACK_STATE_HEADER;
    }

    *bytes_consumed = bytes_to_process;
    return ZTAR_RESULT_OK;
}

static ztar_result_t stream_process_trailer(
    ztar_unpack_t *stream, const uint8_t *data, size_t size, size_t *bytes_consumed)
{
    // Safety check to avoid processing more data if we've already consumed the entire trailer
    if (stream->bytes_processed_in_trailer == ZTAR_TRAILER_SIZE) {
        return ZTAR_RESULT_ARCHIVE_EXAHUSTED;
    }

    // We are in the trailer state, we just need to consume 1024 bytes of zeros
    size_t trailer_bytes_needed = ZTAR_TRAILER_SIZE - stream->bytes_processed_in_trailer;
    size_t bytes_to_consume = MIN(size, trailer_bytes_needed);

    if ((data[0] != 0) || (memcmp(data, data + 1, bytes_to_consume - 1) != 0)) {
        return ZTAR_RESULT_INVALID_ARCHIVE;
    }

    stream->bytes_processed_in_trailer += bytes_to_consume;

    // If we've consumed the entire trailer, we can consider the archive fully processed
    if (stream->bytes_processed_in_trailer == ZTAR_TRAILER_SIZE) {
        return ZTAR_RESULT_ARCHIVE_EXAHUSTED;
    }

    *bytes_consumed = bytes_to_consume;
    return ZTAR_RESULT_OK;
}

static ztar_result_t validate_header(const ztar_header_t *header)
{
    ztar_result_t zres = ZTAR_RESULT_OK;
    if (!header) {
        return ZTAR_RESULT_INVALID_ARGS;
    }

    // Validate the magic indicator
    zres = validate_magic(header);
    if (zres != ZTAR_RESULT_OK) {
        return zres;
    }

    // Validate the version string
    zres = validate_version(header);
    if (zres != ZTAR_RESULT_OK) {
        return zres;
    }

    // Validate the checksum
    uint32_t parsed_chksum = 0;
    zres = get_chksum(header, &parsed_chksum);
    if (zres != ZTAR_RESULT_OK) {
        return zres;
    }

    // Calculate the sum of all bytes in the header block (512 bytes)
    uint32_t expected_chksum = 0;
    uint8_t *raw_header = (uint8_t *) header;
    for (size_t i = 0; i < sizeof(ztar_header_t); i++) {
        expected_chksum += raw_header[i];
    }

    // Subtract the actual bytes in the chksum field and replace them with spaces (' ')
    for (size_t i = 0; i < sizeof(header->chksum); i++) {
        expected_chksum -= (uint8_t) header->chksum[i];
        expected_chksum += ' ';
    }

    if (expected_chksum != parsed_chksum) {
        return ZTAR_RESULT_INVALID_CHECKSUM;
    }

    return ZTAR_RESULT_OK;
}

static ztar_result_t validate_magic(const ztar_header_t *header)
{
    if (!header) {
        return ZTAR_RESULT_INVALID_ARGS;
    }

    // Strict POSIX USTAR uses "ustar\0".
    // This safely rejects GNU tar's "ustar " (space padded).
    if (memcmp(header->magic, "ustar\0", ZTAR_HEADER_FIELD_MAGIC_LEN) != 0) {
        return ZTAR_RESULT_INVALID_MAGIC;
    }

    return ZTAR_RESULT_OK;
}

static ztar_result_t validate_version(const ztar_header_t *header)
{
    if (!header) {
        return ZTAR_RESULT_INVALID_ARGS;
    }

    // Strict POSIX USTAR uses "00".
    // This safely rejects GNU tar's " \0" (space and null).
    if (memcmp(header->version, "00", ZTAR_HEADER_FIELD_VERSION_LEN) != 0) {
        return ZTAR_RESULT_INVALID_VERSION;
    }

    return ZTAR_RESULT_OK;
}

static ztar_result_t get_chksum(const ztar_header_t *header, uint32_t *chksum)
{
    if (!header || !chksum) {
        return ZTAR_RESULT_INVALID_ARGS;
    }

    // Checksum buf is null terminated
    char chksum_buf[ZTAR_HEADER_FIELD_CHKSUM_LEN + 1] = { 0 };
    memcpy(chksum_buf, header->chksum, ZTAR_HEADER_FIELD_CHKSUM_LEN);

    // Checksum is an 8-byte octal string
    *chksum = (uint32_t) strtoul(chksum_buf, NULL, OCTAL_BASE);
    return ZTAR_RESULT_OK;
}
