#!/usr/bin/env bash
set -euo pipefail

if [[ "$(uname -m)" != "aarch64" ]]; then
  printf 'ERROR: run the datapath on the BlueField Arm side (expected aarch64).\n' >&2
  exit 1
fi

for module in doca-common doca-flow doca-dpdk-bridge doca-argp libdpdk; do
  if ! pkg-config --exists "${module}"; then
    printf 'ERROR: pkg-config module not found: %s\n' "${module}" >&2
    exit 1
  fi
done

printf 'doca-common: %s\n' "$(pkg-config --modversion doca-common)"
printf 'doca-flow:   %s\n' "$(pkg-config --modversion doca-flow)"

grep -E 'HugePages_Total|HugePages_Free|Hugepagesize|Hugetlb' /proc/meminfo

if ! mountpoint -q /dev/hugepages; then
  printf 'ERROR: /dev/hugepages is not mounted.\n' >&2
  exit 1
fi

if [[ "$(awk '/HugePages_Free:/ {print $2}' /proc/meminfo)" -eq 0 ]]; then
  printf 'ERROR: no free hugepages are available.\n' >&2
  exit 1
fi

printf '\nVisible NVIDIA devices:\n'
lspci -Dnn -d 15b3: || true

printf '\nRepresentor ports:\n'
devlink port show || true

printf '\nEnvironment preflight passed.\n'
