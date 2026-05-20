<!--
Copyright 2024 SECO Mind Srl

SPDX-License-Identifier: Apache-2.0
-->

# Edgehog app sample

This sample shows how to set up and connect an Edgehog device and its features.

## Before you begin

Each Edgehog device requires an Edgehog instance to connect to, which itself relies on an Astarte instance.

You could setup Edgehog following [this guide](https://docs.edgehog.io/0.12/edgehog_in_5_minutes.html) or you could use an [Astarte Cloud](https://astarte.cloud/) instance.

## Configuring the device

Depending on the type of Astarte/Edgehog instances you are using the configuration parameters for
this sample will be slightly different.

If you are usign a managed Astarte/Edgehog instance such as `astarte.cloud` you will be able to fully take advantage of the secure TLS connection of Astarte.

Set the following configuration entries in the `prj.conf` file of this sample.
```conf
CONFIG_ASTARTE_DEVICE_SDK_HOSTNAME="<HOSTNAME>"
CONFIG_ASTARTE_DEVICE_SDK_HTTPS_CA_CERT_TAG=1
CONFIG_ASTARTE_DEVICE_SDK_MQTTS_CA_CERT_TAG=1
CONFIG_ASTARTE_DEVICE_SDK_CLIENT_CERT_TAG=2
CONFIG_ASTARTE_DEVICE_SDK_REALM_NAME="<REALM_NAME>"

CONFIG_ASTARTE_DEVICE_ID="<DEVICE_ID>"
CONFIG_ASTARTE_CREDENTIAL_SECRET="<CREDENTIAL_SECRET>"
```
Where `<DEVICE_ID>` is the device ID of the device you would like to use in the sample, `<HOSTNAME>` is the hostname for your Astarte instance, `<REALM_NAME>` is the name of your testing realm and `<CREDENTIAL_SECRET>` is the credential secret obtained through the manual registration.

In addition, the file `ca_certificates.h` should be modified, placing in the `astarte_ca_certificate_root` array a valid CA certificate in the PEM format.

### Transport layer choice

Depending on which board you use, different transport layers may be available to you. This sample supports configurations for Ethernet and WiFi.
Ethernet will be built by default for all boards. While to use WiFi you should specify an extra configuration file when building `-DEXTRA_CONF_FILE="prj-wifi.conf"`.

Additionally the SSID and password for the WiFi AP should be added to the configuration:
```conf
CONFIG_WIFI_SSID=
CONFIG_WIFI_PASSWORD=
```

Depending on the board manufacturer you might need to download some blobs for the WiFi to function. For example for NXP boards:
```shell
west blobs fetch hal_nxp
```

## Building the sample

### Over the air (OTA) updates

A central part of Edgehog is that it provides over the air updates for large device fleets. An [OTA Updates](../../doc/architecture/Over%20The%20Air%20(OTA)%20updates.md) is a method for delivering firmware updates to remote devices using a network connection. This sample will show you how to build and deploy a device using `sysbuild` that leverages `MCUboot` capabilities offering out of the box OTA updates.

### Sysbuild specific settings

Building with --sysbuild automatically includes and configures MCUboot to enable OTA updates.

Build the sample using:

```sh
west build -b stm32h573i_dk --sysbuild samples/edgehog_app
```

The board overlay for MCUboot is searched in the `sysbuild/boards/` directory. To support new boards, simply add the corresponding overlay file to this folder and it will be loaded automatically. Alternatively, you can pass the overlay manually using `-Dmcuboot_DTC_OVERLAY_FILE=/path/to/overlay`.

### File transfer

Edgehog devices support bi-directional file transfers, both download (server-to-device) and upload (device-to-server). This sample demonstrates how to implement a stream transfer, spawning dedicated Zephyr threads to handle asynchronous uploading and downloading using pipes.

The sample implements a **loopback mechanism** leveraging in-memory `k_pipe` objects:
- When a download transfer is initiated with the filename `loopback`, the sample intercepts the stream via `.on_stream_transfer_start` and spawns a dedicated download thread. The payload is written directly to a RAM buffer dynamically.
- When an upload transfer is requested matching the `loopback` name, an upload thread is spawned that pushes the previously buffered payload back up to the server.

This mechanism allows you to test both incoming and outgoing data flows rapidly without requiring persistent storage (flash/filesystem) configured on your target board. For file system operations, the sample currently logs when an `.on_filesystem_transfer_done` event occurs.

## Flashing the sample

Flashing this sample can be done easily using the standard command.
```sh
west flash
```

## Additional information

Edgehog leverages [Astarte](https://docs.astarte-platform.org/) to connect the Edgehog device with the Edgehog cloud instance. This means that each Edgehog device comes with an Astarte device for free.

It is important to consider that such Astarte device is managed internally by Edgehog and the user should limit its operations to installing new interfaces and transmitting and receiving on them. The Edgehog device will manage the connection and instantiate/destroy the device when required.

You can provide your interfaces and callbacks during the creation of the Edgehog device in the `astarte_device_config_t` struct exactly as you would do with an ordinary Astarte device. If you want to transmit data with the Astarte device you can obtain its handle using the `edgehog_device_get_astarte_device` function. It will return an Astarte device handle that can then be used to transmit as any other Astarte device.
