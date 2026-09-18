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

# Offline verification of the dpu-eswitch proxy script: ensure-network-device
# host selection and remote forwarding.  SSH is stubbed so no network is used.

set -u

REPO_DIR="$(cd "$(dirname "$0")" && pwd)"
PROXY="${REPO_DIR}/dpu-eswitch.sh"
TEST_ROOT=/tmp/opencode/dpu-eswitch-proxy-test
rm -rf "${TEST_ROOT}"
mkdir -p "${TEST_ROOT}"

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

# Stub ssh: records its command and succeeds.  For the "reachability probe"
# behaviour any host is reachable.
cat > "${TEST_ROOT}/ssh" <<'EOF'
#!/bin/bash
printf '%s\n' "$*" >> "$(dirname "$0")/ssh.log"
echo ok
exit 0
EOF
chmod +x "${TEST_ROOT}/ssh"
export PATH="${TEST_ROOT}:${PATH}"

payload() {
    printf '%s' "$2" > "${TEST_ROOT}/$1.json"
}

# ---------------------------------------------------------------------------
# P1: ensure-network-device without vlan
# ---------------------------------------------------------------------------
payload p1 '{"payload":{"network_id":"42"},"physical-network-extension-details":{"hosts":"192.0.2.10,192.0.2.11"},"network-extension-details":{}}'
out=$("${PROXY}" ensure-network-device "${TEST_ROOT}/p1.json" 60 2>"${TEST_ROOT}/p1.err")
rc=$?
chk "P1 rc" "0" "${rc}"
chk "P1 json host" "yes" "$(printf '%s' "${out}" | grep -q '"host"' && echo yes || echo no)"
chk "P1 single-line json" "1" "$(printf '%s\n' "${out}" | wc -l | tr -d ' ')"
grep -q '"vswitch_id"' "${TEST_ROOT}/p1.json" 2>/dev/null
printf '%s' "${out}" | grep -q '"vswitch_id"' && chk "P1 no ids when vlan absent" "no" "yes" || chk "P1 no ids when vlan absent" "no" "no"

# ---------------------------------------------------------------------------
# P2: ensure-network-device with vlan 100 -> vswitch_id = vr_id = vlan
# ---------------------------------------------------------------------------
payload p2 '{"payload":{"network_id":"42","vlan":"100"},"physical-network-extension-details":{"hosts":"192.0.2.10,192.0.2.11"},"network-extension-details":{}}'
out=$("${PROXY}" ensure-network-device "${TEST_ROOT}/p2.json" 60 2>"${TEST_ROOT}/p2.err")
rc=$?
chk "P2 rc" "0" "${rc}"
printf '%s' "${out}" | grep -q '"vswitch_id":"100"' && chk "P2 vswitch_id" "yes" "yes" || chk "P2 vswitch_id" "yes" "no"
printf '%s' "${out}" | grep -q '"vr_id":"100"' && chk "P2 vr_id" "yes" "yes" || chk "P2 vr_id" "yes" "no"

# ---------------------------------------------------------------------------
# P3: sticky host selection via network-extension-details
# ---------------------------------------------------------------------------
payload p3 '{"payload":{"network_id":"42","vlan":"100"},"physical-network-extension-details":{"hosts":"192.0.2.10,192.0.2.11"},"network-extension-details":{"host":"192.0.2.11","vswitch_id":"100"}}'
out=$("${PROXY}" ensure-network-device "${TEST_ROOT}/p3.json" 60 2>"${TEST_ROOT}/p3.err")
rc=$?
chk "P3 rc" "0" "${rc}"
printf '%s' "${out}" | grep -q '"host":"192.0.2.10"' && chk "P3 sticky host" "no" "yes" || chk "P3 sticky host" "no" "no"

# ---------------------------------------------------------------------------
# P4: VPC refused
# ---------------------------------------------------------------------------
rm -f /tmp/cloudstack-extensions/dpu-eswitch.log
payload p4 '{"payload":{"network_id":"42","vpc_id":"7"},"physical-network-extension-details":{"hosts":"192.0.2.10"},"network-extension-details":{}}'
"${PROXY}" ensure-network-device "${TEST_ROOT}/p4.json" 60 >"${TEST_ROOT}/p4.out" 2>"${TEST_ROOT}/p4.err"
rc=$?
chk "P4 vpc rc" "1" "${rc}"
grep -q "VPC networks are not supported" /tmp/cloudstack-extensions/dpu-eswitch.log && \
    chk "P4 vpc message" "yes" "yes" || chk "P4 vpc message" "yes" "no"

# ---------------------------------------------------------------------------
# P5: non-ensure command forwards to the selected host with the payload file
# ---------------------------------------------------------------------------
payload p5 '{"payload":{"network_id":"42","vlan":"100"},"physical-network-extension-details":{"hosts":"192.0.2.10"},"network-extension-details":{"host":"192.0.2.10"}}'
rm -f "${TEST_ROOT}/ssh.log"
"${PROXY}" implement-network "${TEST_ROOT}/p5.json" 60 >"${TEST_ROOT}/p5.out" 2>"${TEST_ROOT}/p5.err"
rc=$?
chk "P5 forward rc" "0" "${rc}"
grep -qF "dpu-eswitch-wrapper.sh' 'implement-network'" "${TEST_ROOT}/ssh.log" && \
    chk "P5 wrapper invoked" "yes" "yes" || chk "P5 wrapper invoked" "yes" "no"
grep -qF "cs-extnet-payload" "${TEST_ROOT}/ssh.log" && \
    chk "P5 payload upload staged" "yes" "yes" || chk "P5 payload upload" "yes" "no"

echo
echo "result: pass=${pass} fail=${fail}"
[ "${fail}" -eq 0 ]
