<!---
  Copyright 2024 SECO Mind Srl

  SPDX-License-Identifier: Apache-2.0
-->

# Edgehog Zephyr Device

Edgehog device library for the Zephyr framework.

## Getting started

Before getting started, make sure you have a proper Zephyr development environment.
Follow the official
[Zephyr Getting Started Guide](https://docs.zephyrproject.org/latest/getting_started/index.html).

## Implemented Features

The following information is sent to the remote Edgehog instance:

- OS info
- Hardware info
- System info
- [OTA update using Mcuboot](doc/Over The Air (OTA) updates.md)
- Base image
- Runtime info and Zephyr version
- Reboot command
- System status
- Storage usage
- [Led Behavior](doc/Led feedback.md)
- WiFi APs seen by board antenna
- Battery status API
- Data Rate Control for telemetry

### Adding the module as a dependency to an application

To add this module as a dependency to an application, the application `west.yml` manifest file
should be modified in the following way.
First, a new remote should be added:
```yml
  remotes:
    # ... other remotes ...
    - name: edgehog-device-manager
      url-base: https://github.com/edgehog-device-manager
```
Second, a new entry should be added to the projects list:
```yml
  projects:
    # ... other projects ...
    - name: edgehog-zephyr-device
      remote: edgehog-device-manager
      repo-path: edgehog-zephyr-device.git
      path: edgehog-zephyr-device
      revision: v0.6.0
      import: true
```
Remember to run `west update` after performing changes to the manifest file.

### Adding the module as a standalone application

For development or evaluation purposes it can be useful to add this module as a standalone
application in its own workspace.
This will make it possible to run the module samples without having to set up an additional
application.

#### Creating a new workspace, venv and cloning the example

The first step is to create a new workspace folder and a venv where `west` will reside.
As well as cloning this example repository inside such workspace.

```shell
# Create a venv and install the west tool
mkdir ~/edgehog-zephyr-workspace && cd ~/edgehog-zephyr-workspace
python3 -m venv ./.venv
source ./.venv/bin/activate
pip install west
# Clone the example application repo
git clone https://github.com/edgehog-device-manager/edgehog-zephyr-device.git ./edgehog-zephyr-device
```

#### Initializing the workspace

The second step is to initialize the west workspace, using the example repository as the manifest
repository.

```shell
# initialize my-workspace for the example-application (main branch)
west init -l edgehog-zephyr-device
# update Zephyr modules
west update
```

#### Exporting zephyr environment and installing python dependencies

Perform some final configuration operations.

```shell
west zephyr-export
pip install -r ./zephyr/scripts/requirements.txt
pip install -r ./edgehog-zephyr-device/scripts/requirements.txt
```

#### Building and running a sample application

Follow the [sample specific README](samples/edgehog_app/Sample-application.md).

#### One time configuration

While usually the configuration setting are set in a `prj.conf` file, during development it might
be useful to set them manually.
To do so run the following command:
```shell
west build -t menuconfig
```
This command will require the application to have already been build to function correctly.
After changing the configuration, the application can be flashed directly using `west flash`.

#### Reading serial monitor

The standard configuration for all the samples include settings for printing the logs through the
serial port connected to the host.

#### Unit testing

To run the unit tests use:
```shell
west twister -c -v --inline-logs -p unit_testing -T ./edgehog-zephyr-device/tests
```

### Code formatting

The code in this module is formatted using `clang-format`.
An extension command for `west` has been created to facilitate formatting, `clang-format`
should be installed before running this command.
Run `west format --help` to learn about the formatting options.

### Static code analysis

[CodeChecker](https://codechecker.readthedocs.io/en/latest/) is the one of the
[various](https://docs.zephyrproject.org/latest/develop/sca/index.html) natively supported static
analysis tools in `west`.
It can be configured to run different static checkers, such as `clang-tidy` and `cppcheck`.

Install the python dependencies the extensions with the following command:
```bash
pip install -r ~/zephyr-workspace/edgehog-zephyr-device/scripts/requirements.txt
```
In addition, `clang-tidy` should be installed, if not already present on the system:
```bash
wget https://apt.llvm.org/llvm.sh
chmod +x llvm.sh
sudo ./llvm.sh $LLVM_VERSION
sudo update-alternatives --install /bin/clang-tidy clang-tidy /bin/clang-tidy-$LLVM_VERSION 100
```
Where `$LLVM_VERSION` is LLVM version to install.

An extension command for `west` has been created to facilitate running static analysis with
`clang-tidy`.
Run `west static --help` to read what this command performs.

Note that this extension is only available when using this repository as a stand-alone project.

### Build doxygen documentation

The dependencies for generating the doxygen documentation are:
- `doxygen <= 1.9.4`
- `graphviz`

An extension command for `west` has been created to facilitate generating the documentation.
Run `west docs --help` for more information on how to generate the documentation.

## VS Code integration

Here are some quick tips to configure VS Code with a Zephyr workspace.
Make sure the following extensions are installed:
- C/C++ Extension Pack
- Python

Create the file `./.vscode/settings.json` and fill it with the following content:
```json
{
    // Hush CMake
	"cmake.configureOnOpen": false,

    // Configure Python so that each new terminal will activate the venv
    // defaultInterpreterPath is not always be picked up, backup cmd: 'Python: select interpreter'
    "python.defaultInterpreterPath": "${workspaceFolder}/.venv/bin/python",
    "python.terminal.activateEnvironment": true,
    "python.terminal.activateEnvInCurrentTerminal": true,

    // IntelliSense
    "C_Cpp.default.compilerPath": "${userHome}/zephyr-sdk-0.16.3/arm-zephyr-eabi/bin/arm-zephyr-eabi-gcc",
    "C_Cpp.default.compileCommands": "${workspaceFolder}/build/compile_commands.json",
    "C_Cpp.autoAddFileAssociations": false,
    "C_Cpp.debugShortcut": false
}
```
The first setting will avoid noisy popups from the CMake extension.
The python related settings will ensure that each time the integrated terminal is open, the
venv containing `west` will be automatically activated.
The C/C++ settings will configure intellisense. Those setting might be slightly different
for your specific Zephyr SDK installation as versions might change.
Also, the compile commands path assumes `west build` gets run from the root of the zephyr
workspace.

One more step is required. The python extension does not load at startup when opening VS Code
unless some
[specific activation](https://github.com/microsoft/vscode-python/blob/a5ab3b8c05e84670176aef8fe246ff0164707ac4/package.json#L66-L78)
events happens.
This means that for a zephyr workspace, that by default does not have any of such activation
events, the extension does not load and when opening a terminal the venv does not activate.
To fix this you can create an empty `mspythonconfig.json` file that will serve as an activation
event. However, note that this file might interfere when using Pyright in your project.
