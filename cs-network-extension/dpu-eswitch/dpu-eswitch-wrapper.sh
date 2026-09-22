#!/bin/bash
# Licensed to the Apache Software Foundation (ASF) under one
# or more contributor license agreements.  See the NOTICE file
# distributed with this work for additional information
# regarding copyright ownership.  The ASF licenses this file
# to you under the Apache License, Version 2.0 (the
# "License"); you may not use this file except in compliance
# with the License.  You may obtain a copy of the License at
#
#   http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing,
# software distributed under the License is distributed on an
# "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
# KIND, either express or implied.  See the License for the
# specific language governing permissions and limitations
# under the License.

##############################################################################
# dpu-eswitch-wrapper.sh  (extension: dpu-eswitch)
#
# CloudStack NetworkOrchestrator wrapper for NVIDIA BlueField-3 DPUs running
# the `eswitch-management` DOCA application.  Forwarded here by
# dpu-eswitch.sh over SSH; translates CloudStack network-extension commands
# into eswitch-management control commands (Unix socket protocol).
#
# Scope (phase 1): isolated guest networks with SourceNat + Gateway.
# Guest VMs connect through SR-IOV VFs of the BlueField passed by KVM; the
# eSwitch FDB learns guest MACs, so no per-NIC programming is required.
#
# ID scheme: vswitch id = virtual router id = guest VLAN tag.  Networks that
# do not have a VLAN yet use the reserved 4096..32767 range; public WAN VLANs
# use 32769..36862.  Keeping these ranges disjoint avoids forwarding-domain
# aliasing when a network later adds a p0 trunk.
#
# physical-network extension details (registration):
#   hosts, host, port, username, password, sshkey  - consumed by dpu-eswitch.sh
#   eswitchctl.path - eswitchctl binary (optional; falls back to python3/socat)
#   control.socket  - override control socket path (optional)
#   vf.pool         - VF indexes pre-attached to each network vSwitch
#                     ("all", "1-10", "1-6,10-20"; empty = manual attach)
#   uplink.port     - legacy direct VF uplink selector (not used by VLAN WAN)
#                     VLAN WAN always resolves the parent/uplink (p0) port
#   nat.port.range  - SNAT/PAT port range (default 20000-60999)
#
# Exit codes: 0 success, 1 usage/configuration error.
##############################################################################

set -u

_WRAPPER_EXT_DIR="$(basename "$(dirname "$(readlink -f "$0" 2>/dev/null || echo "$0")")")"
LOG_FILE="${DPU_ESWITCH_LOG_FILE:-/var/log/cloudstack/extensions/${_WRAPPER_EXT_DIR}/${_WRAPPER_EXT_DIR}.log}"
STATE_DIR="${DPU_ESWITCH_STATE_DIR:-/var/lib/cloudstack/${_WRAPPER_EXT_DIR}}"

GLOBAL_LOCKFILE=""
PAYLOAD_FILE=""
TIMEOUT_SECONDS="60"
_ESW_SOCKET="/run/eswitch-management/control.sock"
_ESW_TRANSPORT=""
_ESWCTL_BIN=""
PHYS_DETAILS="${CS_PHYSICAL_NETWORK_EXTENSION_DETAILS:-{\}}"
EXTENSION_DETAILS="${CS_NETWORK_EXTENSION_DETAILS:-{\}}"
NAT_PORT_RANGE="20000-60999"
VF_POOL=""
VSWITCH_ID=""
VR_ID=""

# ---------------------------------------------------------------------------
# Logging
# ---------------------------------------------------------------------------

log() {
    local ts
    ts=$(date '+%Y-%m-%d %H:%M:%S')
    printf '[%s] %s\n' "${ts}" "$*" >> "${LOG_FILE}" 2>/dev/null || true
}

die() {
    log "ERROR: $*"
    printf 'ERROR: %s\n' "$*" >&2
    release_lock
    exit 1
}

# ---------------------------------------------------------------------------
# JSON helpers (no jq dependency)
# ---------------------------------------------------------------------------

json_get() {
    printf '%s' "$1" | grep -o "\"$2\":\"[^\"]*\"" | cut -d'"' -f4 || true
}

_payload_json_get() {
    python3 - "$1" "$2" <<'PY'
import json, sys
with open(sys.argv[1], encoding="utf-8") as fh:
    data = json.load(fh)
cur = data
for part in sys.argv[2].split("."):
    if isinstance(cur, dict):
        cur = cur.get(part)
    else:
        cur = None
    if cur is None:
        break
if cur is None:
    print("")
elif isinstance(cur, (dict, list)):
    print(json.dumps(cur, separators=(",", ":")))
else:
    print(str(cur))
PY
}

# ---------------------------------------------------------------------------
# Lock / state helpers
# ---------------------------------------------------------------------------

ensure_dirs() {
    mkdir -p "${STATE_DIR}" "$(dirname "${LOG_FILE}")" 2>/dev/null || true
}

acquire_lock() {
    local network_id="$1"
    mkdir -p "${STATE_DIR}" 2>/dev/null || true
    GLOBAL_LOCKFILE="${STATE_DIR}/lock-network-${network_id}"
    log "acquire_lock: ${GLOBAL_LOCKFILE}"
    exec 200>"${GLOBAL_LOCKFILE}"
    flock -w 30 200 || die "Failed to acquire lock for network ${network_id}"
}

release_lock() {
    exec 200>&- 2>/dev/null || true
    if [ -n "${GLOBAL_LOCKFILE}" ]; then
        rm -f "${GLOBAL_LOCKFILE}" 2>/dev/null || true
        GLOBAL_LOCKFILE=""
    fi
}
trap 'release_lock' EXIT

net_state_dir() { printf '%s/network-%s' "${STATE_DIR}" "${NETWORK_ID}"; }

state_set() {
    local nsd
    nsd=$(net_state_dir)
    mkdir -p "${nsd}" 2>/dev/null || true
    printf '%s\n' "$2" > "${nsd}/$1" 2>/dev/null || true
}

state_get() {
    local f
    f="$(net_state_dir)/$1"
    [ -f "${f}" ] && cat "${f}"
    return 0
}

# Overlay persisted state onto empty fields (payload wins, state fills gaps).
overlay_state() {
    local v val
    for v in VLAN GATEWAY CIDR PUBLIC_IP PUBLIC_VLAN PUBLIC_GATEWAY PUBLIC_CIDR; do
        val=$(state_get "$(echo "${v}" | tr 'A-Z' 'a-z')")
        if [ -n "${val}" ] && [ -z "${!v:-}" ]; then
            eval "${v}=\"\${val}\""
        fi
    done
}

# ---------------------------------------------------------------------------
# Validation helpers
# ---------------------------------------------------------------------------

normalize_vlan() { printf '%s' "${1#vlan://}"; }

is_numeric() {
    [ -n "$1" ] && printf '%s' "$1" | grep -qE '^[0-9]+$'
}

is_ipv4() {
    printf '%s' "$1" | grep -qE '^[0-9]{1,3}\.[0-9]{1,3}\.[0-9]{1,3}\.[0-9]{1,3}$'
}

# vswitch id = virtual router id = guest VLAN tag (1..4094); fall back to a
# deterministic id in 4096..32767 when no VLAN is assigned yet.  That range
# is deliberately disjoint from guest VLAN and public-WAN forwarding domains.
derive_vswitch_id() {
    local vlan="$1" net="$2" fb
    if is_numeric "${vlan}" && [ "${vlan}" -ge 1 ] && [ "${vlan}" -le 4094 ]; then
        printf '%s' "${vlan}"
        return 0
    fi
    if is_numeric "${net}"; then
        fb=$(( net % 28672 + 4096 ))
        log "WARN: unusable vlan '${vlan}' - deriving vswitch id ${fb} from network_id ${net}"
        printf '%s' "${fb}"
        return 0
    fi
    return 1
}

