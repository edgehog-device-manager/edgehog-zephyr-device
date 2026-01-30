<!--
Copyright 2024 SECO Mind Srl
SPDX-License-Identifier: Apache-2.0
-->

# Changelog
All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](http://keepachangelog.com/en/1.1.0/)
and this project adheres to [Semantic Versioning](http://semver.org/spec/v2.0.0.html).

## [0.9.0] - 2025-10-23
### Added
- In the samples, support for the NXP FRDM RW612 board, including WiFi functionality.

### Changed
- Update the Astarte device SDK to v0.9.0.
- Support for Zephyr 4.2.1.

### Removed
- The `wifi_scan.h` header as the wifi scan functionality is now embedded within the edgehog
  device.

## [0.8.0] - 2025-03-26
### Added
- The `edgehog_device_poll` function that should be called periodically throughout the lifetime of
  an Edgehog device.
- The `edgehog_device_get_astarte_device` function that can be used to fetch a handle to the
  Astarte device internal to Edgehog. The user can use such device to transmit and receive data on
  user defined interfaces.
- The `edgehog_device_get_astarte_error` function that can be used by the user to fetch the last
  recorded Astarte device error.

### Changed
- Update the Astarte device SDK to v0.8.0.
- Support for Zephyr 4.1.0.
- Encapsulated the Astarte device within the Edgehog device. The Edgehog device will not take
  full ownership of the Astarte device, handling the connectivity status of the Astarte device
  with the Astarte cloud instance. The Astarte device is still available to the user to transmit
  and receive data on user defined interfaces.
- The configuration now accepts an Astarte device configuration struct instead of an already
  initialized Astarte device.

## [0.7.0] - 2024-12-19
### Added
- The function `edgehog_device_stop` that allows stopping the Edgehog device. Including the
  telemetry services.
- The result value `EDGEHOG_RESULT_TELEMETRY_STOP_TIMEOUT`. This is returned when Edgehog failed
  to stop within the user defined timeout.
- The public definition of the zbus channel `edgehog_ota_chan`. That can be used to interact with
  the Edgehog device during OTA updates.
- The zbus event `EDGEHOG_OTA_PENDING_REBOOT_EVENT`. This event is sent by the Edgehog device on
  the zbus when an OTA has been received, correctly flashed in the secondary partition, and the
  device is ready for a reboot.
- The zbus event `EDGEHOG_OTA_CONFIRM_REBOOT_EVENT`. This event can be used to reply to a
  `EDGEHOG_OTA_PENDING_REBOOT_EVENT` triggering an immediate reboot of the device, skipping the
  usual wait time that the Edgehog device uses for the reboot.
- Support for Zephyr `4.0` for the sample.
- The `stm32h573i_dk` board into the standard boards for the sample.
- A new readme for the sample specific for the NXP Application Hub.

### Changed
- `edgehog_device_telemetry_config_t` has been renamed to `edgehog_telemetry_config_t` to be
  coherent with the library naming scheme.
- `telemetry_type_t` has been renamed to `edgehog_telemetry_type_t` to be coherent with the library
  naming scheme.
- The sample has been reorganized to be easier to understand.
- The Astarte device has been updated to the latest `0.7` version.

### Removed
- The WiFi driver in the sample. Currently no board with WiFi is included in the standard
  boards for the Edgehog sample.

### Fixed
- Prevent illegal memory acces in the function `edgehog_telemetry_destroy`.

## [0.6.0] - 2024-09-12
### Added
- Add support for `io.edgehog.devicemanager.SystemStatus` interface.
- Add support for `io.edgehog.devicemanager.StorageUsage` interface.
- Add support for `io.edgehog.devicemanager.LedBehavior` interface.
- Add support for `io.edgehog.devicemanager.WiFiScanResults` interface.
- Add support for `io.edgehog.devicemanager.BatteryStatus` interface.
- Add support for `io.edgehog.devicemanager.config.Telemetry` interface.

### Changed
- Updated the Astarte device SDK to `v0.7.1`.
- Updated Zephyr to `v3.7.0`.

## [0.5.0] - 2024-05-13
### Added
- Add support for `io.edgehog.devicemanager.BaseImage` interface.
- Add support for `io.edgehog.devicemanager.Commands`. interface.
- Add support for `io.edgehog.devicemanager.RuntimeInfo` interface.

## [0.5.0-alpha] - 2024-04-26
### Added
- Initial release, supports OTA, hardware info, system info and os info.
