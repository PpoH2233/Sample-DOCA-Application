<!--
 Licensed to the Apache Software Foundation (ASF) under one
 or more contributor license agreements.  See the NOTICE file
 distributed with this work for additional information
 regarding copyright ownership.  The ASF licenses this file
 to you under the Apache License, Version 2.0 (the
 "License"); you may not use this file except in compliance
 with the License.  You may obtain a copy of the License at

   http://www.apache.org/licenses/LICENSE-2.0

 Unless required by applicable law or agreed to in writing,
 software distributed under the License is distributed on an
 "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 KIND, either express or implied.  See the License for the
 specific language governing permissions and limitations
 under the License.
 -->
# dpu-eswitch — BlueField-3 NetworkOrchestrator extension

This directory contains the **dpu-eswitch** `NetworkOrchestrator` extension —
a CloudStack plugin that delegates isolated guest network orchestration to an
NVIDIA BlueField-3 DPU running the `eswitch-management` DOCA application
(virtual switch and virtual router implemented with DOCA Flow on the DPU).

Scope (phase 1): **isolated guest networks** offering **SourceNat + Gateway**.
Guest VMs connect through SR-IOV VFs of the BlueField passed through by KVM.

| File | Installed location | Purpose |
|------|--------------------|---------|
| `dpu-eswitch.sh` | management server | SSH proxy — executed by `NetworkExtensionElement` |
| `dpu-eswitch-wrapper.sh` | BlueField Arm (DPU) | Translates CloudStack commands to `eswitchctl` control commands |
| `mock-eswitchctl` | test only | Offline stand-in for `eswitchctl` used by `run-tests.sh` |
| `run-tests.sh` | test machine | Wrapper test harness against the mock daemon |
| `run-proxy-tests.sh` | test machine | Proxy test harness (stubbed SSH) |

The extension is loaded by CloudStack's in-tree
`framework/extensions/.../NetworkExtensionElement.java` — **no Java changes are
required**.  A reference shell-skeleton deployment is documented in the
CloudStack admin guide ("Extensions").

---

## Architecture

```
CloudStack Management Server
   └─ NetworkExtensionElement (in-tree, generic)
        exec: extensions/dpu-eswitch/dpu-eswitch.sh <command> <payload.json> <timeout>
             └─ SSH → BlueField-3 Arm
              └─ dpu-eswitch-wrapper.sh
                 └─ Unix socket /run/eswitch-management/control.sock
                    └─ eswitch-management daemon (DOCA Flow on BF3)

CloudStack KVM agent
   └─ SR-IOV: VM NIC = BlueField host VF (vfio passthrough)
      VF traffic → BF3 eSwitch → vSwitch (representor is a member)
```

### ID scheme

CloudStack already allocates a unique guest VLAN per isolated network, so the
wrapper derives stable DPU identities from it:

```
vswitch id = virtual router id = guest VLAN tag        (1..65535)
gateway RIF name = SW<vlan>                             (e.g. SW100)
RIF MAC = 02:00:00:65:hi:lo                             (unicast, unique per VR)
```

When no VLAN has been assigned yet the proxy stores only the host; the wrapper
falls back to `network_id % 65534 + 1`.

### Command → eswitchctl mapping

| CloudStack trigger | Script command | eswitchctl operations |
|---|---|---|
| Provider/offering validation | `ensure-network-device` (proxy-local) | `status` — DPU reachability is probed by SSH; ids returned in stdout JSON |
| First VM deploy | `implement-network` | `vs create`, `port show`, `vs port attach` (VF pool), `vr create`, `vr switch attach`, `vr interface set`, `vr ip add` |
| SNAT IP acquisition | `assign-ip` | `vr show`, `port show`, `vr port attach` (uplink), `vr ip add/del`, `vr route add/del`, `vr nat enable/disable` |
| SNAT IP release | `release-ip` | `vr nat disable`, `vr route del`, `vr ip del` |
| NIC lifecycle | `prepare-nic` / `release-nic` | none (the eSwitch FDB learns guest MACs) |
| Restart cycle | `restore-network` | idempotent reconcile of the desired state |
| Delete network | `shutdown-network` / `destroy-network` | reverse teardown (NAT → routes → addresses → RIFs → VR → VS) |

All desired-state steps are **read-first and idempotent** per the daemon
contract: existing objects are queried before mutation and
`ALREADY_EXIST`/`NOT_FOUND` races converge through a re-read.

---

## Installation

### Management server

Deploy the proxy as the extension executable:

```
/usr/share/cloudstack-management/extensions/dpu-eswitch/dpu-eswitch.sh
```

Copy the scripts there and `chmod +x dpu-eswitch.sh` (owned by `cloud:cloud`).
In developer mode the extensions directory defaults to `extensions/` in the
repository root, so `extensions/dpu-eswitch/dpu-eswitch.sh` is found
automatically.

Proxy log: `/tmp/cloudstack-extensions/dpu-eswitch.log`.

### BlueField-3 Arm host (one DPU per KVM host)

```bash
ssh <bf3-arm> "mkdir -p /etc/cloudstack/extensions/dpu-eswitch"
scp dpu-eswitch-wrapper.sh <bf3-arm>:/etc/cloudstack/extensions/dpu-eswitch/
ssh <bf3-arm> "chmod +x /etc/cloudstack/extensions/dpu-eswitch/dpu-eswitch-wrapper.sh"
```