# Public VLANs get a separate forwarding domain so the same p0 trunk can be a
# member of many WAN broadcast domains without colliding with guest VLAN IDs.
derive_public_vswitch_id() {
    local vlan="$1"
    is_numeric "${vlan}" && [ "${vlan}" -ge 1 ] && [ "${vlan}" -le 4094 ] || return 1
    printf '%s' $((32768 + vlan))
}

# Deterministic, unicast, unique-per-VR RIF MAC.
rif_mac() {
    printf '02:00:00:65:%02x:%02x' $(( $1 >> 8 & 255 )) $(( $1 & 255 ))
}

vf_pool_match() {
    local spec="$1" vf="$2" item a b
    [ -n "${spec}" ] || return 1
    [ "${spec}" = "all" ] && return 0
    local IFS=','
    for item in ${spec}; do
        case "${item}" in
            *-*)
                a="${item%-*}"
                b="${item#*-}"
                if is_numeric "${a}" && is_numeric "${b}" && \
                   [ "${vf}" -ge "${a}" ] && [ "${vf}" -le "${b}" ]; then
                    return 0
                fi ;;
            *)
                if is_numeric "${item}" && [ "${item}" = "${vf}" ]; then
                    return 0
                fi ;;
        esac
    done
    return 1
}

# ---------------------------------------------------------------------------
# eswitch-management control transport
#
#   OK response  -> full response on stdout, rc 0
#   ERR response -> full response on stdout, rc 1
#   transport failure -> note in log, rc 2
# ---------------------------------------------------------------------------

init_transport() {
    local sock ctl
    sock="${ESWITCH_CONTROL_SOCKET:-$(json_get "${PHYS_DETAILS}" "control.socket")}"
    if [ -n "${sock}" ]; then
        _ESW_SOCKET="${sock}"
        export ESWITCH_CONTROL_SOCKET="${_ESW_SOCKET}"
    fi

    ctl="${ESWITCHCTL:-$(json_get "${PHYS_DETAILS}" "eswitchctl.path")}"
    if [ -n "${ctl}" ] && [ -x "${ctl}" ]; then
        _ESWCTL_BIN="${ctl}"
        _ESW_TRANSPORT="ctl"
    elif command -v python3 >/dev/null 2>&1; then
        _ESW_TRANSPORT="py"
    elif command -v socat >/dev/null 2>&1; then
        _ESW_TRANSPORT="socat"
    else
        die "No eswitch transport available: install eswitchctl, python3 or socat on the BlueField Arm host"
    fi
}

esw_run() {
    local cmd="$1" out rc=0
    case "${_ESW_TRANSPORT}" in
        ctl)
            out=$("${_ESWCTL_BIN}" ${cmd} 2>&1) || rc=1
            printf '%s\n' "${out}"
            return ${rc} ;;
        socat)
            if ! out=$(printf '%s\n' "${cmd}" | socat - UNIX-CONNECT:"${_ESW_SOCKET}" 2>&1); then
                log "socat transport failure"
                return 2
            fi
            printf '%s\n' "${out}"
            case "$(printf '%s' "${out}" | head -n1)" in
                OK*)  return 0 ;;
                ERR*) return 1 ;;
                *)    log "malformed eswitch response: ${out}"; return 2 ;;
            esac ;;
        py)
            python3 - "${_ESW_SOCKET}" "${cmd}" "${TIMEOUT_SECONDS}" <<'PY'
import socket, sys
path, command, timeout = sys.argv[1], sys.argv[2], max(float(sys.argv[3]), 5.0)
try:
    with socket.socket(socket.AF_UNIX, socket.SOCK_STREAM) as client:
        client.settimeout(timeout)
        client.connect(path)
        client.sendall((command + "\n").encode())
        client.shutdown(socket.SHUT_WR)
        chunks = []
        while True:
            chunk = client.recv(16384)
            if not chunk:
                break
            chunks.append(chunk)
    data = b"".join(chunks).decode("utf-8", "replace")
except Exception as exc:
    sys.stderr.write("eswitch transport failure: %s\n" % exc)
    sys.exit(2)
lines = data.splitlines()
if not lines or not (lines[0].startswith("OK") or lines[0].startswith("ERR")):
    sys.stderr.write("malformed eswitch response: %r\n" % data)
    sys.exit(2)
sys.stdout.write(data)
sys.exit(0 if lines[0].startswith("OK") else 1)
PY
            return $? ;;
    esac
}

esw_ok() {
    esw_run "status" >/dev/null 2>&1
}

# ---------------------------------------------------------------------------
# eswitch response parsers
# ---------------------------------------------------------------------------

vs_show() { esw_run "vs show --id $1"; }
vr_show() { esw_run "vr show --id $1"; }

vs_exists() { vs_show "$1" >/dev/null 2>&1; }
vr_exists() { vr_show "$1" >/dev/null 2>&1; }

vr_route_show() { esw_run "vr route show --id $1"; }
vr_nat_show()   { esw_run "vr nat show --id $1"; }

# rif_line <vr> <name>: prints the "<name> key=value ..." RIF line.
rif_line() {
    local out
    out=$(vr_show "$1") || { printf ''; return 0; }
    printf '%s\n' "${out}" | grep -E "^$2 " | head -n1
}

# line_field <field> <line>
line_field() {
    printf '%s\n' "$2" | grep -oE "(^| )$1=[^ ]+" | head -n1 | cut -d= -f2-
}

# route_line <vr> <prefix>: prints the "static <prefix> via=..." line.
route_line() {
    local out
    out=$(vr_route_show "$1") || { printf ''; return 0; }
    printf '%s\n' "${out}" | grep -F "static $2 via=" | head -n1
}

# vs_ports <vs>: prints vSwitch member DPDK port ids, space separated.
vs_ports() {
    local out
    out=$(vs_show "$1") || { printf ''; return 1; }
    printf '%s\n' "${out}" | \
        grep -oE 'ports=\[[^]]*\]' | \
        sed -E 's/ports=\[([^]]*)\]/\1/' | tr ',' '\n' | \
        sed -E 's/:.*$//' | tr '\n' ' '
}

# True when NAT is enabled on <vr> through RIF <iface>.
nat_enabled_on() {
    local vr="$1" iface="$2" nat_line
    nat_line=$(vr_nat_show "${vr}" | grep -m1 '^nat=' || true)
    printf '%s\n' "${nat_line}" | grep -q '^nat=enabled' || return 1
    [ "$(line_field interface "${nat_line}")" = "${iface}" ]
}

# ---------------------------------------------------------------------------
# Parse common arguments
#
# Production invocation (payload-file mode):
#   <wrapper> <command> <payload-file> <timeout-seconds>
# Named-argument mode is for manual testing only.
# ---------------------------------------------------------------------------

