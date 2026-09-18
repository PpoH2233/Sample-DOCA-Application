# DPU-Offloaded Isolated Networks — Implementation Summary

This document summarizes the end-to-end implementation of **per-VM VF PCI-passthrough
for DPU-offloaded isolated networks** in Apache CloudStack, backed by the
`dpu-eswitch` NetworkOrchestrator extension and NVIDIA BlueField-3.

---

## 1. What was built

A CloudStack isolated guest network whose dataplane runs **entirely on the DPU**:

```
     Guest VM                     BlueField-3 (ARM)                    Public L2
  ┌────────────┐   PCI-passthrough    ┌──────────────────────────┐   ┌─────────────┐
  │  VM: ens5  │◄─── host VF ────────►│  vswitch (vs 212)        │   │ VLAN 6      │
  │ 10.66.0.x  │   VF 84:00.7 (vfio)  │  members: [vf 5, ...]    ├──►│ 203.0.113.x │
  └────────────┘                      │        ▲                 │   │ internet    │
                                      │  VR 212 (vRouter)        │   └─────────────┘
                                      │   SW RIF 10.66.0.1/24    │
                                      │   uplink vf 21 = SNAT    │
                                      │   203.0.113.14 (NAT/PAT) │
                                      └──────────────────────────┘
```

- **No host Linux bridge, no tap, no VLAN tagging for the guest** — the guest's
  only interface is a PCI-passthrough VF of the BlueField.
- **VFs are allocated lazily**: none is pre-attached at network implementation;
  one free VF (from the registered pool `vf.pool=5-20`) is allocated per VM NIC,
  its representor attached to the network's vSwitch, and its MAC pinned to the
  CloudStack NIC MAC so cloud-init's config-drive static config matches.
- **Lifecycle guarantees**:
  - VM deploy/start → `prepare-nic` allocates + attaches one VF.
  - VM destroy / NIC detach → `release-nic` detaches + frees.
  - Network delete → `destroy-network` removes the VR, the vSwitch **and frees
    any VF still recorded** for that network.
  - Network shutdown keeps allocations; a later implement/restore reconciles them.
  - Network restart (`restartNetwork cleanup=false`) re-ensures the device and
    re-attaches allocated VFs.

## 2. Architecture

```
CloudStack MS (mgmt-host)                          BlueField-3 (dpu-host)
┌─────────────────────────────┐                 ┌─────────────────────────────┐
│ NetworkExtensionElement     │                 │ dpu-eswitch-wrapper.sh      │
│  (Java, per extension)      │   SSH (root)    │  implement-network          │
│  ensure/implement/assign-ip │────────────────►│  prepare-nic / release-nic  │
│  assign-ip / release-ip     │  payload file   │  shutdown / destroy-network │
│  prepare-nic / release-nic  │                 │  assign-ip / release-ip     │
│                             │   JSON output   │        │                    │
│  • writes VM detail         │◄────────────────│        ▼                    │
│    kvm.pci.bus.addresses    │                 │  eswitchctl (control.sock)  │
│    (vm_instance_details)    │                 │  vs / vr / nat / devlink    │
└─────────────────────────────┘                 └─────────────────────────────┘
        │
        │ VM detail kvm.pci.bus.addresses
        ▼
HypervisorGuruBase.toVirtualMachineTO ──► VirtualMachineTO.pciBusAddresses
        │                                       │
        ▼                                       ▼
KVM agent: attachPciDevices() creates <hostdev managed="yes"> (libvirt auto-binds
vfio-pci at start, rebinds mlx5_core at stop) and SKIPS the bridge vif for
BroadcastDomainType.Dpu nics (createVif guard).
```

**Data path**: guest frames → SR-IOV VF → DPU representor → vswitch (`vs <id>`)
→ VR (`vr <id>`) → SW RIF (10.66.0.1/24, gateway) → uplink port-link (vf 21,
SNAT 203.0.113.14/24, NAT/PAT TCP/UDP ports 20000–60999) → VLAN-6 bridge on the
host → public gateway. ICMP echo is NAT'ed on the ARM slow path.

