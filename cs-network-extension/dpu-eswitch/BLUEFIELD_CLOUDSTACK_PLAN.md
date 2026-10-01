# BlueField–CloudStack Integration Plan

Implementation handoff for an agent running on the BlueField-3 with the installed DOCA SDK. The eSwitch application has already passed initial hardware testing. The goal is to create isolated networks, assign public IPs, and manage ingress firewall, egress firewall, and port forwarding through the CloudStack UI. CloudStack desired state and DPU state must converge without manual `eswitchctl` commands.

## Target topology

```text
physical switch -- tagged VLAN 6 --> BF3 p0
                                    |
                                    +-- VS6 -- tagged VLAN 6 --> host VF0 --> cloudbr0
                                    |    +-- public RIFs of VR900, VR901, ...
                                    +-- VS900 ... VS999
                                         +-- private RIF of the matching VR
                                         +-- guest VFs (untagged frames)
```

- `VS6` is shared infrastructure and survives individual network/IP deletion. Attach `p0` and host VF0 as separate **single-VID trunk 6** members. Frames on both links must carry VID 6. The daemon pops the tag inside VS6 and pushes it on trunk egress.
- A public RIF is a logical `vs-link` attached to VS6, **not** a physical access port. Each VR gets a unique public RIF MAC/IP. Multiple VRs may share VS6.
- CloudStack DPU guest networks receive VIDs 900–999. For VID 900, create numeric VS ID 900, display name `VS900`, VR ID 900, and private RIF `SW900`. Guest VFs are untagged access members. Do not attach guest VSs to `p0` or VF0 automatically.
- VF0 is reserved for the host trunk and must be excluded from the guest `vf.pool`. Resolve `p0` and VF0 from `port show` and stable `(host,pf,vf)` identity on every reconciliation; DPDK port IDs are not stable across daemon restarts.

## Required changes

### 1. Baseline and infrastructure

1. Read repository `AGENTS.md`, eSwitch `README.md`, `CLI.md`, and `AUTHORIZED_CT_TESTING.md`. Record Git revision, `pkg-config --modversion doca-common doca-flow`, `eswitchctl status`, `port show`, `vs show`, `vr show`, and current persisted state before editing. Verify APIs against the installed headers and shipped samples.
2. Identify the real BF3 `p0`, host VF0 representor, host VF0 PCI BDF, guest VF pool, physical-switch port, and host bridge. Stop if VF0 belongs to a VM or `cloudbr0` has another path back to physical VLAN 6.
3. Provision/reconcile VS6 independently of any CloudStack network. Verify `(p0,6)` and `(VF0,6)` ownership before attaching. Use `vs port attach --id 6 --port <runtime-id> --mode trunk --vlan 6` for each. Never take a VLAN membership from another VS.
4. Add an optional persistent VS name to the eSwitch CLI/model/state so `vs create --id 900 --name VS900` and `vs show --id 900` expose `VS900` after restart. Preserve compatibility with the existing numeric-only command and state files. The CLI currently has no VS name field.

### 2. CloudStack network and IP lifecycle

1. Reserve VNET/VLAN pool 900–999 for the DPU physical network/offering; prevent other offerings from consuming it. Reject missing/out-of-range VID and remove the wrapper's `network_id` hash fallback for DPU networks. Persist and validate `CloudStack network ID -> VID -> VS/VR` on every call, including reimplementation after shutdown.
2. `implement-network`: create/reconcile `VS<vid>`, `VR<vid>`, private RIF, and gateway IP. Allocate a guest VF only at `prepare-nic`.
3. `assign-ip`: require public VID 6; ensure shared VS6, attach the VR public RIF to VS6, then configure its MAC/IP, default route, and NAT in CLI dependency order. Detect duplicate public IP/MAC and return an actionable error to CloudStack.
4. `release-ip`, `shutdown-network`, and `destroy-network`: remove dependent policy/PF/CT/NAT/routes/IPs/RIFs, guest allocations, VR, and guest VS as appropriate. **Never delete VS6, its p0/VF0 members, or another VR's RIF.** Make repeated calls idempotent and do not report failed cleanup as success.
5. `restore-network` and daemon restart must reconcile all VS/RIF/VF/IP/NAT/firewall/PF state against CloudStack desired state. Wrapper files are a cache, not the sole authority.

### 3. CloudStack network services