parse_args() {
    NETWORK_ID=""; GUEST_TYPE=""; ZONE_ID=""; VPC_ID=""
    VLAN=""; GATEWAY=""; CIDR=""
    PUBLIC_IP=""; PUBLIC_VLAN=""; PUBLIC_GATEWAY=""; PUBLIC_CIDR=""
    SOURCE_NAT=""; MAC=""; NIC_ID=""; NIC_UUID=""; VM_IP=""; VM_UUID=""
    HOSTNAME=""; NETWORK_STATE=""; EXTENSION_IP=""; RESTORE_DATA=""

    if [ -n "${PAYLOAD_FILE}" ] && [ -f "${PAYLOAD_FILE}" ]; then
        NETWORK_ID=$(_payload_json_get "${PAYLOAD_FILE}" "payload.network_id")
        GUEST_TYPE=$(_payload_json_get "${PAYLOAD_FILE}" "payload.guest_type")
        ZONE_ID=$(_payload_json_get "${PAYLOAD_FILE}" "payload.zone_id")
        VPC_ID=$(_payload_json_get "${PAYLOAD_FILE}" "payload.vpc_id")
        VLAN=$(_payload_json_get "${PAYLOAD_FILE}" "payload.vlan")
        GATEWAY=$(_payload_json_get "${PAYLOAD_FILE}" "payload.gateway")
        CIDR=$(_payload_json_get "${PAYLOAD_FILE}" "payload.cidr")
        PUBLIC_IP=$(_payload_json_get "${PAYLOAD_FILE}" "payload.public_ip")
        PUBLIC_VLAN=$(_payload_json_get "${PAYLOAD_FILE}" "payload.public_vlan")
        PUBLIC_GATEWAY=$(_payload_json_get "${PAYLOAD_FILE}" "payload.public_gateway")
        PUBLIC_CIDR=$(_payload_json_get "${PAYLOAD_FILE}" "payload.public_cidr")
        SOURCE_NAT=$(_payload_json_get "${PAYLOAD_FILE}" "payload.source_nat")
        MAC=$(_payload_json_get "${PAYLOAD_FILE}" "payload.mac")
        NIC_ID=$(_payload_json_get "${PAYLOAD_FILE}" "payload.nic_id")
        NIC_UUID=$(_payload_json_get "${PAYLOAD_FILE}" "payload.nic_uuid")
        VM_UUID=$(_payload_json_get "${PAYLOAD_FILE}" "payload.vm_uuid")
        VM_IP=$(_payload_json_get "${PAYLOAD_FILE}" "payload.ip")
        HOSTNAME=$(_payload_json_get "${PAYLOAD_FILE}" "payload.hostname")
        NETWORK_STATE=$(_payload_json_get "${PAYLOAD_FILE}" "payload.network_state")
        EXTENSION_IP=$(_payload_json_get "${PAYLOAD_FILE}" "payload.extension_ip")
        RESTORE_DATA=$(_payload_json_get "${PAYLOAD_FILE}" "payload.restore_data")
    else
        while [ $# -gt 0 ]; do
            case "$1" in
                --network-id)     NETWORK_ID="$2";  shift 2 ;;
                --guest-type)     GUEST_TYPE="$2";  shift 2 ;;
                --zone-id)        ZONE_ID="$2";     shift 2 ;;
                --vpc-id)         VPC_ID="$2";      shift 2 ;;
                --vlan)           VLAN="$2";        shift 2 ;;
                --gateway)        GATEWAY="$2";     shift 2 ;;
                --cidr)           CIDR="$2";        shift 2 ;;
                --public-ip)      PUBLIC_IP="$2";   shift 2 ;;
                --public-vlan)    PUBLIC_VLAN="$2"; shift 2 ;;
                --public-gateway) PUBLIC_GATEWAY="$2"; shift 2 ;;
                --public-cidr)    PUBLIC_CIDR="$2"; shift 2 ;;
                --source-nat)     SOURCE_NAT="$2";  shift 2 ;;
                --mac)            MAC="$2";         shift 2 ;;
                --nic-id)         NIC_ID="$2";      shift 2 ;;
                --nic-uuid)       NIC_UUID="$2";    shift 2 ;;
                --vm-uuid)        VM_UUID="$2";     shift 2 ;;
                --ip)             VM_IP="$2";       shift 2 ;;
                --hostname)       HOSTNAME="$2";    shift 2 ;;
                --network-state)  NETWORK_STATE="$2"; shift 2 ;;
                *)                shift ;;
            esac
        done
    fi

    SOURCE_NAT="${SOURCE_NAT:-false}"

    local v val
    for v in VLAN GATEWAY CIDR PUBLIC_IP PUBLIC_VLAN PUBLIC_GATEWAY PUBLIC_CIDR; do
        val=$(state_get "$(echo "${v}" | tr 'A-Z' 'a-z')")
        if [ -n "${val}" ] && [ -z "${!v:-}" ]; then
            eval "${v}=\"\${val}\""
        fi
    done

    VLAN=$(normalize_vlan "${VLAN}")
    PUBLIC_VLAN=$(normalize_vlan "${PUBLIC_VLAN}")

    [ -n "${NETWORK_ID}" ] || die "Missing network_id"

    if [ -z "${VSWITCH_ID}" ]; then
        VSWITCH_ID=$(derive_vswitch_id "${VLAN}" "${NETWORK_ID}") || \
            die "Cannot derive vswitch id (vlan=${VLAN} network_id=${NETWORK_ID})"
    fi
    VR_ID="${VR_ID:-${VSWITCH_ID}}"
}

# ---------------------------------------------------------------------------
# VF allocation: representors are attached lazily, one per guest NIC
# (prepare-nic), and freed when the NIC is released.  Allocation state lives
# in a single DPU-wide file so the pool is shared by all extension networks.
# ---------------------------------------------------------------------------

vf_alloc_file() { printf '%s/vf-alloc.txt' "${STATE_DIR}"; }

alloc_get() {
    local nic="$1" f line
    [ -n "${nic}" ] || return 0
    f=$(vf_alloc_file)
    [ -f "${f}" ] || return 0
    line=$(grep -m1 "^${nic} " "${f}" || true)
    [ -n "${line}" ] && printf '%s\n' "${line}"
    return 0
}

# Entry format: <nic_uuid> network=<id> vf=<n> pci=<bdf> port=<dpdk-id>
alloc_add() {
    local nic="$1" network="$2" vf="$3" pci="$4" port="$5" f
    f=$(vf_alloc_file)
    mkdir -p "${STATE_DIR}" 2>/dev/null || true
    grep -v "^${nic} " "${f}" 2>/dev/null > "${f}.tmp" || true
    printf '%s network=%s vf=%s pci=%s port=%s\n' "${nic}" "${network}" "${vf}" "${pci}" "${port}" >> "${f}.tmp"
    mv "${f}.tmp" "${f}"
}

alloc_del() {
    local nic="$1" f
    f=$(vf_alloc_file)
    [ -f "${f}" ] || return 0
    grep -v "^${nic} " "${f}" > "${f}.tmp" 2>/dev/null || true
    mv "${f}.tmp" "${f}"
}

alloc_list_for_network() {
    local network="$1" f line
    f=$(vf_alloc_file)
    [ -f "${f}" ] || return 0
    while read -r line; do
        [ -n "${line}" ] || continue
        case "${line}" in *"network=${network} "*) printf '%s\n' "${line}" ;; esac
    done < "${f}"
    return 0
}

alloc_vf_field() { printf '%s' "$1" | grep -oE 'vf=[0-9]+' | head -n1 | cut -d= -f2-; }
alloc_pci_field() { printf '%s' "$1" | grep -oE 'pci=[0-9a-fA-F:.]+' | head -n1 | cut -d= -f2-; }
alloc_port_field() { printf '%s' "$1" | grep -oE 'port=[0-9]+' | head -n1 | cut -d= -f2-; }

# vf index -> DPDK port id (from a fresh 'port show' pairing)
vf_to_port() {
    local want="$1" out line pid vf
    [ -n "${want}" ] || return 1
    out=$(esw_run "port show") || return 1
    while read -r line; do
        [ -n "${line}" ] || continue
        pid=$(printf '%s' "${line}" | grep -oE 'DPDK port [0-9]+' | grep -oE '[0-9]+' | head -n1)
        vf=$(printf '%s' "${line}" | grep -oE 'vf=[0-9]+' | head -n1 | cut -d= -f2-)
        [ -n "${pid}" ] && [ -n "${vf}" ] && [ "${vf}" = "${want}" ] && { printf '%s' "${pid}"; return 0; }
    done < <(printf '%s\n' "${out}" | grep -E '^DPDK port [0-9]+ \(host=[0-9]+ pf=[0-9]+ vf=[0-9]+\)$')
    return 1
}

