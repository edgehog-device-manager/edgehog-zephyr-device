<!---
  Copyright 2026 SECO Mind Srl

  SPDX-License-Identifier: Apache-2.0
-->

# Edgehog Device File Transfer

The Edgehog Zephyr Device library supports robust file transfer capabilities, allowing files to be transferred both from the server to the device (Download) and from the device to the server (Upload).

All file transfer requests are coordinated through Astarte using the specific interfaces defined in the [edgehog-astarte-interfaces](https://github.com/edgehog-device-manager/edgehog-astarte-interfaces) repository.

## Supported Transfer Modes

The library supports transferring data using two distinct mediums:
1. **File System:** Directly retrieving or storing files to a mounted Zephyr file system.
2. **Stream:** Processing the file transfer in-memory using Zephyr pipes (`k_pipe`) and events (`k_event`), allowing the application to handle the data stream dynamically.

### Feature Support Matrix

Below is the current support status for various file transfer configurations:

| Direction | Compression | Medium | Status |
| :--- | :--- | :--- | :--- |
| **Server -> Device** | Non-compressed | Stream | Supported with content length |
| **Server -> Device** | Non-compressed | File System | Supported with content length |
| **Server -> Device** | Compressed | Stream | Supported with content length |
| **Server -> Device** | Compressed | File System | Supported with content length |
| **Device -> Server** | Non-compressed | Stream | Supported with content length |
| **Device -> Server** | Non-compressed | File System | Supported with content length |
| **Device -> Server** | Compressed | Stream | NOT Supported |
| **Device -> Server** | Compressed | File System | NOT Supported |

## Configuration

To enable file transfers, it is necessary to set the `EDGEHOG_DEVICE_FILE_TRANSFER` kconfig. Also, the device must be properly configured during initialization by populating the `edgehog_device_config_t` structure.

You must provide the application-level callback hooks to accept/reject transfers and, if using the filesystem, define the allowed storage partitions.


```C
edgehog_device_config_t config = {
    // ... other configurations ...

    // configure file system partitions (if using filesystem transfers)
    .file_transfer_partitions = my_partitions,
    .file_transfer_partitions_len = ARRAY_SIZE(my_partitions),

    // configure transfer callbacks
    .file_transfer_cbks = {
        .on_stream_transfer_start = on_stream_transfer_start,
        .on_filesystem_transfer_done = on_filesystem_transfer_done
    }
};
```

### File System Configuration

For file system transfers, Edgehog requires explicit permission to interact with a specific mount point to ensure security. Use the `edgehog_ft_filesystem_partition_t` struct to map mount points (e.g., `/lfs1`) to specific permissions (`EDGEHOG_FT_FILESYSTEM_PERM_READ`, `EDGEHOG_FT_FILESYSTEM_PERM_WRITE`, or `EDGEHOG_FT_FILESYSTEM_PERM_RW`).

## Applicaction Callbacks

The library relies on application-defined callbacks (`edgehog_ft_cbks_t`) to orchestrate the transfers.

### Files sytem Transfers

When a filesystem transfer completes successfully, the library invokes `.on_filesystem_transfer_done`. At this point, the file is either fully downloaded to the specified path or fully uploaded to the server.

### Stream Transfers

For stream transfers, the library invokes `.on_stream_transfer_start`.
If the file transfer is accepted, the library dynamically allocates a `k_pipe` for data exchange and a `k_event` for synchronization.

Important Event Flags:
- `EDGEHOG_FT_STREAM_EOF_EVENT_FLAG`: Indicates the end of the file stream.
- `EDGEHOG_FT_STREAM_ACK_EVENT_FLAG`: Used by the application to acknowledge completion so the library can safely tear down memory.
- `EDGEHOG_FT_STREAM_ERROR_EVENT_FLAG`: Indicates an error occurred during the transfer.

### Example Usage: In-Memory Stream Transfer

The following example demonstrates how to implement a stream transfer, spawning dedicated Zephyr threads to handle asynchronous uploading and downloading using pipes.

```C
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include "edgehog_device/file_transfer.h"

LOG_MODULE_REGISTER(app_file_transfer, LOG_LEVEL_INF);

/* Callback used to intercept and start streams */
static bool on_stream_transfer_start(
    const char *name, edgehog_ft_type_t type, size_t *expected_size, edgehog_ft_stream_t *stream)
{
    if (strcmp(name, "loopback") == 0) {
        struct k_pipe *pipe = stream->pipe;
        struct k_event *event = stream->event;

        if (type == EDGEHOG_FT_TYPE_SERVER_TO_DEVICE) {
            LOG_INF("Starting download [server-to-device] on pipe '%s'. Expected size: %zu", name, *expected_size);

            // Spawn the download thread
            k_thread_create(&download_thread_data, download_thread_stack_area,
                K_THREAD_STACK_SIZEOF(download_thread_stack_area), download_thread_entry_point,
                (void *) pipe, (void *) event, NULL, CONFIG_E2E_DEVICE_THREAD_PRIORITY, 0, K_NO_WAIT);
        } else {
            *expected_size = loopback_file_size;
            LOG_INF("Starting upload [device-to-server] on pipe '%s'. Expected size: %zu", name, *expected_size);

            // Spawn the upload thread
            k_thread_create(&upload_thread_data, upload_thread_stack_area,
                K_THREAD_STACK_SIZEOF(upload_thread_stack_area), upload_thread_entry_point,
                (void *) pipe, (void *) event, NULL, CONFIG_E2E_DEVICE_THREAD_PRIORITY, 0, K_NO_WAIT);
        }
        // Accept transfer
        return true;
    }

    LOG_WRN("Rejected stream transfer for unknown path/name: %s", name);
    return false;
}

/* Completion callback */
static void on_filesystem_transfer_done(edgehog_ft_type_t type, const char *file_path)
{
    if (type == EDGEHOG_FT_TYPE_SERVER_TO_DEVICE) {
        LOG_INF("Filesystem transfer complete: downloaded to '%s'", file_path);
    } else if (type == EDGEHOG_FT_TYPE_DEVICE_TO_SERVER) {
        LOG_INF("Filesystem transfer complete: uploaded from '%s'", file_path);
    }
}

/* Thread for processing an incoming download stream */
static void download_thread_entry_point(void *stream_pipe, void *stream_event, void *unused3)
{
    struct k_pipe *pipe = (struct k_pipe *) stream_pipe;
    struct k_event *event = (struct k_event *) stream_event;
    bool download_complete = false;

    while (!download_complete) {
        uint8_t chunk[256];
        int ret = k_pipe_read(pipe, chunk, sizeof(chunk), K_MSEC(100));

        if (ret > 0) {
            // Process downloaded chunk (e.g., write to RAM buffer)
        } else if (ret < 0 && ret != -EAGAIN) {
            LOG_ERR("Failed to read pipe");
            k_event_post(event, EDGEHOG_FT_STREAM_ERROR_EVENT_FLAG);
            return;
        }

        // Check if Edgehog posted the EOF flag
        if (k_event_test(event, EDGEHOG_FT_STREAM_EOF_EVENT_FLAG)) {
            download_complete = true;
        }
    }

    // Acknowledge completion
    if (event) {
        k_event_post(event, EDGEHOG_FT_STREAM_ACK_EVENT_FLAG);
    }
}

/* Thread for processing an outgoing upload stream */
static void upload_thread_entry_point(void *stream_pipe, void *stream_event, void *unused3)
{
    struct k_pipe *pipe = (struct k_pipe *) stream_pipe;
    struct k_event *event = (struct k_event *) stream_event;
    size_t total_written = 0;

    while (total_written < loopback_file_size) {
        // Write progressively to the pipe
        int ret = k_pipe_write(pipe, loopback_file_buffer + total_written,
            loopback_file_size - total_written, K_MSEC(100));

        if (ret > 0) {
            total_written += ret;
        } else if (ret < 0 && ret != -EAGAIN) {
            LOG_ERR("Failed to write to pipe");
            break;
        }
    }

    // Post EOF to inform Edgehog there is no more data
    if (event) {
        k_event_post(event, EDGEHOG_FT_STREAM_EOF_EVENT_FLAG);
    }
}
```
