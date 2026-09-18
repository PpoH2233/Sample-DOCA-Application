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

# Offline verification of the dpu-eswitch extension wrapper against the
# mock eswitchctl daemon.  No hardware required.

set -u

REPO_DIR="$(cd "$(dirname "$0")" && pwd)"
EXT="$REPO_DIR"
WRAPPER="${EXT}/dpu-eswitch-wrapper.sh"
PROXY="${EXT}/dpu-eswitch.sh"
MOCK_SRC="${EXT}/mock-eswitchctl"

TEST_ROOT=/tmp/opencode/dpu-eswitch-test
rm -rf "${TEST_ROOT}"
mkdir -p "${TEST_ROOT}"
export MOCK_STATE="${TEST_ROOT}/dpu-state"
export ESWITCHCTL="${TEST_ROOT}/mock-eswitchctl"
export DPU_ESWITCH_STATE_DIR="${TEST_ROOT}/wrapper-state"
export DPU_ESWITCH_LOG_FILE="${TEST_ROOT}/wrapper.log"
cp "${MOCK_SRC}" "${ESWITCHCTL}"
chmod +x "${ESWITCHCTL}"

pass=0
fail=0

chk() {
    local desc="$1" expected="$2" got="$3"
    if [ "${got}" = "${expected}" ]; then
        pass=$((pass+1))
        printf 'PASS: %s\n' "${desc}"
    else
        fail=$((fail+1))
        printf 'FAIL: %s (expected [%s] got [%s])\n' "${desc}" "${expected}" "${got}"
    fi
}

cmd_count() {
    grep -cxF "$1" "${MOCK_STATE}/commands.log"
}

payload() {
    printf '%s' "$2" > "${TEST_ROOT}/$1.json"
}

# ---------------------------------------------------------------------------
# T1: implement-network with VF pool 1-2 and vlan 100
# ---------------------------------------------------------------------------
payload impl '{
  "payload": {"network_id": "42", "vlan": "100", "gateway": "192.168.100.1",
              "cidr": "192.168.100.0/24", "guest_type": "isolated", "zone_id": "1"},
  "physical-network-extension-details": {"vf.pool": "1-2", "uplink.port": "0"},
  "network-extension-details": {}
}'
"${WRAPPER}" implement-network "${TEST_ROOT}/impl.json" 60 >"${TEST_ROOT}/impl.out" 2>"${TEST_ROOT}/impl.err"
rc=$?
chk "T1 implement rc" "0" "${rc}"
chk "T1 status probe"   "1" "$(cmd_count 'status')"
chk "T1 vs create"      "1" "$(cmd_count 'vs create --id 100')"
chk "T1 no pool attach (lazy VF)" "0" "$(cmd_count 'vs port attach --id 100 --port 1')"
chk "T1 no vf3"         "0" "$(cmd_count 'vs port attach --id 100 --port 3')"
chk "T1 vr create"      "1" "$(cmd_count 'vr create --id 100')"
chk "T1 switch attach"  "1" "$(cmd_count 'vr switch attach --id 100 --switch-id 100 --name SW100')"
chk "T1 rif mac"        "1" "$(cmd_count 'vr interface set --id 100 --interface SW100 --mac 02:00:00:65:00:64')"
chk "T1 rif address"    "1" "$(cmd_count 'vr ip add --id 100 --interface SW100 --address 192.168.100.1/24')"
grep -q '"network.broadcast_domain_type":"Dpu"' "${TEST_ROOT}/impl.out" && chk "T1 broadcast type" "yes" "yes" || chk "T1 broadcast type" "yes" "no"
grep -q '"network.broadcast_uri":"dpu://100"' "${TEST_ROOT}/impl.out" && chk "T1 broadcast uri" "yes" "yes" || chk "T1 broadcast uri" "yes" "no"