# vf index -> host PCI bus address (BlueField-3 layout: 6 VFs on 84:00.2-.7,
# 8 VFs on 84:01.0-.7, 8 VFs on 84:02.0-.7).
vf_pci_bdf() {
    local n="$1"
    is_numeric "${n}" || return 1
    if [ "${n}" -ge 0 ] && [ "${n}" -le 5 ]; then
        printf '0000:84:00.%d' "$(( n + 2 ))"
        return 0
    fi
    if [ "${n}" -ge 6 ] && [ "${n}" -le 13 ]; then
        printf '0000:84:01.%d' "$(( n - 6 ))"
        return 0
    fi
    if [ "${n}" -ge 14 ] && [ "${n}" -le 21 ]; then
        printf '0000:84:02.%d' "$(( n - 14 ))"
        return 0
    fi
    return 1
}

# Set the VF MAC address from the DPU side (the guest will see this MAC, so
# cloud-init's config-drive network config can match the interface by MAC).
# Best effort: the flow still works with the VF's random MAC.
devlink_set_vf_mac() {
    local vf="$1" mac="$2" out try
    command -v devlink >/dev/null 2>&1 || { log "devlink not available; skipping VF mac=${mac} for vf=${vf}"; return 0; }
    for try in 1 2 3; do
        out=$(devlink port show 2>/dev/null | grep -E "flavour pcivf .* pfnum 0 vfnum ${vf}( |$)" | head -n1 | cut -d: -f1-3) || true
        [ -n "${out}" ] || { log "devlink: no pcivf port for vf=${vf} (try ${try}); retrying"; sleep 1; continue; }
        if devlink port function set "${out}" hw_addr "${mac}" >/dev/null 2>&1; then
            log "devlink: vf=${vf} hw_addr=${mac} (${out})"
            return 0
        fi
        log "devlink: hw_addr set failed for vf=${vf} (${out}) try ${try}; retrying"
        sleep 1
    done
    log "WARN: devlink hw_addr could not be set for vf=${vf}; the VF keeps its current MAC"
    return 0
}

emit_success() {
    printf '{"status":"success"%s}\n' "$*"
}

# Attach one VF representor to a vSwitch (idempotent read-first).
attach_vf_port() {
    local vs="$1" vf="$2" pid="${3:-}" members
    if [ -z "${pid}" ]; then
        pid=$(vf_to_port "${vf}") || { log "attach_vf_port: vf=${vf} has no DPDK port"; return 1; }
    fi
    members=$(vs_ports "${vs}" || true)
    if printf '%s\n' "${members}" | grep -qw "${pid}"; then
        log "attach_vf_port: vs=${vs} port=${pid} (vf=${vf}) already attached"
        printf '%s' "${pid}"
        return 0
    fi
    if ! esw_run "vs port attach --id ${vs} --port ${pid}" >/dev/null; then
        members=$(vs_ports "${vs}" || true)
        if ! printf '%s\n' "${members}" | grep -qw "${pid}"; then
            log "vs port attach --id ${vs} --port ${pid} (vf=${vf}) refused"
            return 1
        fi
    fi
    log "attach_vf_port: vs=${vs} port=${pid} (vf=${vf})"
    printf '%s' "${pid}"
    return 0
}

detach_vf_port() {
    local vs="$1" vf="$2" pid="${3:-}" members
    if [ -z "${pid}" ]; then
        pid=$(vf_to_port "${vf}") || { log "detach_vf_port: vf=${vf} has no DPDK port; skipped"; return 0; }
    fi
    members=$(vs_ports "${vs}" 2>/dev/null || true)
    if ! printf '%s\n' "${members}" | grep -qw "${pid}"; then
        return 0
    fi
    if ! esw_run "vs port detach --id ${vs} --port ${pid}" >/dev/null; then
        log "vs port detach --id ${vs} --port ${pid} (vf=${vf}) failed"
        return 1
    fi
    log "detach_vf_port: vs=${vs} port=${pid} (vf=${vf})"
    return 0
}

# Re-attach every VF recorded for a network to its vSwitch (recovery path
# after daemon restart, network shutdown or vSwitch re-creation).
reconcile_allocated_vfs() {
    local vs="$1" network="$2" entry vf port pid
    while read -r entry; do
        [ -n "${entry}" ] || continue
        vf=$(alloc_vf_field "${entry}")
        port=$(alloc_port_field "${entry}")
        pid=$(attach_vf_port "${vs}" "${vf}" "${port}") || log "WARN: reconcile vs=${vs} vf=${vf} failed"
    done < <(alloc_list_for_network "${network}")
}

# Allocate the lowest free VF of the pool for a NIC.  VFs already allocated to
# any NIC, VFs without a probed representor and the public uplink VF are
# skipped.  Prints the vf index on success.
allocate_vf() {
    # Prints "<vf> <port>" of the allocated VF on success.
    local vs="$1" network="$2" nic="$3" uplinkvf uplinkpid members vf pci pid pid_ok
    [ -n "${VF_POOL}" ] || { log "allocate_vf: no vf.pool registration detail"; return 1; }
    uplinkvf=$(json_get "${PHYS_DETAILS}" "uplink.port")
    uplinkpid=$(resolve_uplink_port 2>/dev/null || true)
    # Idempotency: an allocation for this NIC is reused (re-attached below).
    local entry vf_saved port_saved
    entry=$(alloc_get "${nic}")
    if [ -n "${entry}" ]; then
        vf_saved=$(alloc_vf_field "${entry}")
        port_saved=$(alloc_port_field "${entry}")
        if vf_pool_match "${VF_POOL}" "${vf_saved}" && pid=$(attach_vf_port "${vs}" "${vf_saved}" "${port_saved}"); then
            printf '%s %s' "${vf_saved}" "${pid}"
            return 0
        fi
        # Stale entry (representor gone / detached): fall through and re-allocate.
        log "allocate_vf: stale allocation ${entry} - reallocating"
        alloc_del "${nic}"
    fi

    for vf in $(vf_pool_indexes "${VF_POOL}"); do
        [ "${vf}" = "${uplinkvf}" ] && continue
        grep -q "vf=${vf} " "$(vf_alloc_file)" 2>/dev/null && continue
        pid=$(vf_to_port "${vf}") || continue
        [ -n "${uplinkpid}" ] && [ "${pid}" = "${uplinkpid}" ] && continue
        # A representor may already be a member of a different vSwitch
        pid_ok=1
        for other_vs in $(esw_run "vs show" 2>/dev/null | grep -E '^vs=' | grep -v "^vs=${vs} " | sed 's/^vs=//; s/ ports=.*//' || true); do
            members=$(vs_ports "${other_vs}" 2>/dev/null || true)
            printf '%s\n' "${members}" | grep -qw "${pid}" && pid_ok=0
        done
        [ "${pid_ok}" = "0" ] && continue
        if pid=$(attach_vf_port "${vs}" "${vf}" "${pid}"); then
            printf '%s %s' "${vf}" "${pid}"
            return 0
        fi
    done
    log "allocate_vf: no free VF in pool ${VF_POOL} for nic=${nic}"
    return 1
}

# Expand a pool spec like "5-20" or "5,7,9-10" into ascending VF indexes.
vf_pool_indexes() {
    local spec="$1" part a b i
    local -a out=()
    IFS=',' read -ra parts <<< "${spec}"
    for part in "${parts[@]}"; do
        part="${part// /}"
        case "${part}" in
            *-*)
                a="${part%%-*}"; b="${part##*-}"
                if is_numeric "${a}" && is_numeric "${b}"; then
                    i=${a}
                    while [ "${i}" -le "${b}" ]; do printf '%s ' "${i}"; i=$((i+1)); done
                fi ;;
            *) is_numeric "${part}" && printf '%s ' "${part}" ;;
        esac
    done
    return 0
}

