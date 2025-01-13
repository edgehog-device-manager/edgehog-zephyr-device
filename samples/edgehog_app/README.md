<!--
Copyright 2024 SECO Mind Srl

SPDX-License-Identifier: Apache-2.0
-->

# Edgehog app sample

This sample shows how to setup and connect an Edgehog device and their features.

## Before you begin

This sample shows how to set up and connect an Edgehog device.
Each Edgehog device requires an Edgehog instance to connect to, which itself relies on an Astarte
instance.
If you are using a managed Edgehog instance both components will be provided to you and you can skip
directly to the **Configuring the device** section.

### Astarte instance

You will need an Astarte instance to connect with the Edgehog instance.
The fastest way to set this up is to follow the
[Astarte quick instance](https://docs.astarte-platform.org/device-sdks/common/astarte_quick_instance.html)
guide provided with the Astarte documentation.

Make sure you do not loose the following information:
- Your Astarte realm name. The default for the Astarte quick instance is `test`.
- The location of your Astarte instance. The default for the Astarte quick instance is an `astarte`
  folder in your home.
- The Astarte API endpoint. If you followed the Astarte quick instance guide you should have
  choosen to configure Astarte for an external device. As a consequence this endpoint will be:
  `api.astarte.<YOUR IP ADDRESS>.nip.io`.
- A device ID registered using the dashboard and its associated credential secret. If you followed
  the Astarte quick instance guide you can register a device in the dashboard, navigating to the
  devices tab, clicking on register a new device and register a device with a random ID using the
  default settings.

### Edgehog instance

Once you have your Astarte instance up and running only a couple steps are required for
installing and running the Edgehog instance.

Start by cloning the Edgehog repository and navigate into it.
```sh
git clone https://github.com/edgehog-device-manager/edgehog -b v0.9.2 && cd edgehog
```

Next you will need to change some of the configuration variables in the `.env` file within the
cloned repository. Replace the following variables with your own values.
```sh
DOCKER_COMPOSE_EDGEHOG_BASE_DOMAIN=edgehog.<YOUR IP ADDRESS>.nip.io
DOCKER_COMPOSE_ASTARTE_BASE_DOMAIN=astarte.<YOUR IP ADDRESS>.nip.io
SEEDS_REALM_PRIVATE_KEY_FILE=~/astarte/test_private.pem
```
The variables `DOCKER_COMPOSE_EDGEHOG_BASE_DOMAIN` and `DOCKER_COMPOSE_ASTARTE_BASE_DOMAIN` have
been given values expecting your Astarte API endpoint to be `api.astarte.<YOUR IP ADDRESS>.nip.io`.
If this was not the case and the Astarte API endpoint was for example `api.astarte.localhost` the
two variables would be `edgehog.localhost` and `astarte.localhost` respectively.

You can now bring up the docker containers required using docker compose.
```sh
docker compose up -d
```

Next, we will popuplate the Edgehog database.
```sh
docker compose exec edgehog-backend bin/edgehog eval Edgehog.Release.seed
```
Edgehog will automatically install its interfaces in our Astarte instance in a process called
reconciliation.
By design reconciliation happens each couple of minutes, to avoid waiting we can manually trigger
a reconciliation using the command below.
```sh
docker compose exec edgehog-backend bin/edgehog rpc "Edgehog.Tenants.Tenant |> Ash.read! |> Enum.each(&Edgehog.Tenants.reconcile_tenant/1)"
```

Finally we will generate a login token with a Python script.
```sh
python3 -m venv .venv
source .venv/bin/activate
pip install -r ./tools/requirements.txt
./tools/gen-edgehog-jwt -k ./backend/priv/repo/seeds/keys/tenant_private.pem -t tenant
```

Now you can login to your Edgehog tennant in the page `http://edgehog.<YOUR IP ADDRESS>.nip.io`.
You should use the token just generated with the tenant name `acme-inc`.

## Configuring the device

Depending on the type of Astarte/Edgehog instances you are using the configuration parameters for
this sample will be slightly different.


- If you are usign a managed Astarte/Edgehog instance such as [astarte.cloud](https://astarte.cloud/)
you will be able to fully take advantage of the secure TLS connection of Astarte.

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
  Where `<DEVICE_ID>` is the device ID of the device you would like to use in the sample, `<HOSTNAME>`
  is the hostname for your Astarte instance, `<REALM_NAME>` is the name of your testing realm and
  `<CREDENTIAL_SECRET>` is the credential secret obtained through the manual registration.

  In addition, the file `ca_certificates.h` should be modified, placing in the `ca_certificate_root`
  array a valid CA certificate in the PEM format.

- If you instead configured Astarte and Edgehog on a local machine using the Astarte quick instance
  guide and the steps above you will need to use unsafe HTTP/MQTT connectivity.

  This is clearly a very bad idea in production devices and should be only enabled for testing and
  development purposes.

  The following entries should be modified in the `proj.conf` file.
  ```conf
  CONFIG_ASTARTE_DEVICE_SDK_HOSTNAME="<HOSTNAME>"
  # Replaces CONFIG_ASTARTE_DEVICE_SDK_HTTPS_CA_CERT_TAG
  CONFIG_ASTARTE_DEVICE_SDK_DEVELOP_USE_NON_TLS_HTTP=y
  # Replaces CONFIG_ASTARTE_DEVICE_SDK_MQTTS_CA_CERT_TAG
  CONFIG_ASTARTE_DEVICE_SDK_DEVELOP_USE_NON_TLS_MQTT=y
  CONFIG_ASTARTE_DEVICE_SDK_CLIENT_CERT_TAG=1
  CONFIG_ASTARTE_DEVICE_SDK_REALM_NAME="<REALM_NAME>"

  CONFIG_EDGEHOG_DEVICE_DEVELOP_DISABLE_OR_IGNORE_TLS=y

  CONFIG_ASTARTE_DEVICE_ID="<DEVICE_ID>"
  CONFIG_ASTARTE_CREDENTIAL_SECRET="<CREDENTIAL_SECRET>"
  ```
  Where `<DEVICE_ID>` is the device ID of the device you would like to use in the sample, `<HOSTNAME>`
  is the hostname for your Astarte instance, `<REALM_NAME>` is the name of your testing realm and
  `<CREDENTIAL_SECRET>` is the credential secret obtained through the manual registration.

  You will most likely replace `<HOSTNAME>` and `<REALM_NAME>` with
  `api.astarte.<YOUR IP ADDRESS>.nip.io` and `test` respectively if you followed the Astarte quick
  instance guide.

## Building the sample

### Over the air (OTA) updates

A central part of Edgehog is that it provides over the air updates for large device fleets.
An [Over-the-Air (OTA) Updates](../../doc/Over The Air (OTA) updates.md) is a method for delivering
firmware updates to remote devices using a network connection. This sample will show you how to
build and deploy a device using `sysbuild` that leverages `MCUboot` capabilities offering out of the
box OTA updates.

### Sysbuild specific settings

MCUboot is automatically included in this sample when built using sysbuild.

This is achieved with a sysbuild specific Kconfig configuration, `sysbuild.conf`.
The `SB_CONFIG_BOOTLOADER_MCUBOOT=y` setting in the sysbuild Kconfig file enables the bootloader
when building with sysbuild.
The `sysbuild/mcuboot.conf` file will be used as an extra configuration fragment that is merged
together with the default configuration files used by MCUboot. `sysbuild/mcuboot.conf` adjusts the
log level in MCUboot, as well as configures MCUboot to prevent downgrades and operate in
upgrade-only mode.

Use ``--sysbuild`` to select the `sysbuild` build infrastructure with `west build`.

By default, sysbuild use the `app.overlay` from MCUboot folder, so if you want MCUboot to use your
board's overlay file, you can pass `mcuboot_DTC_OVERLAY_FILE` parameter to `west build`.
Or you can put your board overlay file in the `sysbuild/boards` directory, and this parameter will
be automatically added by `sysbuild.cmake` contained in this sample.
```sh
west build -b stm32h573i_dk --sysbuild samples/edgehog_app
```
Or:
```sh
west build -b stm32h573i_dk --sysbuild samples/edgehog_app -Dmcuboot_DTC_OVERLAY_FILE=${PWD}/samples/edgehog_app/boards/stm32h573i_dk.overlay
```

## Flashing the sample

Flashing this sample can be done easily using the standard command.
```sh
west flash
```

### Changing runner for nxp boards

The default runner used for nxp boards is `jlink`. To flash with the on board debug probe use:
```sh
west flash --runner=linkserver
```

## More in depth

### The Astarte device

Edgehog leverages [Astarte](https://docs.astarte-platform.org/) to connect the Edgehog device with
the Edgehog cloud instance. This means that each Edgehog device comes with an Astarte device for
free.

It is important to consider that such Astarte device is managed internally by Edgehog and the user
should limit its operations to installing new interfaces and transmitting and receiving on them.
The Edgehog device will manage the connection and instantiate/destroy the device when required.

You can provide your interfaces and callbacks during the creation of the Edgehog device in the
`astarte_device_config_t` struct exactly as you would do with an ordinary Astarte device. If you
want to transmit data with the Astarte device you can obtain its handle using the
`edgehog_device_get_astarte_device` function. It will return an Astarte device handle that can
then be used to transmit as any other Astarte device.