**Guest configuration** (no DHCP): the offering declares
`Dhcp` + `UserData` with the **ConfigDrive** provider. The MS builds a
config-drive ISO containing `openstack/latest/network_data.json` (per-NIC
static ip/gateway/netmask/MAC — works because the offering has a Dhcp service)
plus user data; cloud-init inside the guest applies the static config to the
interface whose MAC equals the NIC MAC (the VF MAC is pinned by the wrapper via
`devlink port function set … hw_addr`).

## 3. Code changes

### cloudstack fork — branch `ce-project/dpu-offload` (from `nacl/4.23`)

| Commit | Content |
| --- | --- |
| `561b0e1fba` (cherry-pick of `04a3f5d619`) | PCI passthrough plumbing: `pcibusaddresses` deploy param → VM detail `kvm.pci.bus.addresses` → `VirtualMachineTO.pciBusAddresses` → KVM `attachPciDevices()` → `<hostdev managed="yes">` via `LibvirtGpuDef.defPci()` |
| `25d20e6a4a` | `Networks.BroadcastDomainType.Dpu("dpu", String)` — new isolation scheme (`dpu://`) whose guest NICs get **no tap/bridge**: guard in `LibvirtComputingResource.createVif` + `LibvirtPlugNicCommandWrapper` |
|  | `NetworkExtensionElement`: `prepare-nic`/`release-nic` payloads carry `vm_uuid`, `instance_name` and the VM's `kvm.*` details; script output keys `vm.pci.bus.addresses` / `vm.pci.bus.addresses.remove` are merged/stripped into the VM detail `kvm.pci.bus.addresses` (via the already-injected `VMInstanceDetailsDao`) — this is what makes the guru provision the hostdev automatically, with **zero user input** |

Build: `mvn -DskipTests install` at the repo root; deploy artifacts:
- `client/target/cloud-client-ui-4.23.0.0.jar` → `/usr/share/cloudstack-management/lib/cloudstack-4.23.0.0.jar` (shaded fat jar)
- `api/target/cloud-api-4.23.0.0.jar` → `/usr/share/cloudstack-agent/lib/cloud-api-4.23.0.0.jar`
- `plugins/hypervisors/kvm/target/cloud-plugin-hypervisor-kvm-4.23.0.0.jar` → `/usr/share/cloudstack-agent/lib/cloud-plugin-hypervisor-kvm-4.23.0.0.jar`

Restart `cloudstack-management` and `cloudstack-agent` afterwards.

### cloudstack-extensions — branch `dpu-eswitch`

`dpu-eswitch/dpu-eswitch-wrapper.sh` (runs on the BlueField; the MS-side proxy
`dpu-eswitch.sh` is unchanged):

- `implement-network` — creates vSwitch + VR + SW RIF; **no VF is attached**;
  emits `{"status":"success","network.broadcast_domain_type":"Dpu","network.broadcast_uri":"dpu://<vs-id>"}` (required by `NetworkExtensionGuestNetworkGuru`).
- `prepare-nic` — re-ensures the device (recovers networks shut down while
  VMs were stopped), allocates the lowest free pool VF (skips VFs without a
  probed representor, the public uplink VF, and VFs already allocated to any
  NIC or member of another vSwitch), attaches the representor, pins the VF MAC
  via devlink (3 retries), records `nic_uuid → {vf, pci, port}` in a DPU-wide
  `vf-alloc.txt`, and returns the PCI BDF.
- `release-nic` — detaches the representor and frees the VF; returns the
  removed PCI address so the element can clean the VM detail.
- `destroy-network` — frees every allocation of that network, then tears down
  the VR and the vSwitch; `shutdown-network` keeps allocations (VMs may restart).
- `restore-network` / `implement-network` — reconcile: re-attach every VF
  recorded for the network.

Offline tests: `dpu-eswitch/run-tests.sh` — **70/70 pass** (against
`mock-eswitchctl`); `run-proxy-tests.sh` — **14/14 pass**.

## 4. CloudStack objects used in the lab