# ---------------------------------------------------------------------------
# T2: assign-ip (SNAT) on the implemented network
# ---------------------------------------------------------------------------
payload assign '{
  "payload": {"network_id": "42", "vlan": "100", "gateway": "192.168.100.1",
              "cidr": "192.168.100.0/24", "guest_type": "isolated", "zone_id": "1",
              "public_ip": "203.0.113.10", "source_nat": "true",
              "public_vlan": "555", "public_gateway": "203.0.113.1",
              "public_cidr": "203.0.113.0/24"},
  "physical-network-extension-details": {"vf.pool": "1-2", "uplink.port": "0"},
  "network-extension-details": {"host": "192.0.2.10", "vswitch_id": "100", "vr_id": "100", "vlan": "100"}
}'
"${WRAPPER}" assign-ip "${TEST_ROOT}/assign.json" 60 >"${TEST_ROOT}/assign.out" 2>"${TEST_ROOT}/assign.err"
rc=$?
chk "T2 assign rc" "0" "${rc}"
chk "T2 uplink attach"  "1" "$(cmd_count 'vr port attach --id 100 --port 0 --name uplink')"
chk "T2 uplink address" "1" "$(cmd_count 'vr ip add --id 100 --interface uplink --address 203.0.113.10/24')"
chk "T2 default route"  "1" "$(cmd_count 'vr route add --id 100 --prefix 0.0.0.0/0 --via 203.0.113.1 --interface uplink')"
chk "T2 nat enable"     "1" "$(cmd_count 'vr nat enable --id 100 --interface uplink --address interface --port-range 20000-60999')"

# ---------------------------------------------------------------------------
# T3: assign-ip again must be a no-op (idempotency)
# ---------------------------------------------------------------------------
"${WRAPPER}" assign-ip "${TEST_ROOT}/assign.json" 60 >"${TEST_ROOT}/assign2.out" 2>"${TEST_ROOT}/assign2.err"
rc=$?
chk "T3 re-assign rc" "0" "${rc}"
chk "T3 second uplink attach" "1" "$(cmd_count 'vr port attach --id 100 --port 0 --name uplink')"
chk "T3 second nat enable"    "1" "$(cmd_count 'vr nat enable --id 100 --interface uplink --address interface --port-range 20000-60999')"
chk "T3 second vs create"     "1" "$(cmd_count 'vs create --id 100')"

# ---------------------------------------------------------------------------
# T4: restore-network reconciles without duplicating objects
# ---------------------------------------------------------------------------
"${WRAPPER}" restore-network "${TEST_ROOT}/assign.json" 60 >"${TEST_ROOT}/restore.out" 2>"${TEST_ROOT}/restore.err"
rc=$?
chk "T4 restore rc" "0" "${rc}"
chk "T4 no second vr create" "1" "$(cmd_count 'vr create --id 100')"

# ---------------------------------------------------------------------------
# T5: release-ip tears down NAT, route and uplink address
# ---------------------------------------------------------------------------
"${WRAPPER}" release-ip "${TEST_ROOT}/assign.json" 60 >"${TEST_ROOT}/release.out" 2>"${TEST_ROOT}/release.err"
rc=$?
chk "T5 release rc" "0" "${rc}"
chk "T5 nat disable"    "1" "$(cmd_count 'vr nat disable --id 100')"
chk "T5 route del"      "1" "$(cmd_count 'vr route del --id 100 --prefix 0.0.0.0/0')"
chk "T5 uplink address removed" "1" "$(cmd_count 'vr ip del --id 100 --interface uplink --address 203.0.113.10/24')"

# re-assign after release must re-apply NAT
"${WRAPPER}" assign-ip "${TEST_ROOT}/assign.json" 60 >"${TEST_ROOT}/reassign.out" 2>"${TEST_ROOT}/reassign.err"
rc=$?
chk "T5b re-assign after release rc" "0" "${rc}"
chk "T5b second nat enable" "2" "$(cmd_count 'vr nat enable --id 100 --interface uplink --address interface --port-range 20000-60999')"

# ---------------------------------------------------------------------------
# T6: shutdown-network tears down the VR and the vSwitch, keeps state dir
# ---------------------------------------------------------------------------
"${WRAPPER}" shutdown-network "${TEST_ROOT}/impl.json" 60 >"${TEST_ROOT}/shutdown.out" 2>"${TEST_ROOT}/shutdown.err"
rc=$?
chk "T6 shutdown rc" "0" "${rc}"
chk "T6 vr delete" "1" "$(cmd_count 'vr delete --id 100')"
chk "T6 vs delete" "1" "$(cmd_count 'vs delete --id 100')"
chk "T6 state dir kept" "1" "$([ -d "${DPU_ESWITCH_STATE_DIR}/network-42" ] && echo 1 || echo 0)"

# implement again on the same ids (restart flow) must recreate cleanly
"${WRAPPER}" implement-network "${TEST_ROOT}/impl.json" 60 >"${TEST_ROOT}/impl2.out" 2>"${TEST_ROOT}/impl2.err"
rc=$?
chk "T6b re-implement rc" "0" "${rc}"
chk "T6b second vs create" "2" "$(cmd_count 'vs create --id 100')"

