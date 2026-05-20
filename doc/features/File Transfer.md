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

### Feature Support

Support status for the **Server -> Device** file transfer configuration:

| Compression | Medium | Status |
| :--- | :--- | :--- |
| Non-compressed | Stream | Supported |
| Non-compressed | File System | Supported |
| Compressed | Stream | Supported |
| Compressed | File System | Supported |

Support status for the **Device -> Server** file transfer configuration:

| Compression | Medium | Status |
| :--- | :--- | :--- |
| Non-compressed | Stream | Supported |
| Non-compressed | File System | Supported |
| Compressed | Stream | NOT Supported |
| Compressed | File System | NOT Supported |

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

### Filesytem Transfers

When a filesystem transfer completes successfully, the library invokes `.on_filesystem_transfer_done`. At this point, the file is either fully downloaded to the specified path or fully uploaded to the server.

### Stream Transfers

For stream transfers, the library invokes `.on_stream_transfer_start`.
If the file transfer is accepted, the library dynamically allocates a `k_pipe` for data exchange and a `k_event` for synchronization.

Important Event Flags:
- `EDGEHOG_FT_STREAM_EOF_EVENT_FLAG`: Indicates the end of the file stream.
- `EDGEHOG_FT_STREAM_ACK_EVENT_FLAG`: Used by the application to acknowledge completion so the library can safely tear down memory.
- `EDGEHOG_FT_STREAM_ERROR_EVENT_FLAG`: Indicates an error occurred during the transfer.