| Object | Value |
| --- | --- |
| Extension | `dpu-eswitch` (id `<extension-uuid>`), details: `network.services=SourceNat,Gateway`, `network.service.capabilities={...}`, **`network.isolation.method=NetworkExtension`** |
| Registration | Physical Network 1 (resource-map details: `hosts=192.0.2.2`, `username=root`, `vf.pool=5-20`, `uplink.port=21`, `nat.port.range=20000-60999`) |
| Offering | **DPU Offloaded SNAT** (`<offering-uuid>`): Isolated/Guest, SourceNat→dpu-eswitch, Dhcp→ConfigDrive, UserData→ConfigDrive |
| Network | `dpu-net-2` (`<network-uuid>`), CIDR 10.66.0.0/24, broadcast `dpu://212` |
| SNAT IP | 203.0.113.14 (uplink = vf 21 via host VLAN-6 bridge — temporary, as allowed) |
| Test VMs | `vm-1` (10.66.0.171), `vm-2` (10.66.0.43, destroyed after the test) |

## 5. E2E results (all verified live)

- ✅ `vm-1` deployed on the offloaded network; guest `ens5` = VF 84:00.7;
  `virsh domiflist` shows **no interfaces** (no host bridge).
- ✅ `vm-1` → gateway `10.66.0.1` (the DPU VR) — 2/2 replies.
- ✅ `vm-2` deployed → second VF (84:01.0); `vm-1 ↔ vm-2` ping 2/2 both ways
  (switching inside `vs 212`).
- ✅ Both VMs `ping 8.8.8.8` → 2/2 replies each (SNAT via vf 21).
- ✅ `vm-2` destroyed → `release-nic` freed vf 6 (`vs 212 ports=[6]`).
- ✅ Network restart succeeded; vSwitch/VR recreated, VF re-attached.
- ✅ Root password `<root-password>` on test VMs (baked into the template root disk;
  cloud-init handles the static network config).

## 6. Reproduction guide

### Prerequisites
- KVM host `mgmt-host` with management server (4.23.0.0, `nacl/4.23` + patches),
  KVM agent, and BlueField-3 reachable at `192.0.2.2` (root SSH from the MS,
  key at `/var/lib/cloudstack/management/.ssh/id_ed25519`).
- BlueField running the `eswitch-management` daemon (docker, control socket
  `/run/eswitch-management/control.sock`) and `eswitchctl` in `/usr/local/bin`.
- Host: vf 21 (uplink) bound to `mlx5_core`, netdev `ens6f0v21` promisc +
  enslaved to the VLAN-6 bridge `brens3f0np0-6`; vfio usable (IOMMU on).

### Install

1. **Build + deploy the fork** (branch `ce-project/dpu-offload`):
   ```bash
   cd cloudstack && git checkout ce-project/dpu-offload
   mvn -DskipTests install
   scp client/target/cloud-client-ui-4.23.0.0.jar \
       mgmt-host:/usr/share/cloudstack-management/lib/cloudstack-4.23.0.0.jar
   scp api/target/cloud-api-4.23.0.0.jar \
       mgmt-host:/usr/share/cloudstack-agent/lib/cloud-api-4.23.0.0.jar
   scp plugins/hypervisors/kvm/target/cloud-plugin-hypervisor-kvm-4.23.0.0.jar \
       mgmt-host:/usr/share/cloudstack-agent/lib/cloud-plugin-hypervisor-kvm-4.23.0.0.jar
   ssh mgmt-host 'systemctl restart cloudstack-management cloudstack-agent'
   ```
2. **Deploy the extension scripts** (branch `dpu-eswitch` of cloudstack-extensions):
   - proxy `/usr/share/cloudstack-management/extensions/dpu-eswitch/dpu-eswitch.sh`
     (+ symlink `dpu-eswitch` without `.sh`) on mgmt-host;
   - wrapper `/etc/cloudstack/extensions/dpu-eswitch/dpu-eswitch-wrapper.sh`
     (chmod +x) on dpu-host.
