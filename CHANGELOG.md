<!--
Copyright 2024 SECO Mind Srl
SPDX-License-Identifier: Apache-2.0
-->

# Changelog
All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](http://keepachangelog.com/en/1.1.0/)
and this project adheres to [Semantic Versioning](http://semver.org/spec/v2.0.0.html).

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
