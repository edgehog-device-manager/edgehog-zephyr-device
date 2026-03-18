#!/bin/bash

# (C) Copyright 2026, SECO Mind Srl
#
# SPDX-License-Identifier: Apache-2.0

# Check if an argument was provided
if [[ $# -eq 0 ]]; then
  echo "Please provide an argument."
  exit 1
fi

while true; do
  echo "Installing the interfaces using astartectl"
  astartectl realm-management interfaces sync $1/*.json --non-interactive
  sleep 5
  echo "Checking the installed interfaces."
  installed_interfaces=$(astartectl realm-management interfaces ls)
  missing_interface=false
  for file in $1/*.json; do
    interface_name=$(basename "$file" .json)
    if ! grep -q "$interface_name" <<< "$installed_interfaces"; then
      echo "Error: Interface $interface_name not found in $installed_interfaces"
      missing_interface=true
    fi
  done
  if [[ "$missing_interface" == "true" ]]; then
    continue
  else
    echo "All interfaces have been installed correctly"
    break
  fi
done
