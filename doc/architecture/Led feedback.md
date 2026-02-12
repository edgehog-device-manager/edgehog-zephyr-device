<!--
Copyright 2024 SECO Mind Srl

SPDX-License-Identifier: Apache-2.0
-->

# Led feedback
Edgehog devices provide visual feedback functionality using an onboard LED. It can be enabled at
compilation time by setting the `edgehog-led` alias to an existing LED in the board overlay file.
The visual feedback can be triggered by a publish from the Astarte cluster to the dedicated LED
interface. A dedicated thread will be spawned with a fixed lifetime of 1 minute.

## Device tree overlay alias example
You can add aliases to your devicetree using overlays: an alias is just a property of the /aliases
node. For example:

```
/ {
     aliases {
             edgehog-led = &blue_led;
     };
};
```
