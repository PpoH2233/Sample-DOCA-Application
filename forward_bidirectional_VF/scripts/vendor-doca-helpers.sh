#!/usr/bin/env bash
set -euo pipefail

DOCA_ROOT="${DOCA_ROOT:-/opt/mellanox/doca}"
PROJECT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
DEST_DIR="${PROJECT_DIR}/vendor/doca"

mkdir -p "${DEST_DIR}"

cp "${DOCA_ROOT}/samples/common.c" "${DEST_DIR}/common.c"
cp "${DOCA_ROOT}/samples/common.h" "${DEST_DIR}/common.h"
cp "${DOCA_ROOT}/samples/common_utils.h" "${DEST_DIR}/common_utils.h"

cp "${DOCA_ROOT}/samples/doca_flow/flow_common.c" "${DEST_DIR}/flow_common.c"
cp "${DOCA_ROOT}/samples/doca_flow/flow_common.h" "${DEST_DIR}/flow_common.h"
cp "${DOCA_ROOT}/samples/doca_flow/flow_switch_common.c" "${DEST_DIR}/flow_switch_common.c"
cp "${DOCA_ROOT}/samples/doca_flow/flow_switch_common.h" "${DEST_DIR}/flow_switch_common.h"

cp "${DOCA_ROOT}/applications/common/dpdk_utils.c" "${DEST_DIR}/dpdk_utils.c"
cp "${DOCA_ROOT}/applications/common/dpdk_utils.h" "${DEST_DIR}/dpdk_utils.h"
cp "${DOCA_ROOT}/applications/common/utils.h" "${DEST_DIR}/utils.h"

printf 'Imported DOCA helper sources from %s\n' "${DOCA_ROOT}"
