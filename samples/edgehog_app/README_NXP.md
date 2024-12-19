<!--
Copyright 2024 SECO Mind Srl

SPDX-License-Identifier: Apache-2.0
-->

# Astarte IoT devices using Zephyr RTOS

[![License badge](https://img.shields.io/badge/License-Apache%202.0-red)](https://www.apache.org/licenses/LICENSE-2.0.txt)
![Language badge](https://img.shields.io/badge/Language-C-yellow)
![Language badge](https://img.shields.io/badge/Language-C++-yellow)
[![Board badge](https://img.shields.io/badge/Board-EVK&ndash;MIMXRT1064-blue)](https://www.nxp.com/pip/MIMXRT1064-EVK)
[![Category badge](https://img.shields.io/badge/Category-CLOUD%20CONNECTED%20DEVICES-yellowgreen)](https://mcuxpresso.nxp.com/appcodehub?search=cloud%20connected%20devices)
[![Toolchain badge](https://img.shields.io/badge/Toolchain-VS%20CODE-orange)](https://github.com/nxp-mcuxpresso/vscode-for-mcux/wiki)

Edgehog is an open source device management platform focused on device fleet management.
It eases the management of your connected embedded systems, by making all their information
available remotely. Furthermore, it enables remote updates and system configuyration.

This demo application shows how to create an Edgehog device using the Zephyr RTOS framework on
the i.MX RT1064 evaluation kit.

For more information on Astarte check out the
[Astarte documentation](https://docs.astarte-platform.org/).

## Table of Contents
1. [Software](#step1)
2. [Hardware](#step2)
3. [Setup](#step3)
4. [Results](#step4)
5. [Support](#step5)
6. [Release Notes](#step6)

## 1. Software<a name="step1"></a>

This demo application is intended to be built using VS Code through the
[MCUXpresso](https://www.nxp.com/design/design-center/software/embedded-software/mcuxpresso-for-visual-studio-code:MCUXPRESSO-VSC)
extension. It furthermore requires the Zephyr developer dependencies for MCUXpresso to be
installed, this can be done through the
[MCUXpresso Installer](https://github.com/nxp-mcuxpresso/vscode-for-mcux/wiki/Dependency-Installation).

The application demonstrates the use of the
[Edgehog device for Zephyr](https://github.com/edgehog-device-manager/edgehog-zephyr-device) at
version **0.7.0**.

Every Edgehog device relies internally on the [Astarte](https://docs.astarte-platform.org/) plaftorm
to enable remote communication. In this application we will rely on the
[Astarte quick instance](https://docs.astarte-platform.org/device-sdks/common/astarte_quick_instance.html)
tutorial. Ensure all its requirements are satisfied and dependencies installed.

## 2. Hardware<a name="step2"></a>

This demo application is provided for use with the NXP MIMXRT1064-EVK board.
However, the application can be run on a wide range of boards with minimal configuration changes.
The minimum requirements for this application are the following:
- At least 500KB of RAM
- At least 500KB of FLASH storage
- An Ethernet interface

Additionally, each board should provide a dedicated flash partition for Astarte and one for Edgehog.

## 3. Setup<a name="step3"></a>

### 3.1 Step 1

As mentioned above Edgehog relies on Astarte for the communication between device and cloud. As a
consequence you will need an Astarte instance to which the device will connect.
The easiest way to set up one is to follow the
[Astarte quick instance](https://docs.astarte-platform.org/device-sdks/common/astarte_quick_instance.html)
tutorial.
The tutorial guides you through setting up an Astarte instance on a local host machine. Both the
host machine and the device board should be connected to the same LAN.

Make sure you do not lose the following information:
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

### 3.2 Step 2

In this step we will configure a local Edgehog instance on the same host machine that is running
the Astarte instance.

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

### 3.3 Step 3

We will now configure the application for your Astarte and Edgehog instances.
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
Where `<HOSTNAME>` is the hostname for your Astarte instance, `<REALM_NAME>` is the name of your
Astarte realm, `<DEVICE_ID>` is the device ID of the Astarte device you registered above and
`<CREDENTIAL_SECRET>` is the credential secret obtained through registration process.

Since you followed the Astarte quick instance guide `<HOSTNAME>` and `<REALM_NAME>` can be replaced
with `api.astarte.<YOUR IP ADDRESS>.nip.io` and `test` respectively.

## 4. Results<a name="step4"></a>

Upon running the code the demo application will display on the serial monitor its progression.
The device will first connect to Astarte using the credential secret.
Once the device is connected it will appear in the Edgehog dashoard and the user will be able to
interact with it.
For example an OTA update can be performed. You will first need to locate the binary generated
during the build process and then trigger the update procedure using the dashboard.

## 5. Support<a name="step5"></a>

For more information about Edgehog check out the
[Edgehog documentation](https://docs.edgehog.io/0.9/intro_user.html).
Or check out our projects on [GitHub](https://github.com/edgehog-device-manager).

Questions regarding the content/correctness of this example can be filed as Issues within this
GitHub repository.

## 6. Release Notes<a name="step6"></a>
| Version | Description / Update                           | Date                        |
|:-------:|------------------------------------------------|----------------------------:|
| 1.0     | Initial release on Application Code Hub        | December 11<sup>rd</sup> 2024 |

## Licensing

This project is licensed under Apache 2.0 license. Find our more at
[www.apache.org](https://www.apache.org/licenses/LICENSE-2.0.).