uplink_vf_index() {
    local explicit
    explicit=$(json_get "${PHYS_DETAILS}" "uplink.port")
    if is_numeric "${explicit}" && [ "${explicit}" -ge 0 ]; then
        # uplink.port is interpreted as a VF index first (see resolve_uplink_port)
        printf '%s' "${explicit}"
        return 0
    fi
    return 1
}

# ---------------------------------------------------------------------------
# Public uplink resolution
# ---------------------------------------------------------------------------

resolve_uplink_port() {
    local explicit port out
    explicit=$(json_get "${PHYS_DETAILS}" "uplink.port")
    if is_numeric "${explicit}" && [ "${explicit}" -ge 0 ]; then
        out=$(esw_run "port show") || { log "port show failed (uplink.port=${explicit})"; return 1; }

        # Prefer the VF-index interpretation (stable across daemon restarts:
        # DPDK port ids are re-assigned on every probe); fall back to the
        # literal DPDK port id when no VF matches.
        port=$(printf '%s\n' "${out}" | \
            grep -E '^DPDK port [0-9]+ \(host=[0-9]+ pf=[0-9]+ vf='"${explicit}"'\)$' | \
            grep -oE '[0-9]+' | head -n1)
        if is_numeric "${port}"; then
            log "uplink.port ${explicit} resolved as vf index -> DPDK port ${port}"
            printf '%s' "${port}"
            return 0
        fi
        if printf '%s\n' "${out}" | grep -qE "^DPDK port ${explicit} "; then
            log "uplink.port ${explicit} used as DPDK port id"
            printf '%s' "${explicit}"
            return 0
        fi
        log "uplink.port ${explicit}: neither a VF index nor a DPDK port id present in 'port show'"
        return 1
    fi
    out=$(esw_run "port show") || { log "port show failed"; return 1; }
    port=$(printf '%s\n' "${out}" | \
        grep -oE '^DPDK port [0-9]+ \(uplink/parent\)' | \
        grep -oE '[0-9]+' | head -n1)
    if is_numeric "${port}"; then
        printf '%s' "${port}"
        return 0
    fi
    log "cannot resolve public uplink: no parent/uplink port in 'port show'; set 'uplink.port' registration detail"
    return 1
}

# VLAN WAN always uses the physical parent representor (p0). A VF selected by
# the legacy uplink.port detail cannot carry the physical trunk.
resolve_parent_uplink_port() {
    local out port
    out=$(esw_run "port show") || { log "port show failed"; return 1; }
    port=$(printf '%s\n' "${out}" | \
        grep -oE '^DPDK port [0-9]+ \(uplink/parent\)' | \
        grep -oE '[0-9]+' | head -n1)
    if is_numeric "${port}"; then
        printf '%s' "${port}"
        return 0
    fi
    log "cannot resolve p0: no parent/uplink port in 'port show'"
    return 1
}

ensure_trunk_member() {
    local vs="$1" port="$2" vlan="$3" out descriptors
    out=$(vs_show "${vs}" || true)
    descriptors=$(printf '%s\n' "${out}" | grep -oE 'ports=\[[^]]*\]' | \
        sed -E 's/ports=\[([^]]*)\]/\1/' | tr ',' '\n')
    if printf '%s\n' "${descriptors}" | grep -qx "${port}:trunk/vlan=${vlan}"; then
        return 0
    fi
    if printf '%s\n' "${descriptors}" | grep -qE "^${port}:"; then
        log "port ${port} is already attached to vs=${vs} with a different mode/VLAN"
        return 1
    fi
    if ! esw_run "vs port attach --id ${vs} --port ${port} --mode trunk --vlan ${vlan}" >/dev/null; then
        log "trunk attach failed: vs=${vs} port=${port} vlan=${vlan}"
        return 1
    fi
    log "trunk attached: vs=${vs} port=${port} vlan=${vlan}"
}

# ---------------------------------------------------------------------------
# VF pool: pre-attach the configured VF representor range to the network
# vSwitch so any guest VM may use any free VF of that pool.
# ---------------------------------------------------------------------------

# ---------------------------------------------------------------------------
# VF pool pre-attach was removed: guest VFs are allocated lazily per NIC
# (see allocate_vf / attach_vf_port).
# ---------------------------------------------------------------------------

# ---------------------------------------------------------------------------
# Desired-state helpers (idempotent: read first, mutate, verify)
# ---------------------------------------------------------------------------

ensure_vs() {
    local vs="$1"
    if vs_exists "${vs}"; then
        log "ensure_vs: vs=${vs} exists"
        return 0
    fi
    esw_run "vs create --id ${vs}" >/dev/null || log "vs create --id ${vs} returned ERR"
    vs_exists "${vs}"
}

ensure_vr() {
    local vr="$1"
    if vr_exists "${vr}"; then
        log "ensure_vr: vr=${vr} exists"
        return 0
    fi
    esw_run "vr create --id ${vr}" >/dev/null || log "vr create --id ${vr} returned ERR"
    vr_exists "${vr}"
}

# ensure_sw_rif <vr> <vs> <name> <mac> <addr>
ensure_sw_rif() {
    local vr="$1" vs="$2" name="$3" mac="$4" addr="$5"
    local line cur_mac cur_addr
    line=$(rif_line "${vr}" "${name}")
    if [ -z "${line}" ]; then
        if ! esw_run "vr switch attach --id ${vr} --switch-id ${vs} --name ${name}" >/dev/null; then
            log "vr switch attach --id ${vr} --switch-id ${vs} --name ${name} failed"
            return 1
        fi
        line=$(rif_line "${vr}" "${name}")
        [ -n "${line}" ] || { log "RIF ${name} not visible in vr show after attach"; return 1; }
        log "ensure_sw_rif: ${name} attached to vr=${vr}"
    fi
    cur_mac=$(line_field mac "${line}")
    cur_addr=$(line_field address "${line}")
    [ "${cur_mac}" = "-" ] && cur_mac=""
    [ "${cur_addr}" = "-" ] && cur_addr=""
    if [ "${cur_mac}" != "${mac}" ]; then
        if ! esw_run "vr interface set --id ${vr} --interface ${name} --mac ${mac}" >/dev/null; then
            log "vr interface set ${name} --mac ${mac} failed"
            return 1
        fi
        log "ensure_sw_rif: ${name} mac=${mac}"
    fi
    if [ -n "${addr}" ] && [ "${cur_addr}" != "${addr}" ]; then
        if [ -n "${cur_addr}" ]; then
            esw_run "vr ip del --id ${vr} --interface ${name} --address ${cur_addr}" >/dev/null || true
        fi
        if ! esw_run "vr ip add --id ${vr} --interface ${name} --address ${addr}" >/dev/null; then
            log "vr ip add ${name} --address ${addr} failed"
            return 1
        fi
        log "ensure_sw_rif: ${name} address=${addr}"
    fi
    return 0
}

# ensure_default_route <vr> <via>
ensure_default_route() {
    local vr="$1" via="$2" line
    line=$(route_line "${vr}" "0.0.0.0/0")
    if [ -n "${line}" ]; then
        if [ "$(line_field via "${line}")" = "${via}" ]; then
            return 0
        fi
        esw_run "vr route del --id ${vr} --prefix 0.0.0.0/0" >/dev/null || true
    fi
    if ! esw_run "vr route add --id ${vr} --prefix 0.0.0.0/0 --via ${via} --interface uplink" >/dev/null; then
        log "vr route add 0.0.0.0/0 --via ${via} --interface uplink failed"
        return 1
    fi
    log "ensure_default_route: vr=${vr} via=${via}"
    return 0
}