# ---------------------------------------------------------------------------
# T7: destroy-network removes the state dir
# ---------------------------------------------------------------------------
"${WRAPPER}" destroy-network "${TEST_ROOT}/impl.json" 60 >"${TEST_ROOT}/destroy.out" 2>"${TEST_ROOT}/destroy.err"
rc=$?
chk "T7 destroy rc" "0" "${rc}"
chk "T7 state dir removed" "1" "$([ -d "${DPU_ESWITCH_STATE_DIR}/network-42" ] && echo 0 || echo 1)"
chk "T7 second vs delete" "2" "$(cmd_count 'vs delete --id 100')"

# destroy again on a clean DPU must still succeed (idempotent)
"${WRAPPER}" destroy-network "${TEST_ROOT}/impl.json" 60 >"${TEST_ROOT}/destroy2.out" 2>"${TEST_ROOT}/destroy2.err"
rc=$?
chk "T7b destroy-again rc" "0" "${rc}"

# ---------------------------------------------------------------------------
# T8: daemon unreachable -> mutation commands fail loudly
# ---------------------------------------------------------------------------
export ESWITCHCTL="${TEST_ROOT}/missing-eswitchctl"
payload dead '{"payload":{"network_id":"42","vlan":"100","gateway":"192.168.100.1","cidr":"192.168.100.0/24"},"physical-network-extension-details":{},"network-extension-details":{}}'
"${WRAPPER}" implement-network "${TEST_ROOT}/dead.json" 60 >"${TEST_ROOT}/dead.out" 2>"${TEST_ROOT}/dead.err"
rc=$?
chk "T8 daemon-down implement rc" "1" "${rc}"
grep -q "daemon not reachable" "${TEST_ROOT}/dead.err" && chk "T8 daemon-down message" "yes" "yes" || chk "T8 daemon-down message" "yes" "no"
export ESWITCHCTL="${TEST_ROOT}/mock-eswitchctl"

# ---------------------------------------------------------------------------
# T9: no vlan -> deterministic fallback id from network_id
# ---------------------------------------------------------------------------
payload nofb '{
  "payload": {"network_id": "42", "gateway": "192.168.100.1", "cidr": "192.168.100.0/24", "guest_type": "isolated"},
  "physical-network-extension-details": {"vf.pool": "1-2", "uplink.port": "0"},
  "network-extension-details": {}
}'
"${WRAPPER}" implement-network "${TEST_ROOT}/nofb.json" 60 >"${TEST_ROOT}/nofb.out" 2>"${TEST_ROOT}/nofb.err"
rc=$?
chk "T9 fallback id rc" "0" "${rc}"
chk "T9 vs create --id 43" "1" "$(cmd_count 'vs create --id 43')"

# ---------------------------------------------------------------------------
# T10: status command passes the daemon status through
# ---------------------------------------------------------------------------
"${WRAPPER}" status >"${TEST_ROOT}/status.out" 2>"${TEST_ROOT}/status.err"
rc=$?
chk "T10 status rc" "0" "${rc}"
grep -q '^OK$' "${TEST_ROOT}/status.out" && chk "T10 status body" "yes" "yes" || chk "T10 status body" "yes" "no"

# ---------------------------------------------------------------------------
# T11: unknown command refused
# ---------------------------------------------------------------------------
"${WRAPPER}" frobnicate "${TEST_ROOT}/impl.json" 60 >"${TEST_ROOT}/unk.out" 2>"${TEST_ROOT}/unk.err"
rc=$?
chk "T11 unknown command rc" "1" "${rc}"

# ---------------------------------------------------------------------------
# T12: prepare-nic lazily allocates VFs (pool 1-2, uplink vf 0 excluded)
# ---------------------------------------------------------------------------
"${WRAPPER}" implement-network "${TEST_ROOT}/impl.json" 60 >"${TEST_ROOT}/impl3.out" 2>"${TEST_ROOT}/impl3.err"
rc=$?
chk "T12 re-implement rc" "0" "${rc}"

