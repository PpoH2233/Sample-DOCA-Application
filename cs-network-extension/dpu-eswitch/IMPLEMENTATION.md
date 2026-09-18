# DPU-Offloaded Isolated Networks — Design Notes

Design notes for the **per-VM VF PCI-passthrough for DPU-offloaded isolated
networks** feature in Apache CloudStack, backed by the `dpu-eswitch`
NetworkOrchestrator extension and NVIDIA BlueField-3.

---

## 1. Overview

A CloudStack isolated guest network whose dataplane runs **entirely on the DPU**:

```
     Guest VM                     BlueField-3 (ARM)                    Public L2
  ┌────────────┐   PCI-passthrough     ┌──────────────────────────┐   ┌─────────────┐
  │  VM: ens5  │◄─── host VF ────────► │  vswitch (vs 212)        │   │ VLAN 6      │
  │ 10.66.0.x  │   VF 84:00.7 (vfio)   │  members: [vf 5, ...]    ├──►│ 203.0.113.x │
  └────────────┘                       │        ▲                 │   │ internet    │
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

## 3. Key design points

### CloudStack side

- **`Networks.BroadcastDomainType.Dpu`** (`dpu://`): new isolation scheme whose
  guest NICs get **no tap/bridge** — guarded in `LibvirtComputingResource.createVif`
  and `LibvirtPlugNicCommandWrapper`. VMware/XenServer would reject this type.
- **PCI passthrough plumbing**: `pcibusaddresses` deploy param → VM detail
  `kvm.pci.bus.addresses` → `VirtualMachineTO.pciBusAddresses` → KVM
  `attachPciDevices()` → `<hostdev managed="yes">` via `LibvirtGpuDef.defPci()`.
- **`NetworkExtensionElement`**: `prepare-nic`/`release-nic` payloads carry
  `vm_uuid`, `instance_name` and the VM's `kvm.*` details; script output keys
  `vm.pci.bus.addresses` / `vm.pci.bus.addresses.remove` are merged/stripped into
  the VM detail `kvm.pci.bus.addresses` (via the injected `VMInstanceDetailsDao`).
  This is what makes the guru provision the hostdev automatically, with
  **zero user input**.

### Extension side (`dpu-eswitch-wrapper.sh`, runs on the BlueField)

The MS-side proxy `dpu-eswitch.sh` is unchanged.

- `implement-network` — creates vSwitch + VR + SW RIF; **no VF is attached**;
  emits `{"status":"success","network.broadcast_domain_type":"Dpu","network.broadcast_uri":"dpu://<vs-id>"}`
  (required by `NetworkExtensionGuestNetworkGuru`).
- `prepare-nic` — re-ensures the device (recovers networks shut down while
  VMs were stopped), allocates the lowest free pool VF (skips VFs without a
  probed representor, the public uplink VF, and VFs already allocated to any
  NIC or member of another vSwitch), attaches the representor, pins the VF MAC
  via devlink, records `nic_uuid → {vf, pci, port}` in a DPU-wide
  `vf-alloc.txt`, and returns the PCI BDF.
- `release-nic` — detaches the representor and frees the VF; returns the
  removed PCI address so the element can clean the VM detail.
- `destroy-network` — frees every allocation of that network, then tears down
  the VR and the vSwitch; `shutdown-network` keeps allocations (VMs may restart).
- `restore-network` / `implement-network` — reconcile: re-attach every VF
  recorded for the network.

### Extension registration (required details)

- Extension details: `network.services=SourceNat,Gateway`,
  `network.service.capabilities` (peraccount SourceNat, non-redundant),
  **`network.isolation.method=NetworkExtension`**.
- Resource-map details (per physical network): `hosts`, `username`,
  `vf.pool` (e.g. `5-20`), `uplink.port` (e.g. `21`),
  `nat.port.range` (e.g. `20000-60999`).

### Network offering

Isolated/Guest offering: SourceNat → `dpu-eswitch`, Dhcp → ConfigDrive,
UserData → ConfigDrive (ConfigDrive is what delivers the static guest config).

## 4. Testing

- Offline tests: `dpu-eswitch/run-tests.sh` (against `mock-eswitchctl`) and
  `dpu-eswitch/run-proxy-tests.sh`.
- Verification on a live DPU:
  ```bash
  virsh domiflist <vm-name>       # no interfaces; dumpxml shows <hostdev>
  eswitchctl vs show              # one port per VM on the network's vswitch
  eswitchctl fdb show --id <vs>   # learned guest MACs
  eswitchctl vr show --id <vr>    # SW RIF + uplink ACTIVE_ARM_UPLINK/NAT
  ```
  In-guest: ping the gateway (the DPU VR), the other VM, and a public IP.

## 5. Known limitations / notes

- PoC scope: single host/DPU, no live migration, no hot-attach of a DPU NIC
  after start (PlugNic returns success without a tap for `Dpu` nics).
- The eswitch daemon supports **one SNAT network per DPU** (single uplink RIF);
  static NAT / multiple public IPs per VR are not exposed by the daemon CLI.
- VM stop keeps the VF allocated (per-IP-lease semantics); `prepare-nic`
  re-attaches it on restart. VM delete/expunge frees it.
- VM IP configuration is static via config-drive `bootcmd`/cloud-init; there is
  no DHCP in the offloaded network.
