<!--
Copyright 2024 SECO Mind Srl

SPDX-License-Identifier: Apache-2.0
-->

# Edgehog app example

This sample shows how to setup and connect an Edgehog device and their features.

## Required configuration

The examples need to be configured to work with a testing/demonstration Astarte and Edgehog instance or
a fully deployed Astarte and Edgehog instance.
We assume a device with a known device id has been manually registered in the Astarte instance.
The credential secret obtained through the registration should be added to the configuration.
The configuration can be added to the `prj.conf` file of this example.

All use cases require setting the WiFi SSID and password to valid values.

### Configuration for testing or demonstration

This option assumes you are using this example with an Astarte instance similar to the
one that results from following the
[Astarte in 5 minutes](https://docs.astarte-platform.org/astarte/latest/010-astarte_in_5_minutes.html)
and [Edgehog in 5 minutes](https://docs.edgehog.io/snapshot/edgehog_in_5_minutes.html)
tutorials.

The following entries should be modified in the `proj.conf` file.
```conf
CONFIG_ASTARTE_DEVICE_SDK_DEVICE_ID="<DEVICE_ID>"
CONFIG_ASTARTE_DEVICE_SDK_HOSTNAME="<HOSTNAME>"
CONFIG_ASTARTE_DEVICE_SDK_DEVELOP_DISABLE_OR_IGNORE_TLS=y
CONFIG_ASTARTE_DEVICE_SDK_CLIENT_CERT_TAG=1
CONFIG_ASTARTE_DEVICE_SDK_REALM_NAME="<REALM_NAME>"

CONFIG_CREDENTIAL_SECRET="<CREDENTIAL_SECRET>"
```
Where `<DEVICE_ID>` is the device ID of the device you would like to use in the sample, `<HOSTNAME>`
is the hostname for your Astarte instance, `<REALM_NAME>` is the name of your testing realm and
`<CREDENTIAL_SECRET>` is the credential secret obtained through the manual registration.

### Configuration for fully functional Astarte

This option assumes you are using a fully deployed Astarte instance with valid certificates from
an official certificate authority.

```conf
CONFIG_ASTARTE_DEVICE_SDK_HOSTNAME="<HOSTNAME>"
CONFIG_ASTARTE_DEVICE_SDK_HTTPS_CA_CERT_TAG=1
CONFIG_ASTARTE_DEVICE_SDK_MQTTS_CA_CERT_TAG=1
CONFIG_ASTARTE_DEVICE_SDK_CLIENT_CERT_TAG=2
CONFIG_ASTARTE_DEVICE_SDK_REALM_NAME="<REALM_NAME>"

CONFIG_DEVICE_ID="<DEVICE_ID>"
CONFIG_CREDENTIAL_SECRET="<CREDENTIAL_SECRET>"
```
Where `<DEVICE_ID>` is the device ID of the device you would like to use in the sample, `<HOSTNAME>`
is the hostname for your Astarte instance, `<REALM_NAME>` is the name of your testing realm and
`<CREDENTIAL_SECRET>` is the credential secret obtained through the manual registration.

In addition, the file `ca_certificates.h` should be modified, placing in the `ca_certificate_root`
array a valid CA certificate in the PEM format.

### OTA
[Over-the-Air (OTA) Update](../../doc/ota.md) is a method for delivering firmware updates to remote devices using a network connection.

This simple example that demonstrates how building a sample using `sysbuild` can automatically include `MCUboot` as the bootloader.

### Sysbuild specific settings
This sample automatically includes MCUboot as bootloader when built using sysbuild.

This is achieved with a sysbuild specific Kconfig configuration, `sysbuild.conf`.
The `SB_CONFIG_BOOTLOADER_MCUBOOT=y` setting in the sysbuild Kconfig file enables the bootloader when building with sysbuild.
The `sysbuild/mcuboot.conf` file will be used as an extra fragment that is merged together with the default configuration files used by MCUboot.

`sysbuild/mcuboot.conf` adjusts the log level in MCUboot, as well as configures MCUboot to prevent downgrades and operate in upgrade-only mode.

Use ``--sysbuild`` to select the `sysbuild` build infrastructure with `west build` to build multiple domains.

**_Note:_**
By default, sysbuild use the `app.overlay` from MCUboot folder, so if you want MCUboot to use your board's overlay file, you can pass `mcuboot_DTC_OVERLAY_FILE` parameter to `west build`.

```
west build -b mimxrt1064_evk --sysbuild samples/edgehog_app -Dmcuboot_DTC_OVERLAY_FILE=${PWD}/samples/edgehog_app/boards/mimxrt1064_evk.overlay
```

Or you can put your board overlay file in the `sysbuild/boards` directory, and this parameter was automatically added by `sysbuild.cmake` contained in this sample.

```
west build -b mimxrt1064_evk --sysbuild samples/edgehog_app
```

### Flash runner for nxp boards

The default runner used for nxp boards is `jlink`. To flash without a debug link use:
```sh
west flash --runner=linkserver
```