3. **Register the extension** (details include `network.isolation.method=NetworkExtension`):
   ```bash
   cmk updateExtension id=<extension-uuid> \
     "details[0].network.services=SourceNat,Gateway" \
     "details[1].network.service.capabilities={\"SourceNat\":{\"SupportedSourceNatTypes\":\"peraccount\",\"RedundantRouter\":\"false\"}}" \
     "details[2].network.isolation.method=NetworkExtension"
   cmk updateRegisteredExtension extensionid=<extension-uuid> \
     resourcetype=PhysicalNetwork resourceid=<physical-network-uuid> \
     "details[0].hosts=192.0.2.2" "details[1].username=root" \
     "details[2].vf.pool=5-20" "details[3].uplink.port=21" "details[4].nat.port.range=20000-60999"
   ```
4. **Offering** (already created; recreate if needed):
   ```bash
   cmk createNetworkOffering name="DPU Offloaded SNAT" ... \
     supportedServices=SourceNat,Dhcp,UserData \
     "serviceProviderList[0].service=SourceNat" "serviceProviderList[0].provider=dpu-eswitch" \
     "serviceProviderList[1].service=Dhcp"    "serviceProviderList[1].provider=ConfigDrive" \
     "serviceProviderList[2].service=UserData" "serviceProviderList[2].provider=ConfigDrive" \
     specifyvlan=false state=Enabled
   ```
5. **Network**: `cmk createNetwork name=dpu-net-2 networkofferingid=<uuid> zoneid=... gateway=10.66.0.1 netmask=255.255.255.0 startip=10.66.0.10 endip=10.66.0.100`.
6. **Deploy a VM** (no PCI address anywhere — fully automatic):
   - guest config is delivered by the config drive; to set the root password and
     per-VM IP, deploy with `startvm=false`, read the allocated IP
     (`listVirtualMachines`), then `updateVirtualMachine userdata=<base64 #cloud-config>`
     with `bootcmd` entries that bring up `ens5` with that IP, and start the VM.
   - the test template's root password (`<root-password>`) is baked into the template
     image (see below).
7. **Verify**:
   ```bash
    virsh domiflist <vm-i-2-62-VM>    # no interfaces; dumpxml shows <hostdev>
   eswitchctl vs show               # vs 212 ports=[6,...] — one port per VM
   eswitchctl fdb show --id 212     # learned guest MACs
   eswitchctl vr show --id 212      # SW RIF + uplink ACTIVE_ARM_UPLINK/NAT
   ```
   In-guest: `ping 10.66.0.1`, ping the other VM, `ping 8.8.8.8`.

### Template root password

The test template (`<template-uuid>`, Ubuntu 24.04, local primary) is linked as the
backing file of each VM disk. The root password was baked by:
1. stopping the VMs holding the template file,
2. `cp <template> test-root-template.qcow2`, editing the copy via
   `qemu-nbd + mount + chroot (echo root:<root-password> | chpasswd; PermitRootLogin yes)`,
3. overwriting the template file `/mnt/<primary-pool>/<template-uuid>` with the
   copy (the template is recreated by a fresh VM deploy — an existing VM's
   overlay keeps its stale shadow block).

### Offline tests

```bash
cd cloudstack-extensions && ./dpu-eswitch/run-tests.sh      # 70 pass
./dpu-eswitch/run-proxy-tests.sh                            # 14 pass
```

## 7. Known limitations / notes

- PoC scope: single host/DPU, no live migration, no hot-attach of a DPU NIC
  after start (PlugNic returns success without a tap for `Dpu` nics).
- The eswitch daemon supports **one SNAT network per DPU** (single uplink RIF);
  static NAT / multiple public IPs per VR are not exposed by the daemon CLI.
- VM stop keeps the VF allocated (per-IP-lease semantics); `prepare-nic`
  re-attaches it on restart. VM delete/expunge frees it.
- VMware/XenServer would reject the new `Dpu` broadcast type (not used here).
- VM IP configuration is static via config-drive `bootcmd`/cloud-init; there is
  no DHCP in the offloaded network.