1. Advertise `SourceNat,Gateway,Firewall,PortForwarding` in `network.services`; use `dpu-eswitch` as the offering provider for those services. Egress rules belong to `Firewall` and its offering-level default egress policy. Verify the callbacks and payload schema of the installed CloudStack version. Existing `NetworkExtensionElement` supports `add-port-forward`, `delete-port-forward`, and `apply-fw-rules`; no CloudStack rebuild is expected if that implementation and the DPU PCI integration are already installed.
2. Add `apply-fw-rules` to the wrapper. Validate the JSON `fw_rules` payload. Map ingress rules to the VR public RIF on VS6 and egress rules to private RIF `SW<vid>`. Expand multiple source CIDRs into stable, collision-free rule IDs. Map protocol, ports, ICMP type/code, and default egress policy without weakening unsupported rules. Reconcile additions, changes, and deletions from a full CloudStack rule snapshot; reject unrepresentable policy rather than silently allowing traffic.
3. Add idempotent `add-port-forward` and `delete-port-forward`. Validate TCP/UDP, equal public/private port spans, public IP ownership, and a target private IP reachable within the intended VR. Persist a stable mapping from CloudStack rule identity to numeric `eswitchctl` rule ID `1..65535`; release it on deletion.
4. Phase 1 supports **one public IP per VR, and PF only on that VR's SNAT/public-RIF IP**. Reject another public IP explicitly. The current daemon has one IPv4 address per RIF and PF uses that RIF's IP. Do not advertise StaticNat, load balancing, VPC, VPN, or DPU DHCP/DNS without separate implementation and tests.
5. Update wrapper/proxy mock tests for create/update/delete, duplicate/replayed events, restore, malformed payloads, unsupported rules, and partial failures. Run `run-tests.sh` and `run-proxy-tests.sh`.

## BF3 and CloudStack acceptance tests

1. Build with Meson on BF3, run `meson test`, verify installed DOCA versions and `ldd` linkage to `libdoca_flow`. Restart the sole eSwitch owner only in a maintenance window.
2. Send VLAN 6 traffic in both directions between physical switch and VF0. Verify VS6 selection, VID 6 on the wire toward `cloudbr0`, isolation from other VIDs, and no unintended untagged acceptance. Confirm tags from more than one observation point because NIC VLAN offload can hide them in a local capture.
3. Create an isolated network in the UI with VID 900. Verify `vs show` reports `id=900 name=VS900`, `vr show --id 900` shows the private RIF, guest VF frames are untagged, and two VMs can reach their gateway and each other. Reject VID 899 and 1000.
4. Assign a VLAN 6 public IP in the UI. Verify VR900's public RIF/IP/MAC, upstream ARP, SNAT, and replies. Add VR901 on VS6; removing VR900 must leave VR901 and VS6 operational.
5. Create, edit, and delete ingress firewall, egress firewall, and PF rules in the UI. Test allowed/denied TCP, UDP, supported ICMP, source CIDRs, default egress policy, PF replies, live revocation, and restore after restart. Compare packet captures with `eswitchctl status` counters (`public_ingress_hw`, `egress_acl`, `egress_authorized`, `hw_ct_state`, `ct_authorization`, promotions, failures, and hits). Pipe creation alone does not prove hardware packet hits.
6. Confirm there is no duplicate L2 path/loop and that isolated-network lifecycle operations never delete VS6.

## Limits and handoff evidence

- The daemon currently has 64 VS and 64 VR slots. VID range 900–999 contains 100 values but cannot provide 100 simultaneous networks on one DPU; include VS6 in capacity accounting and reject exhaustion cleanly.
- A network spanning multiple KVM hosts needs a separate inter-DPU L2 design. Pin each network to the host/DPU owning its guest VFs. If that DPU fails, do not hash the existing network to another DPU without moving VM and network state.
- Check that `cloudbr0` has no second physical route to VLAN 6. eSwitch split horizon does not prevent an external L2 loop.
- Report changed files, actual CloudStack payload samples, build/test output, packet and offload counters, unsupported rule shapes, state migration, and rollback procedure. Do not change BFB, firmware, or DPU mode for this work.

## Local references

- [eSwitch CLI](../../application/eswitch_management/CLI.md)
- [eSwitch README](../../application/eswitch_management/README.md)
- [Authorized CT tests](../../application/eswitch_management/AUTHORIZED_CT_TESTING.md)
- [Extension README](README.md)
- [Extension design notes](IMPLEMENTATION.md)
