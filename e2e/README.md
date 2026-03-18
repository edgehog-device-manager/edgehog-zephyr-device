<!--
Copyright 2024 SECO Mind Srl

SPDX-License-Identifier: Apache-2.0
-->

# End to end tests local setup

The end-to-end tests are configured to run in a GitHub workflow, which sets up a dedicated environment.
However, it is possible to run them locally for testing purposes.
The end-to-end tests are a Zephyr application that can be built only for the `native_sim` board.
Any other board or architecture is not supported and the build process will fail.

## Connection to astarte cloud

This repo includes an Astarte cloud root certificate. If you want to connect to a different Astarte
cluster, you must switch the certificate with your own.
Additionally, the `net-setup` script included in Zephyr's `net-tools` will need to be started
**with root privileges** and left running for the duration of the tests.

```sh
./net-setup.sh --config nat.conf
```

> :information_source: **CERTIFICATE path** In the Kconfig you'll find `CONFIG_E2E_ASTARTE_TLS_CERTIFICATE_PATH`
> You can update the value of this option to include a custom certificate
> that will be used for the https and mqtts connection.

Before running the test application you must configure it properly:

Open `prj.conf` or create your `private.conf` file with the following content:

```prj
CONFIG_ASTARTE_DEVICE_SDK_HOSTNAME="..."
CONFIG_ASTARTE_DEVICE_SDK_REALM_NAME="..."
CONFIG_E2E_ASTARTE_DEVICE_ID="..."
CONFIG_E2E_ASTARTE_CREDENTIAL_SECRET="..."
CONFIG_ASTARTE_DEVICE_SDK_PAIRING_JWT="..."
CONFIG_E2E_APPENGINE_TOKEN="..."
CONFIG_E2E_APPENGINE_URL="..."
```

Fill in the required values and then run:

```sh
west build -b native_sim -t run e2e
```

## Connection to a local astarte instance

If you want to connect to a local astarte instance you'll have to change the dns server.
You have to use the one created as part of the `net-setup` script (currently a dnsmasq instance).
You also have to configure additional entries in the dnsmasq configuration to return
the correct zeth interface address.

Open the `net-tools` directory as cloned from the zephyr organization, add to `dnsmasq_nat.conf`:
```plain
address=/api.astarte.localhost/192.0.2.2
address=/broker.astarte.localhost/192.0.2.2
```

You can now start the net-setup script as described above.

Open `prj.conf` or create your private `private.conf` file:

```prj
CONFIG_ASTARTE_DEVICE_SDK_HOSTNAME="api.astarte.localhost"
CONFIG_ASTARTE_DEVICE_SDK_REALM_NAME="..."
CONFIG_E2E_ASTARTE_DEVICE_ID="..."
CONFIG_E2E_ASTARTE_CREDENTIAL_SECRET="..."
CONFIG_ASTARTE_DEVICE_SDK_PAIRING_JWT="..."
CONFIG_E2E_APPENGINE_TOKEN="..."
CONFIG_E2E_APPENGINE_URL="http://api.astarte.localhost/appengine"
# The dns used must be the one of the zeth interface
# Since we are injecting there the correct domain name resolutions
CONFIG_DNS_SERVER1="192.0.2.2:53"
# These last two lines disable tls
CONFIG_ASTARTE_DEVICE_SDK_DEVELOP_USE_NON_TLS_HTTP=y
CONFIG_ASTARTE_DEVICE_SDK_DEVELOP_USE_NON_TLS_MQTT=y
```

This setup works as long as you don't want to use tls.
The setup with tls is a bit more involved and requires editing the astarte docker-compose.
Also remember to load your locally generated certificate by using the `CERTIFICATE` configuration file.

```prj
CERTIFICATE_PATH = path/to/your/certificate
```