# ensure_public_uplink <vr> <public_ip>
# Applies the daemon-required order: uplink RIF attach -> address ->
# default route -> NAT enable; NAT is disabled first on address changes.
ensure_public_uplink() {
    local vr="$1" pub_ip="$2" prefix up line cur_addr want_addr wan_vs
    is_ipv4 "${pub_ip}" || { log "assign-ip: missing/invalid public_ip"; return 1; }
    wan_vs=$(derive_public_vswitch_id "${PUBLIC_VLAN}") || {
        log "assign-ip: public_vlan must be in range 1-4094"; return 1;
    }
    prefix="${PUBLIC_CIDR##*/}"
    is_numeric "${prefix}" || prefix="32"
    want_addr="${pub_ip}/${prefix}"

    # Always reconcile the physical trunk before trusting restored router
    # state. This repairs a missing p0 membership after daemon/container
    # replacement without requiring the uplink RIF to be recreated.
    up=$(resolve_parent_uplink_port) || return 1
    ensure_vs "${wan_vs}" || return 1
    ensure_trunk_member "${wan_vs}" "${up}" "${PUBLIC_VLAN}" || return 1
    state_set uplink_port "${up}"
    state_set public_vswitch_id "${wan_vs}"

    line=$(rif_line "${vr}" "uplink")
    if [ -z "${line}" ]; then
        ensure_sw_rif "${vr}" "${wan_vs}" "uplink" "$(rif_mac "${wan_vs}")" "" || return 1
        line=$(rif_line "${vr}" "uplink")
        [ -n "${line}" ] || { log "uplink RIF not visible in vr show after attach"; return 1; }
        log "ensure_public_uplink: vr=${vr} uplink-vs=${wan_vs} p0-port=${up} vlan=${PUBLIC_VLAN}"
    elif [ "$(line_field type "${line}")" != "vs-link" ] ||
         [ "$(line_field switch "${line}")" != "${wan_vs}" ]; then
        log "existing uplink is not WAN vs=${wan_vs}; release the old public IP before changing public VLAN"
        return 1
    fi

    cur_addr=$(line_field address "${line}")
    [ "${cur_addr}" = "-" ] && cur_addr=""
    cur_ip="${cur_addr%%/*}"
    want_ip="${want_addr%%/*}"
    if [ "${cur_ip}" != "${want_ip}" ]; then
        if nat_enabled_on "${vr}" "uplink"; then
            if ! esw_run "vr nat disable --id ${vr}" >/dev/null; then
                log "vr nat disable failed before uplink address change"
                return 1
            fi
            log "ensure_public_uplink: NAT disabled before address change"
        fi
        if [ -n "${cur_addr}" ]; then
            esw_run "vr ip del --id ${vr} --interface uplink --address ${cur_addr}" >/dev/null || true
        fi
        if ! esw_run "vr ip add --id ${vr} --interface uplink --address ${want_addr}" >/dev/null; then
            log "vr ip add uplink --address ${want_addr} failed"
            return 1
        fi
        log "ensure_public_uplink: uplink address=${want_addr}"
    fi

    [ -n "${PUBLIC_GATEWAY}" ] || { log "assign-ip: missing public_gateway"; return 1; }
    is_ipv4 "${PUBLIC_GATEWAY}" || { log "assign-ip: invalid public_gateway"; return 1; }
    ensure_default_route "${vr}" "${PUBLIC_GATEWAY}" || return 1

    if ! nat_enabled_on "${vr}" "uplink"; then
        if ! esw_run "vr nat enable --id ${vr} --interface uplink --address interface --port-range ${NAT_PORT_RANGE}" >/dev/null; then
            if ! nat_enabled_on "${vr}" "uplink"; then
                log "vr nat enable (uplink, ${NAT_PORT_RANGE}) failed"
                return 1
            fi
        fi
        log "ensure_public_uplink: NAT enabled on uplink (${NAT_PORT_RANGE})"
    else
        log "ensure_public_uplink: NAT already enabled on uplink"
    fi
    return 0
}

# ---------------------------------------------------------------------------
# Teardown (fail-open: daemon unreachable -> treat DPU objects as gone)
# ---------------------------------------------------------------------------

teardown_vr() {
    local vr="$1" out rline rprefix name rtype raddr
    if ! esw_ok; then
        log "teardown_vr: eswitch-management not reachable; skipping vr=${vr}"
        return 0
    fi
    if ! vr_exists "${vr}"; then
        log "teardown_vr: vr=${vr} absent"
        return 0
    fi

    if nat_enabled_on "${vr}" "uplink"; then
        esw_run "vr nat disable --id ${vr}" >/dev/null || true
        log "teardown_vr: NAT disabled for vr=${vr}"
    fi

    out=$(vr_route_show "${vr}")
    while read -r rline; do
        [ -n "${rline}" ] || continue
        rprefix=$(printf '%s' "${rline}" | awk '{print $2}')
        esw_run "vr route del --id ${vr} --prefix ${rprefix}" >/dev/null 2>&1 || true
    done < <(printf '%s\n' "${out}" | grep -E '^static [^ ]+ via=' || true)

    out=$(vr_show "${vr}")
    while read -r rline; do
        [ -n "${rline}" ] || continue
        name=$(printf '%s' "${rline}" | awk '{print $1}')
        rtype=$(line_field type "${rline}")
        raddr=$(line_field address "${rline}")
        if [ -n "${raddr}" ]; then
            esw_run "vr ip del --id ${vr} --interface ${name} --address ${raddr}" >/dev/null 2>&1 || true
        fi
        case "${rtype}" in
            port-link) esw_run "vr port detach --id ${vr} --interface ${name}"   >/dev/null 2>&1 || true ;;
            vs-link)   esw_run "vr switch detach --id ${vr} --interface ${name}" >/dev/null 2>&1 || true ;;
        esac
        log "teardown_vr: detached RIF ${name} (${rtype}) from vr=${vr}"
    done < <(printf '%s\n' "${out}" | tail -n +2 || true)

    if vr_exists "${vr}"; then
        if ! esw_run "vr delete --id ${vr}" >/dev/null; then
            log "teardown_vr: vr delete --id ${vr} failed"
            return 1
        fi
        log "teardown_vr: vr=${vr} deleted"
    fi
    return 0
}

teardown_vs() {
    local vs="$1" out p
    if ! esw_ok; then
        log "teardown_vs: eswitch-management not reachable; skipping vs=${vs}"
        return 0
    fi
    if ! vs_exists "${vs}"; then
        log "teardown_vs: vs=${vs} absent"
        return 0
    fi
    out=$(vs_show "${vs}")
    for p in $(vs_ports "${vs}" || true); do
        [ -n "${p}" ] || continue
        esw_run "vs port detach --id ${vs} --port ${p}" >/dev/null 2>&1 || true
        log "teardown_vs: vs=${vs} port=${p} detached"
    done
    if ! esw_run "vs delete --id ${vs}" >/dev/null; then
        log "teardown_vs: vs delete --id ${vs} failed"
        return 1
    fi
    log "teardown_vs: vs=${vs} deleted"
    return 0
}

# ---------------------------------------------------------------------------
# Commands
# ---------------------------------------------------------------------------