payload prep1 '{
  "payload": {"network_id": "42", "vlan": "100", "gateway": "192.168.100.1",
              "cidr": "192.168.100.0/24", "guest_type": "isolated", "zone_id": "1",
              "nic_id": "5", "nic_uuid": "nic-aaa", "mac": "02:01:00:00:00:01", "ip": "192.168.100.10",
              "vm_uuid": "vm-uuid-1", "hostname": "vm-1", "default_nic": "true"},
  "physical-network-extension-details": {"vf.pool": "1-2", "uplink.port": "0"},
  "network-extension-details": {"host": "192.0.2.10", "vswitch_id": "100", "vr_id": "100", "vlan": "100"}
}'
"${WRAPPER}" prepare-nic "${TEST_ROOT}/prep1.json" 60 >"${TEST_ROOT}/prep1.out" 2>"${TEST_ROOT}/prep1.err"
rc=$?
chk "T12 prepare nic-1 rc" "0" "${rc}"
chk "T12 vf1 attached" "1" "$(cmd_count 'vs port attach --id 100 --port 1')"
grep -q '"nic.pci.address":"0000:84:00.3"' "${TEST_ROOT}/prep1.out" && chk "T12 pci bdf vf1" "yes" "yes" || chk "T12 pci bdf vf1" "yes" "no"
grep -q '"vm.pci.bus.addresses":"0000:84:00.3"' "${TEST_ROOT}/prep1.out" && chk "T12 vm detail key" "yes" "yes" || chk "T12 vm detail key" "yes" "no"
grep -q '"status":"success"' "${TEST_ROOT}/prep1.out" && chk "T12 status success" "yes" "yes" || chk "T12 status success" "yes" "no"

# ---------------------------------------------------------------------------
# T13: second NIC gets the next free VF
# ---------------------------------------------------------------------------
payload prep2 '{
  "payload": {"network_id": "42", "vlan": "100", "gateway": "192.168.100.1",
              "cidr": "192.168.100.0/24", "guest_type": "isolated", "zone_id": "1",
              "nic_id": "6", "nic_uuid": "nic-bbb", "mac": "02:01:00:00:00:02", "ip": "192.168.100.11",
              "vm_uuid": "vm-uuid-2", "hostname": "vm-2"},
  "physical-network-extension-details": {"vf.pool": "1-2", "uplink.port": "0"},
  "network-extension-details": {"host": "192.0.2.10", "vswitch_id": "100", "vr_id": "100", "vlan": "100"}
}'
"${WRAPPER}" prepare-nic "${TEST_ROOT}/prep2.json" 60 >"${TEST_ROOT}/prep2.out" 2>"${TEST_ROOT}/prep2.err"
rc=$?
chk "T13 prepare nic-2 rc" "0" "${rc}"
chk "T13 vf2 attached" "1" "$(cmd_count 'vs port attach --id 100 --port 2')"
grep -q '"nic.pci.address":"0000:84:00.4"' "${TEST_ROOT}/prep2.out" && chk "T13 pci bdf vf2" "yes" "yes" || chk "T13 pci bdf vf2" "yes" "no"

# ---------------------------------------------------------------------------
# T14: re-prepare the same NIC is idempotent (same VF, no new attach)
# ---------------------------------------------------------------------------
"${WRAPPER}" prepare-nic "${TEST_ROOT}/prep1.json" 60 >"${TEST_ROOT}/prep1b.out" 2>"${TEST_ROOT}/prep1b.err"
rc=$?
chk "T14 re-prepare rc" "0" "${rc}"
chk "T14 no second vf1 attach" "1" "$(cmd_count 'vs port attach --id 100 --port 1')"
grep -q '"nic.pci.address":"0000:84:00.3"' "${TEST_ROOT}/prep1b.out" && chk "T14 same pci" "yes" "yes" || chk "T14 same pci" "yes" "no"

# ---------------------------------------------------------------------------
# T15: pool exhaustion fails cleanly
# ---------------------------------------------------------------------------
payload prep3 '{
  "payload": {"network_id": "42", "vlan": "100", "gateway": "192.168.100.1",
              "cidr": "192.168.100.0/24", "guest_type": "isolated", "zone_id": "1",
              "nic_id": "7", "nic_uuid": "nic-ccc", "mac": "02:01:00:00:00:03", "ip": "192.168.100.12",
              "vm_uuid": "vm-uuid-3", "hostname": "vm-3"},
  "physical-network-extension-details": {"vf.pool": "1-2", "uplink.port": "0"},
  "network-extension-details": {"host": "192.0.2.10", "vswitch_id": "100", "vr_id": "100", "vlan": "100"}
}'
"${WRAPPER}" prepare-nic "${TEST_ROOT}/prep3.json" 60 >"${TEST_ROOT}/prep3.out" 2>"${TEST_ROOT}/prep3.err"
rc=$?
chk "T15 exhaustion rc" "1" "${rc}"
grep -q "no free VF" "${TEST_ROOT}/prep3.err" && chk "T15 exhaustion msg" "yes" "yes" || chk "T15 exhaustion msg" "yes" "no"