Wrapper log: `/var/log/cloudstack/extensions/dpu-eswitch/dpu-eswitch.log`
(state under `/var/lib/cloudstack/dpu-eswitch/network-<id>/`).

Prerequisites on the Arm host:

| Requirement | Purpose |
|-------------|---------|
| `eswitch-management` daemon running (container or systemd) | owns the eSwitch, DOCA Flow, parent port and VF representors |
| `eswitchctl` in PATH (or registration detail `eswitchctl.path`) | control transport; falls back to `python3` or `socat` |
| `flock`, `python3` | wrapper serialisation and payload parsing |
| `sshd` reachable from the management server | proxy transport |

The control socket has **no authentication**; access control is filesystem
permissions plus the SSH boundary.  Never expose the socket over the network.

---

## Step-by-step API setup

### 1. Create the extension

```bash
cmk createExtension name=dpu-eswitch type=NetworkOrchestrator path=dpu-eswitch \
    details[0].network.services="SourceNat,Gateway" \
    details[1].network.service.capabilities='{"SourceNat":{"SupportedSourceNatTypes":"peraccount","RedundantRouter":"false"}}'
```

### 2. Register with the guest physical network

```bash
cmk registerExtension id=<ext-uuid> resourcetype=PhysicalNetwork resourceid=<phys-net-uuid>
cmk updateRegisteredExtension \
    extensionid=<ext-uuid> resourcetype=PhysicalNetwork resourceid=<phys-net-uuid> \
    "details[0].hosts=<bf3-arm-ip>[,<bf3-arm-ip2>...]" \
    "details[1].username=<user>" \
    "details[2].sshkey=<pem-key>" \
    "details[3].vf.pool=1-10" \
    "details[4].uplink.port=" \
    "details[5].nat.port.range=20000-60999"
```

Registration details:

| Key | Meaning | Default |
|-----|---------|---------|
| `hosts` | BlueField Arm endpoints (one DPU per KVM host) | — |
| `username` / `sshkey` / `password` / `port` | SSH transport to the Arm host | `root`, port 22, agent auth |
| `vf.pool` | VF indexes pre-attached to each network's vSwitch | manual attach |
| `uplink.port` | DPDK port id of the public uplink RIF | auto-detect the parent/uplink port |
| `nat.port.range` | SNAT/PAT port range | `20000-60999` |
| `eswitchctl.path` | explicit `eswitchctl` path | PATH lookup |
| `control.socket` | control socket override | `/run/eswitch-management/control.sock` |

### 3. Create a network offering

```bash
cmk createNetworkOffering \
    name="DPU Isolated SNAT" displaytext="Isolated network on BlueField DPU" \
    guestiptype=Isolated traffictype=GUEST \
    supportedservices="SourceNat,Gateway" \
    "serviceProviderList[0].service=SourceNat" "serviceProviderList[0].provider=dpu-eswitch" \
    "serviceProviderList[1].service=Gateway"   "serviceProviderList[1].provider=dpu-eswitch" \
    "serviceCapabilityList[0].service=SourceNat" \
    "serviceCapabilityList[0].capabilitytype=SupportedSourceNatTypes" \
    "serviceCapabilityList[0].capabilityvalue=peraccount"
cmk updateNetworkOffering id=<offering-uuid> state=Enabled
```

Guests use CloudStack-allocated static IPs (no DHCP service is declared).

### 4. Create and implement an isolated network

```bash
cmk createNetwork name=my-dpu-net displaytext="My DPU network" \
    networkofferingid=<offering-uuid> zoneid=<zone-uuid>
```

`implement-network` creates, on the DPU: the vSwitch (`vs id = vlan`), the VR
(`vr id = vlan`), the private gateway RIF (`SW<vlan>`, gateway IP) and attaches
the VF pool.  `assign-ip` (source NAT) then attaches the parent port as the
public uplink RIF, sets its address, the default route and enables SNAT/PAT.

Guest VMs get a passed-through BlueField VF; the eSwitch FDB learns the VM's
MAC on first traffic — no per-NIC DPU configuration is needed.

---

## Data plane expectations and limitations

* One DPU per KVM host; every isolated network is pinned to exactly one DPU
  (`ensure-network-device` hashes `network_id` over the host list).
* One SNAT-able network per DPU today: the daemon permits one public uplink
  RIF per VR and a port serves one VR only.
* VLAN-tagged public traffic requires the uplink-VLAN capability in the
  daemon (phase-1 work); until then present the public side untagged.
* No DHCP/DNS/UserData (static-IP guests), no StaticNat/PortForwarding/Firewall
  yet (DNAT is not implemented in the daemon; declare only `SourceNat,Gateway`).
* Daemon limits apply: 64 vSwitches/VRs, 254 ports per vSwitch,
  NAT port range per VR.
* DPDK port IDs are never persisted; the wrapper re-resolves them from
  `port show` on every call and keys inventory by `host/pf/vf`.

---

## Offline testing

```bash
./run-tests.sh          # wrapper against mock-eswitchctl (44 assertions)
./run-proxy-tests.sh    # proxy ensure/forward paths with a stubbed ssh
```

The harnesses reset `TEST_ROOT` under `/tmp/opencode` and use
`DPU_ESWITCH_STATE_DIR`/`DPU_ESWITCH_LOG_FILE` overrides, so no `/var/lib` or
`/var/log` writes are needed.