cmd_prepare_nic() {
    parse_args "$@"
    overlay_state
    [ -n "${VSWITCH_ID}" ] || die "prepare-nic: cannot derive vswitch id"
    [ -n "${NIC_UUID}" ] || die "prepare-nic: missing nic_uuid"
    log "prepare-nic: network=${NETWORK_ID} vs=${VSWITCH_ID} nic=${NIC_ID} uuid=${NIC_UUID} mac=${MAC} ip=${VM_IP} vm_uuid=${VM_UUID}"

    esw_ok || die "eswitch-management daemon not reachable on ${_ESW_SOCKET} (eswitchctl status failed)"

    acquire_lock "${NETWORK_ID}"

    # Re-ensure the network device (idempotent): a network that was shut down
    # while VMs were stopped must be re-created before the guest boots.
    ensure_vs "${VSWITCH_ID}" || die "prepare-nic: vs create/verify failed for id ${VSWITCH_ID}"
    ensure_vr "${VR_ID}" || die "prepare-nic: vr create/verify failed for id ${VR_ID}"
    if [ -n "${GATEWAY}" ] && [ -n "${CIDR}" ]; then
        ensure_sw_rif "${VR_ID}" "${VSWITCH_ID}" "SW${VSWITCH_ID}" "$(rif_mac "${VR_ID}")" "${GATEWAY}/${CIDR##*/}" \
            || die "prepare-nic: private RIF setup failed for vr=${VR_ID}"
    fi
    if [ -n "$(state_get public_ip)" ] && [ -n "$(state_get public_gateway)" ]; then
        ensure_public_uplink "${VR_ID}" "$(state_get public_ip)" || die "prepare-nic: public uplink reconcile failed for vr=${VR_ID}"
    fi

    # Allocate one free VF for this NIC and attach its representor to the vSwitch.
    local alloc vf port pci
    alloc=$(allocate_vf "${VSWITCH_ID}" "${NETWORK_ID}" "${NIC_UUID}") || \
        die "prepare-nic: no free VF available in pool '${VF_POOL}' for nic ${NIC_UUID}"
    vf="${alloc%% *}"
    port="${alloc##* }"
    pci=$(vf_pci_bdf "${vf}") || die "prepare-nic: cannot map vf=${vf} to PCI bus address"

    if [ -n "${MAC}" ]; then
        devlink_set_vf_mac "${vf}" "${MAC}"
    fi

    alloc_add "${NIC_UUID}" "${NETWORK_ID}" "${vf}" "${pci}" "${port}"
    release_lock
    log "prepare-nic: nic=${NIC_UUID} allocated vf=${vf} pci=${pci} port=${port}"
    emit_success ",\"network.broadcast_domain_type\":\"Dpu\",\"network.broadcast_uri\":\"dpu://${VSWITCH_ID}\",\"nic.pci.address\":\"${pci}\",\"nic.vf.index\":\"${vf}\",\"vm.pci.bus.addresses\":\"${pci}\""
    exit 0
}

cmd_release_nic() {
    parse_args "$@"
    log "release-nic: network=${NETWORK_ID} nic=${NIC_ID} nic_uuid=${NIC_UUID} mac=${MAC}"
    local entry vf port pci removed=""
    entry=$(alloc_get "${NIC_UUID}")
    if [ -n "${entry}" ]; then
        vf=$(alloc_vf_field "${entry}")
        port=$(alloc_port_field "${entry}")
        pci=$(alloc_pci_field "${entry}")
        if esw_ok; then
            acquire_lock "${NETWORK_ID}"
            detach_vf_port "${VSWITCH_ID}" "${vf}" "${port}" || true
            alloc_del "${NIC_UUID}"
            release_lock
        else
            log "release-nic: eswitch-management not reachable; freeing state only"
            alloc_del "${NIC_UUID}"
        fi
        log "release-nic: nic=${NIC_UUID} freed vf=${vf} pci=${pci}"
        emit_success ",\"vm.pci.bus.addresses.remove\":\"${pci}\""
    else
        log "release-nic: no allocation recorded for nic=${NIC_UUID} (converged)"
        emit_success ""
    fi
    exit 0
}

cmd_implement_network() {
    parse_args "$@"
    overlay_state
    [ -n "${VPC_ID}" ] && die "VPC networks are not supported by ${_WRAPPER_EXT_DIR} (isolated only)"
    if [ -n "${GUEST_TYPE}" ] && [ "${GUEST_TYPE}" != "isolated" ]; then
        die "guest_type '${GUEST_TYPE}' not supported (isolated only)"
    fi
    [ -n "${GATEWAY}" ] || die "implement-network: missing gateway"
    [ -n "${CIDR}" ] || die "implement-network: missing cidr"

    acquire_lock "${NETWORK_ID}"
    log "implement-network: network=${NETWORK_ID} vswitch=${VSWITCH_ID} vr=${VR_ID} vlan=${VLAN} gw=${GATEWAY} cidr=${CIDR}"

    esw_ok || die "eswitch-management daemon not reachable on ${_ESW_SOCKET} (eswitchctl status failed)"

    ensure_vs "${VSWITCH_ID}" || die "vs create/verify failed for id ${VSWITCH_ID}"
    reconcile_allocated_vfs "${VSWITCH_ID}" "${NETWORK_ID}"

    ensure_vr "${VR_ID}" || die "vr create/verify failed for id ${VR_ID}"
    ensure_sw_rif "${VR_ID}" "${VSWITCH_ID}" "SW${VSWITCH_ID}" "$(rif_mac "${VR_ID}")" "${GATEWAY}/${CIDR##*/}" \
        || die "private RIF setup failed for vr=${VR_ID}"

    state_set vswitch_id "${VSWITCH_ID}"
    state_set vr_id "${VR_ID}"
    state_set vlan "${VLAN}"
    state_set gateway "${GATEWAY}"
    state_set cidr "${CIDR}"

    release_lock
    log "implement-network: done network=${NETWORK_ID}"
    # No VF is pre-allocated: guest VFs are attached lazily per NIC (prepare-nic).
    emit_success ",\"network.broadcast_domain_type\":\"Dpu\",\"network.broadcast_uri\":\"dpu://${VSWITCH_ID}\""
    exit 0
}

teardown_network() {
    local remove_state="$1"
    parse_args "$@"
    overlay_state
    if [ -z "${VSWITCH_ID}" ]; then
        VSWITCH_ID=$(derive_vswitch_id "${VLAN}" "${NETWORK_ID}") || \
            die "Cannot derive vswitch id (vlan=${VLAN} network_id=${NETWORK_ID})"
        VR_ID="${VSWITCH_ID}"
    fi
    acquire_lock "${NETWORK_ID}"
    log "teardown_network: network=${NETWORK_ID} vswitch=${VSWITCH_ID} vr=${VR_ID} remove_state=${remove_state}"

    if [ "${remove_state}" = "true" ]; then
        # Destroy: free every VF still allocated to this network (defensive:
        # VMs must have been destroyed first, but converge even if a release
        # was missed).
        local entry vf port pci nic
        while read -r entry; do
            [ -n "${entry}" ] || continue
            vf=$(alloc_vf_field "${entry}")
            port=$(alloc_port_field "${entry}")
            pci=$(alloc_pci_field "${entry}")
            nic=$(printf '%s' "${entry}" | awk '{print $1}')
            detach_vf_port "${VSWITCH_ID}" "${vf}" "${port}" || true
            alloc_del "${nic}"
            log "teardown_network: freed vf=${vf} pci=${pci} nic=${nic}"
        done < <(alloc_list_for_network "${NETWORK_ID}")
    fi

    local public_vs
    public_vs=$(state_get public_vswitch_id)
    teardown_vr "${VR_ID}" || true
    teardown_vs "${VSWITCH_ID}" || true
    if [ -n "${public_vs}" ] && [ "${public_vs}" != "${VSWITCH_ID}" ]; then
        teardown_vs "${public_vs}" || true
    fi

    if [ "${remove_state}" = "true" ]; then
        rm -rf "$(net_state_dir)" 2>/dev/null || true
        log "teardown_network: removed state for network ${NETWORK_ID}"
    fi
    release_lock
    log "teardown_network: done network=${NETWORK_ID}"
    exit 0
}