# ---------------------------------------------------------------------------
# T16: release-nic frees the VF; the next NIC reuses it
# ---------------------------------------------------------------------------
"${WRAPPER}" release-nic "${TEST_ROOT}/prep1.json" 60 >"${TEST_ROOT}/rel1.out" 2>"${TEST_ROOT}/rel1.err"
rc=$?
chk "T16 release rc" "0" "${rc}"
chk "T16 vf1 detached" "1" "$(cmd_count 'vs port detach --id 100 --port 1')"
grep -q '"vm.pci.bus.addresses.remove":"0000:84:00.3"' "${TEST_ROOT}/rel1.out" && chk "T16 remove key" "yes" "yes" || chk "T16 remove key" "yes" "no"
"${WRAPPER}" prepare-nic "${TEST_ROOT}/prep3.json" 60 >"${TEST_ROOT}/prep3b.out" 2>"${TEST_ROOT}/prep3b.err"
rc=$?
chk "T16b reuse vf1 rc" "0" "${rc}"
grep -q '"nic.pci.address":"0000:84:00.3"' "${TEST_ROOT}/prep3b.out" && chk "T16b reused vf1" "yes" "yes" || chk "T16b reused vf1" "yes" "no"

# ---------------------------------------------------------------------------
# T17: release of an unknown NIC still succeeds (converged)
# ---------------------------------------------------------------------------
payload relx '{
  "payload": {"network_id": "42", "vlan": "100", "gateway": "192.168.100.1",
              "cidr": "192.168.100.0/24", "guest_type": "isolated", "zone_id": "1",
              "nic_id": "9", "nic_uuid": "nic-unknown", "mac": "02:01:00:00:00:09"},
  "physical-network-extension-details": {"vf.pool": "1-2", "uplink.port": "0"},
  "network-extension-details": {"host": "192.0.2.10", "vswitch_id": "100", "vr_id": "100", "vlan": "100"}
}'
"${WRAPPER}" release-nic "${TEST_ROOT}/relx.json" 60 >"${TEST_ROOT}/relx.out" 2>"${TEST_ROOT}/relx.err"
rc=$?
chk "T17 unknown-nic release rc" "0" "${rc}"

# ---------------------------------------------------------------------------
# T18: destroy-network frees remaining allocations
# ---------------------------------------------------------------------------
"${WRAPPER}" destroy-network "${TEST_ROOT}/impl.json" 60 >"${TEST_ROOT}/destroy3.out" 2>"${TEST_ROOT}/destroy3.err"
rc=$?
chk "T18 destroy with allocs rc" "0" "${rc}"
chk "T18 alloc file emptied" "1" "$([ -f "${DPU_ESWITCH_STATE_DIR}/vf-alloc.txt" ] && grep -q "nic-ccc\|nic-bbb" "${DPU_ESWITCH_STATE_DIR}/vf-alloc.txt" && echo 0 || echo 1)"

# ---------------------------------------------------------------------------
# T19: shutdown does NOT free allocations; VM restart re-uses the same VF
# ---------------------------------------------------------------------------
"${WRAPPER}" implement-network "${TEST_ROOT}/impl.json" 60 >"${TEST_ROOT}/impl4.out" 2>"${TEST_ROOT}/impl4.err"
"${WRAPPER}" prepare-nic "${TEST_ROOT}/prep1.json" 60 >"${TEST_ROOT}/prep1c.out" 2>"${TEST_ROOT}/prep1c.err"
rc=$?
chk "T19 re-prepare after destroy rc" "0" "${rc}"
"${WRAPPER}" shutdown-network "${TEST_ROOT}/impl.json" 60 >"${TEST_ROOT}/shutdown2.out" 2>"${TEST_ROOT}/shutdown2.err"
"${WRAPPER}" implement-network "${TEST_ROOT}/impl.json" 60 >"${TEST_ROOT}/impl5.out" 2>"${TEST_ROOT}/impl5.err}"
"${WRAPPER}" prepare-nic "${TEST_ROOT}/prep1.json" 60 >"${TEST_ROOT}/prep1d.out" 2>"${TEST_ROOT}/prep1d.err"
rc=$?
chk "T19 same VF after shutdown+reimplement rc" "0" "${rc}"
grep -q '"nic.pci.address":"0000:84:00.3"' "${TEST_ROOT}/prep1d.out" && chk "T19 same VF retained" "yes" "yes" || chk "T19 same VF retained" "yes" "no"

echo
echo "result: pass=${pass} fail=${fail}"
[ "${fail}" -eq 0 ]
