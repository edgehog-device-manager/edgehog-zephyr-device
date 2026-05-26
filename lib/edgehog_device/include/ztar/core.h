/*
 * (C) Copyright 2026, SECO Mind Srl
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZTAR_H
#define ZTAR_H

/**
 * @file ztar/core.h
 * @brief Tar archive extraction and packing.
 * @details This parser only supports the USTAR format and is designed to work with streaming data,
 * making it suitable for processing large tar archives without needing to load the entire archive
 * into memory. It provides callbacks for file start, file data, and file end events, allowing users
 * to handle extracted files as they are processed from the stream. It also supports packing files
 * into a stream.
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Fundamental block size for the USTAR format, equal to the size of the header. */
#define ZTAR_BLOCK_SIZE 512
/** @brief Trailer size for the USTAR format which is composed by two empty blocks. */
#define ZTAR_TRAILER_SIZE (ZTAR_BLOCK_SIZE * 2)
/**
 * @brief Maximum file name length according to the USTAR format.
 * @details Includes the full path to the file and the NULL terminator.
 */
#define ZTAR_FILE_NAME_BUFF_SIZE 257
/** @brief Calculates the requred padding for a file of arbitrary size. */
#define ZTAR_REQUIRED_FILE_PADDING(file_size)                                                      \
    ((ZTAR_BLOCK_SIZE - (file_size % ZTAR_BLOCK_SIZE)) % ZTAR_BLOCK_SIZE)

/** @brief Size of the field name. */
#define ZTAR_HEADER_FIELD_NAME_LEN 100
/** @brief Size of the field mode. */
#define ZTAR_HEADER_FIELD_MODE_LEN 8
/** @brief Size of the field uid. */
#define ZTAR_HEADER_FIELD_UID_LEN 8
/** @brief Size of the field gid. */
#define ZTAR_HEADER_FIELD_GID_LEN 8
/** @brief Size of the field size. */
#define ZTAR_HEADER_FIELD_SIZE_LEN 12
/** @brief Size of the field mtime. */
#define ZTAR_HEADER_FIELD_MTIME_LEN 12
/** @brief Size of the field chksum. */
#define ZTAR_HEADER_FIELD_CHKSUM_LEN 8
/** @brief Size of the field typeflag. */
#define ZTAR_HEADER_FIELD_TYPEFLAG_LEN 1
/** @brief Size of the field linkname. */
#define ZTAR_HEADER_FIELD_LINKNAME_LEN 100
/** @brief Size of the field magic. */
#define ZTAR_HEADER_FIELD_MAGIC_LEN 6
/** @brief Size of the field version. */
#define ZTAR_HEADER_FIELD_VERSION_LEN 2
/** @brief Size of the field uname. */
#define ZTAR_HEADER_FIELD_UNAME_LEN 32
/** @brief Size of the field gname. */
#define ZTAR_HEADER_FIELD_GNAME_LEN 32
/** @brief Size of the field devmajor. */
#define ZTAR_HEADER_FIELD_DEVMAJOR_LEN 8
/** @brief Size of the field devminor. */
#define ZTAR_HEADER_FIELD_DEVMINOR_LEN 8
/** @brief Size of the field prefix. */
#define ZTAR_HEADER_FIELD_PREFIX_LEN 155
/** @brief Final padding to get the ztar_header_t to the full header size. */
#define ZTAR_HEADER_HEADER_PADDING 12

/** @brief Result codes for ztar stream operations. */
typedef enum
{
    /** @brief Operation completed successfully. */
    ZTAR_RESULT_OK = 0,
    /** @brief Internal error, should not occur. */
    ZTAR_RESULT_INTERNAL_ERROR,
    /** @brief Invalid arguments provided to the function. */
    ZTAR_RESULT_INVALID_ARGS,
    /** @brief The archive format is invalid. */
    ZTAR_RESULT_INVALID_ARCHIVE,
    /** @brief The provided buffer is insufficient. */
    ZTAR_RESULT_INSUFFICIENT_BUFF,
    /** @brief The TAR magic indicator is invalid. */
    ZTAR_RESULT_INVALID_MAGIC,
    /** @brief The TAR version is invalid. */
    ZTAR_RESULT_INVALID_VERSION,
    /** @brief The checksum of the header is invalid. */
    ZTAR_RESULT_INVALID_CHECKSUM,
    /** @brief The file within the archive is truncated. */
    ZTAR_RESULT_TRUNCATED_FILE,
    /** @brief The archive itself is truncated. */
    ZTAR_RESULT_TRUNCATED_ARCHIVE,
    /** @brief The archive stream has been fully exhausted. */
    ZTAR_RESULT_ARCHIVE_EXAHUSTED,
    /** @brief The user callback returned an error. */
    ZTAR_RESULT_USER_CBK_ERROR,
} ztar_result_t;

/** @brief Extracted TAR file types. */
typedef enum
{
    /** @brief Standard regular file. */
    ZTAR_REGULAR_FILE,
    /** @brief Directory file type. */
    ZTAR_DIRECTORY,
    /** @brief Handles symlinks, hardlinks, FIFOs, and PAX extension headers. */
    ZTAR_UNSUPPORTED
} ztar_filetype_t;

/**
 * @brief Standard POSIX TAR header structure.
 * @note This structure is exactly 512 bytes, which is the size of a TAR header block. It relies
 * on the compiler not adding any padding between fields, which is generally safe given the
 * use of only char arrays and single char fields, but should be verified if any changes are made.
 */
typedef struct
{
    /** @brief Name of the file. */
    char name[ZTAR_HEADER_FIELD_NAME_LEN];
    /** @brief File permissions mode. */
    char mode[ZTAR_HEADER_FIELD_MODE_LEN];
    /** @brief Numeric user ID of the file owner. */
    char uid[ZTAR_HEADER_FIELD_UID_LEN];
    /** @brief Numeric group ID of the file owner. */
    char gid[ZTAR_HEADER_FIELD_GID_LEN];
    /** @brief File size in bytes (octal string). */
    char size[ZTAR_HEADER_FIELD_SIZE_LEN];
    /** @brief Last modification time in numeric Unix time format. */
    char mtime[ZTAR_HEADER_FIELD_MTIME_LEN];
    /** @brief Checksum for the header block. */
    char chksum[ZTAR_HEADER_FIELD_CHKSUM_LEN];
    /** @brief Link indicator / file type flag. */
    char typeflag;
    /** @brief Name of the linked file. */
    char linkname[ZTAR_HEADER_FIELD_LINKNAME_LEN];
    /** @brief UStar magic indicator ("ustar"). */
    char magic[ZTAR_HEADER_FIELD_MAGIC_LEN];
    /** @brief UStar version ("00"). */
    char version[ZTAR_HEADER_FIELD_VERSION_LEN];
    /** @brief Owner user name. */
    char uname[ZTAR_HEADER_FIELD_UNAME_LEN];
    /** @brief Owner group name. */
    char gname[ZTAR_HEADER_FIELD_GNAME_LEN];
    /** @brief Device major number. */
    char devmajor[ZTAR_HEADER_FIELD_DEVMAJOR_LEN];
    /** @brief Device minor number. */
    char devminor[ZTAR_HEADER_FIELD_DEVMINOR_LEN];
    /** @brief Prefix for the file name. */
    char prefix[ZTAR_HEADER_FIELD_PREFIX_LEN];
    /** @brief Padding to fill the 512-byte block. */
    char padding[ZTAR_HEADER_HEADER_PADDING];
} ztar_header_t;

#ifdef __cplusplus
}
#endif

#endif
