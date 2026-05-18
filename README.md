<!---
  Copyright 2026 SECO Mind Srl

  SPDX-License-Identifier: Apache-2.0
-->

# Edgehog Zephyr Device

A Zephyr RTOS library that enables embedded devices to connect to an [Edgehog](https://edgehog.io) instance for remote device management. It allows your device to report metrics and status information to Edgehog, receive and execute remote commands, and perform over-the-air firmware updates and file transfers — all from within a standard Zephyr application.

## Features

The following capabilities are provided out of the box:

- Device information: OS info, hardware info, base image, and runtime info
- System monitoring: system status, storage usage, and Zephyr version
- Network: WiFi access points visible to the board antenna
- Battery status API
- Remote reboot command
- [OTA update using Mcuboot](doc/architecture/OTA Updates.md)
- [File transfer](doc/features/File Transfer.md)
- [LED behavior feedback](doc/architecture/Led Feedback.md)
- Data Rate Control for telemetry

## Installation

There are two ways to use this library: as a dependency within an existing Zephyr application, or as a standalone workspace for development and evaluation.

### As a dependency in an existing application

To integrate the library into your Zephyr application, update your `west.yml` manifest file in two steps.

First, add the Edgehog remote:

```yml
remotes:
  # ... other remotes ...
  - name: edgehog-device-manager
    url-base: https://github.com/edgehog-device-manager
```

Then add the project entry:

```yml
projects:
  # ... other projects ...
  - name: edgehog-zephyr-device
    remote: edgehog-device-manager
    repo-path: edgehog-zephyr-device.git
    path: edgehog-zephyr-device
    revision: v0.9.0
    import: true
```

After editing the manifest, run `west update` to fetch the module.

### As a standalone application

This setup is recommended for development or evaluation, as it lets you run the included samples without needing a separate application project.

**1. Create a workspace and install west**

```shell
mkdir ~/edgehog-zephyrproject && cd ~/edgehog-zephyrproject
python3 -m venv .venv
source .venv/bin/activate
pip install west
```

**2. Initialize the workspace**

```shell
west init -m git@github.com:edgehog-device-manager/edgehog-zephyr-device --mr v0.9.0
west update
```

**3. Export the Zephyr environment and install Python dependencies**

```shell
west zephyr-export
west packages pip --install
west packages pip --install -- -r ./edgehog-zephyr-device/scripts/requirements.txt
```

**4. Build and run a sample**

See the Edgehog sample for board-specific build instructions.

## Development

### Unit tests

```shell
west twister -c -v --inline-logs -p unit_testing -T ./edgehog-zephyr-device/tests
```

### Extension commands

This module provides several `west` extension commands to streamline common development tasks.

**Code formatting** — The codebase is formatted with `clang-format`. Make sure it is installed, then run:

```shell
west astarte-format --help
```

**Static analysis** — [CodeChecker](https://codechecker.readthedocs.io/en/latest/) is used for static analysis, configured to run `clang-tidy` and `cppcheck`. First install the dependencies:

```shell
pip install -r ~/zephyr-workspace/edgehog-zephyr-device/scripts/requirements.txt
```

If `clang-tidy` is not already installed:

```shell
wget https://apt.llvm.org/llvm.sh
chmod +x llvm.sh
sudo ./llvm.sh $LLVM_VERSION
sudo update-alternatives --install /bin/clang-tidy clang-tidy /bin/clang-tidy-$LLVM_VERSION 100
```

Replace `$LLVM_VERSION` with the desired LLVM version. Then run:

```shell
west astarte-static --help
```

Note that this command is only available when using the repository as a standalone project.

### API documentation (Doxygen)

The API reference documentation is generated locally with Doxygen. The required dependencies are `doxygen` (version ≤ 1.9.4) and `graphviz`. Once installed, use the provided extension command:

```shell
west astarte-docs --help
```

This will generate the documentation locally in your workspace, since there is currently no hosted version available online.
