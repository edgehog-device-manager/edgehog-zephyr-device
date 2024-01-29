# (C) Copyright 2024, SECO Mind Srl
#
# SPDX-License-Identifier: Apache-2.0

"""docs.py

West extension that can be used to build doxygen documentation for this project.

Checked using pylint with the following command (pip install pylint):
python -m pylint --rcfile=./scripts/.pylintrc ./scripts/*.py
Formatted using black with the following command (pip install black):
python -m black --line-length 100 ./scripts/*.py
"""

import os
import subprocess
from pathlib import Path

from west.commands import WestCommand  # your extension must subclass this

# from west import log  # use this for user output

static_name = "docs"
static_help = "Generate documentation (doxygen)"
static_description = """Convenience wrapper to buld documentation with doxygen."""


class WestCommandDocs(WestCommand):
    """Extension of the WestCommand class, specific for this command."""

    def __init__(self):
        super().__init__(static_name, static_help, static_description)

    def do_add_parser(self, parser_adder):
        """
        This function can be used to add custom options for this command.

        Allows you full control over the type of argparse handling you want.

        Parameters
        ----------
        parser_adder : Any
            Parser adder generated by a call to `argparse.ArgumentParser.add_subparsers()`.

        Returns
        -------
        argparse.ArgumentParser
            The argument parser for this command.
        """
        parser = parser_adder.add_parser(self.name, help=self.help, description=self.description)

        # Add some options using the standard argparse module API.
        parser.add_argument(
            "-c", "--clean", help="clean previous documentation", action="store_true"
        )

        return parser  # gets stored as self.parser

    # pylint: disable-next=arguments-renamed,unused-argument
    def do_run(self, args, unknown_args):
        """
        Function called when the user runs the custom command, e.g.:

          $ west clean

        Parameters
        ----------
        args : Any
            Arguments pre parsed by the parser defined by `do_add_parser()`.
        unknown_args : Any
            Extra unknown arguments.
        """
        module_path = Path(self.topdir).joinpath("edgehog-zephyr-device")
        if args.clean:
            subprocess.run(
                "make -C doc clean",
                shell=True,
                cwd=module_path,
                timeout=60,
                check=True,
                env=dict(os.environ, EDGEHOG_DEVICE_BASE=f"{module_path}"),
            )
        subprocess.run(
            "make -C doc doxygen",
            shell=True,
            cwd=module_path,
            timeout=60,
            check=True,
            env=dict(os.environ, EDEGHOG_DEVICE_BASE=f"{module_path}"),
        )