cmd_shutdown_network() { teardown_network "false"; }
cmd_destroy_network()  { teardown_network "true"; }

cmd_assign_ip() {
    parse_args "$@"
    overlay_state
    [ -z "${VPC_ID}" ] || die "VPC networks are not supported by ${_WRAPPER_EXT_DIR} (isolated only)"
    if [ "${SOURCE_NAT}" != "true" ]; then
        log "assign-ip: network=${NETWORK_ID} ip=${PUBLIC_IP} source_nat=false - nothing to do"
        exit 0
    fi
    is_ipv4 "${PUBLIC_IP}" || die "assign-ip: missing/invalid public_ip"
    [ -n "${PUBLIC_VLAN}" ] || die "assign-ip: missing public_vlan"
    is_ipv4 "${PUBLIC_GATEWAY}" || die "assign-ip: missing/invalid public_gateway"

    acquire_lock "${NETWORK_ID}"
    log "assign-ip: network=${NETWORK_ID} vswitch=${VSWITCH_ID} public=${PUBLIC_IP} vlan=${PUBLIC_VLAN}"

    esw_ok || die "eswitch-management daemon not reachable on ${_ESW_SOCKET}"

    ensure_public_uplink "${VR_ID}" "${PUBLIC_IP}" || die "public uplink/NAT setup failed for vr=${VR_ID}"

    state_set public_ip "${PUBLIC_IP}"
    state_set public_vlan "${PUBLIC_VLAN}"
    state_set public_gateway "${PUBLIC_GATEWAY}"
    state_set public_cidr "${PUBLIC_CIDR}"
    state_set nat_port_range "${NAT_PORT_RANGE}"

    release_lock
    log "assign-ip: done ${PUBLIC_IP} on network ${NETWORK_ID}"
    exit 0
}

cmd_release_ip() {
    parse_args "$@"
    overlay_state
    [ -n "${PUBLIC_IP}" ] || die "release-ip: missing public_ip"
    is_ipv4 "${PUBLIC_IP}" || die "release-ip: invalid public_ip"

    if [ "${SOURCE_NAT}" != "true" ]; then
        log "release-ip: network=${NETWORK_ID} ip=${PUBLIC_IP} source_nat=false - nothing to do"
        exit 0
    fi

    if esw_ok; then
        if nat_enabled_on "${VR_ID}" "uplink"; then
            esw_run "vr nat disable --id ${VR_ID}" >/dev/null || true
            log "release-ip: NAT disabled for vr=${VR_ID}"
        fi
        if [ -n "$(route_line "${VR_ID}" "0.0.0.0/0")" ]; then
            esw_run "vr route del --id ${VR_ID} --prefix 0.0.0.0/0" >/dev/null || true
            log "release-ip: default route removed for vr=${VR_ID}"
        fi
        local line cur_addr public_vs
        line=$(rif_line "${VR_ID}" "uplink")
        cur_addr=$(line_field address "${line}")
        if [ -n "${cur_addr}" ]; then
            esw_run "vr ip del --id ${VR_ID} --interface uplink --address ${cur_addr}" >/dev/null || true
            log "release-ip: uplink address ${cur_addr} removed"
        fi
        if [ -n "${line}" ] && [ "$(line_field type "${line}")" = "vs-link" ]; then
            esw_run "vr switch detach --id ${VR_ID} --interface uplink" >/dev/null 2>&1 || true
        fi
        public_vs=$(state_get public_vswitch_id)
        if [ -n "${public_vs}" ]; then
            teardown_vs "${public_vs}" || true
        fi
    else
        log "release-ip: eswitch-management not reachable; skipping DPU cleanup"
    fi

    rm -f "$(net_state_dir)/public_ip" "$(net_state_dir)/public_vlan" \
          "$(net_state_dir)/public_gateway" "$(net_state_dir)/public_cidr" \
          "$(net_state_dir)/public_vswitch_id" "$(net_state_dir)/uplink_port" \
          2>/dev/null || true
    log "release-ip: done ${PUBLIC_IP} on network ${NETWORK_ID}"
    exit 0
}

cmd_restore_network() {
    parse_args "$@"
    overlay_state
    [ -z "${VPC_ID}" ] || die "VPC networks are not supported by ${_WRAPPER_EXT_DIR} (isolated only)"
    [ -n "${GATEWAY}" ] || die "restore-network: missing gateway"
    [ -n "${CIDR}" ] || die "restore-network: missing cidr"

    acquire_lock "${NETWORK_ID}"
    log "restore-network: reconciling network=${NETWORK_ID} (restore_data=${RESTORE_DATA})"

    esw_ok || die "eswitch-management daemon not reachable on ${_ESW_SOCKET}"
    ensure_vs "${VSWITCH_ID}" || die "vs create/verify failed for id ${VSWITCH_ID}"
    reconcile_allocated_vfs "${VSWITCH_ID}" "${NETWORK_ID}"
    ensure_vr "${VR_ID}" || die "vr create/verify failed for id ${VR_ID}"
    ensure_sw_rif "${VR_ID}" "${VSWITCH_ID}" "SW${VSWITCH_ID}" "$(rif_mac "${VR_ID}")" "${GATEWAY}/${CIDR##*/}" \
        || die "private RIF reconcile failed for vr=${VR_ID}"

    if [ -n "${PUBLIC_IP}" ] && [ -n "${PUBLIC_GATEWAY}" ]; then
        ensure_public_uplink "${VR_ID}" "${PUBLIC_IP}" || die "public uplink reconcile failed for vr=${VR_ID}"
    fi
    release_lock
    log "restore-network: done network=${NETWORK_ID}"
    exit 0
}

cmd_status() {
    esw_run "status"
    esw_run "port show"
}

# ---------------------------------------------------------------------------
# Main dispatcher
# ---------------------------------------------------------------------------

usage() {
    echo "Usage: $0 {implement-network|shutdown-network|destroy-network|restore-network|" \
         "assign-ip|release-ip|prepare-nic|release-nic|status} <payload-file> <timeout-seconds>" >&2
}

ensure_dirs

COMMAND="${1:-}"
[ -n "${COMMAND}" ] || { usage; exit 1; }
shift || true

if [ $# -ge 1 ] && [ -f "$1" ]; then
    PAYLOAD_FILE="$1"
    TIMEOUT_SECONDS="${2:-60}"
    shift 2 || true
fi

if [ -n "${PAYLOAD_FILE}" ] && [ -f "${PAYLOAD_FILE}" ]; then
    PHYS_DETAILS=$(_payload_json_get "${PAYLOAD_FILE}" "physical-network-extension-details")
    EXTENSION_DETAILS=$(_payload_json_get "${PAYLOAD_FILE}" "network-extension-details")
fi

init_transport

_range=$(json_get "${PHYS_DETAILS}" "nat.port.range")
[ -n "${_range}" ] && NAT_PORT_RANGE="${_range}"
VF_POOL=$(json_get "${PHYS_DETAILS}" "vf.pool")

case "${COMMAND}" in
    implement-network)  cmd_implement_network  "$@" ;;
    shutdown-network)   cmd_shutdown_network   "$@" ;;
    destroy-network)    cmd_destroy_network    "$@" ;;
    restore-network)    cmd_restore_network    "$@" ;;
    assign-ip)          cmd_assign_ip          "$@" ;;
    release-ip)         cmd_release_ip         "$@" ;;
    status)             cmd_status             "$@" ;;
    prepare-nic)        cmd_prepare_nic        "$@" ;;
    release-nic)        cmd_release_nic        "$@" ;;
    *)
        echo "Unknown command: ${COMMAND} (supported services: SourceNat, Gateway; isolated networks only)" >&2
        exit 1 ;;
esac

exit 0
