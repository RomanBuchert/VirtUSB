#!/usr/bin/env bash

set -euo pipefail

readonly MODULE_NAME="virtusb"

if ! lsmod | awk '{ print $1 }' | grep -qx "${MODULE_NAME}"; then
   echo "Error: Module '${MODULE_NAME}' is not loaded." >&2
   exit 1
fi

echo "Unloading module '${MODULE_NAME}'"
sudo rmmod "${MODULE_NAME}"

echo
echo "Module '${MODULE_NAME}' unloaded successfully."
echo
echo "Recent kernel messages:"
sudo journalctl --dmesg --no-pager -n 20
