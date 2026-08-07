#!/usr/bin/env bash

set -euo pipefail

readonly MODULE_NAME="virtusb"
readonly DEFAULT_BUILD_DIR="build/dev-gcc"

build_dir="${1:-${DEFAULT_BUILD_DIR}}"
module_path="${build_dir}/kernel/${MODULE_NAME}.ko"

if [[ $# -gt 0 ]]; then
   shift
fi

if [[ ! -f "${module_path}" ]]; then
   echo "Error: Kernel module not found:" >&2
   echo "  ${module_path}" >&2
   echo >&2
   echo "Build the module first or pass the build directory:" >&2
   echo "  $0 build/dev-gcc [module_parameter=value ...]" >&2
   exit 1
fi

if lsmod | awk '{ print $1 }' | grep -qx "${MODULE_NAME}"; then
   echo "Error: Module '${MODULE_NAME}' is already loaded." >&2
   exit 1
fi

echo "Loading ${module_path}"

if [[ $# -gt 0 ]]; then
   echo "Module parameters: $*"
fi

sudo insmod "${module_path}" "$@"

echo
echo "Module '${MODULE_NAME}' loaded successfully."
echo
echo "Recent kernel messages:"
sudo journalctl --dmesg --no-pager -n 20
