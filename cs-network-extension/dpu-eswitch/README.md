# CloudStack / BlueField eSwitch extension

Provides `SourceNat,Gateway,Firewall,PortForwarding` for isolated networks.
Egress belongs to Firewall: the offering default selects allow-by-default
with deny rules, or deny-by-default with allow rules. Create the offering after
the DPU deployment; see [IMPLEMENTATION.md](IMPLEMENTATION.md).

```text
NetworkExtensionElement -> dpu-eswitch.sh (Management Server)
  -> SSH + JSON payload file -> dpu-eswitch-wrapper.sh (BlueField Arm)
    -> dpu_eswitch.py -> eswitchctl or Unix control.sock
      -> sole eswitch-management daemon -> DOCA Flow
```

Install both the shell wrapper and adjacent `dpu_eswitch.py` on Arm, with
Python3.9+. Invocation: `wrapper COMMAND PAYLOAD.json TIMEOUT_SECONDS`.
Success is one JSON line with `status=success`; errors return nonzero with
actionable stderr. `status` returns daemon/port output as JSON fields.

## Topology

- Shared VS6: p0 and host VF0 are separate **single-VID trunk6** members.
  Both physical links carry VID6; the internal router domain is untagged.
  Tenant cleanup never removes VS6 or either member.
- Guest VID900–999 maps directly to matching VS/VR IDs and SW<vid> private
  gateway RIF. No network-ID hash fallback. Guest VFs are untagged access
  members; VF0 is reserved and cannot appear in `vf.pool`.
- Each VR has a logical public `uplink` vs-link on VS6 with a unique MAC/IP.
  Phase1 accepts one public/SNAT IP per VR and PF only on that IP.
- A persistent DPU-wide flock serializes shared infrastructure, VF allocation,
  IP/VID ownership and policies. Runtime port IDs are never cached:
  `port show --all` resolves stable `(host,pf,vf)` even after assignment/restart.
- Existing networks remain pinned to their DPU; no automatic failover to a
  different DPU without moving VM/network state.

## Supported callbacks

| Callback | Result |
| --- | --- |
| `ensure-network-device` (proxy) | Select/revalidate DPU and return host/VID IDs |
| `ensure-infrastructure` (wrapper) | Independently reconcile shared VS6 |
| `implement-network` | Guest VS/VR/gateway; no VF allocation |
| `prepare-nic` / `release-nic` | Allocate/attach/pin MAC or detach/free one VF; return KVM PCI metadata |
| `assign-ip` | Public VS-RIF, IP, default route and NAT |
| `release-ip` | Remove PF/ingress/NAT/public route/IP/RIF; retain VS6 and egress |
| `apply-fw-rules` | Full ingress/egress snapshot reconciliation |
| `add-port-forward` / `delete-port-forward` | Idempotent TCP/UDP PF reconciliation |
| `shutdown-network` | Remove tenant dataplane, retain cache/VF reservations |
| `destroy-network` | Remove tenant dataplane/cache and its VF reservations |
| `restore-network` | Repair topology/VFs/public/NAT/firewall/PF against live state |

Firewall supports TCP/UDP/ICMP/all, IPv4 source/destination CIDRs, TCP/UDP
destination port spans and ICMP type/code (`-1` means any). Multiple CIDRs
expand into deterministic, collision-free IDs per direction, starting at2.
ID1 is reserved for a highest-priority deny-all update fence. Ingress is
default-deny; egress actions follow the offering default. Unsupported shapes
are rejected before mutation. Failed replacements retain/re-establish the
fence and keep the previous committed cache. Retry a valid full snapshot to
recover; replacement temporarily interrupts traffic. Identical snapshots are
compared with live policies/rules and avoid unnecessary hardware rewrites.
New public RIFs start default-deny before the first firewall callback.

If first implementation fails before acquiring a VID, cleanup callbacks can
acknowledge an already-empty DPU without guessing tenant identity. This requires
live zero routers, no switches except VS6, no tenant caches or VF allocations,
and no unresolved VR/VS identity. Empty firewall snapshots also permit this
cleanup path; normal provisioning and nonempty rules still require a valid VID.
This acknowledgement does not release CloudStack's database IP association:
retry the network deletion through CloudStack after deploying the wrapper.

Upstream PF callbacks lack a rule ID. The stable key is
`protocol:public_ip:public_span`; numeric IDs1–65535 persist separately.
TCP/UDP colon/hyphen port spans require equal public/private widths and a guest
private IP within the VR CIDR. Changing the private target reuses the ID.
A delayed revoke for an old private target cannot delete its replacement.

Cache files are fsync/rename JSON under `/var/lib/cloudstack/dpu-eswitch`.
Full CloudStack firewall callbacks supersede cached rules, including deletions.
`restore_data` contains NIC metadata, not firewall/PF desired state; CloudStack
restores those through dedicated callbacks. Its current upstream aggregate
restore skips this provider when it owns no DHCP/DNS/UserData service, so
recovery also uses dedicated callbacks and daemon persistence. After cache
loss, replay CloudStack desired state; do not guess VF/tenant ownership.

## Registration

Declare `network.services=SourceNat,Gateway,Firewall,PortForwarding` and
`network.isolation.method=NetworkExtension`; keep SourceNat non-redundant and
per-account. ConfigDrive can supply guest static network settings/UserData.

| Resource registration detail | Meaning |
| --- | --- |
| `hosts`, `username`, `port`, `sshkey`/`password` | Proxy SSH connection to Arm |
| `vf.pool` | Guest indexes/ranges, default1–50; VF0 forbidden |
| `vf.host`, `vf.pf` | Representor identity, defaults1,0 |
| `vf.pci.map` | JSON mapping VF index to **KVM host** PCI BDF |
| `host.pf.pci` | Alternative discovery from host PF `virtfnN` if visible |
| `nat.port.range` | Default20000–60999 |
| `eswitchctl.path` | CLI path; default PATH, then direct Unix socket |
| `control.socket` | Default `/run/eswitch-management/control.sock` |
| `router.state` | Default `/var/lib/eswitch-management/eswitch.conf.router` |
| `vf.mac.preconfigured` | `true` only when devlink absent and MACs already pinned |

Arm PCI addresses cannot reliably identify KVM-host VFs. Obtain host
`virtfn*` symlink targets and register `vf.pci.map` (object or JSON string).
Alternatively install an operator-verified local JSON mapping at
`/etc/cloudstack/extensions/dpu-eswitch/vf-pci-map.json`. The verified
zona-01/PF0000:84:00.0 mapping is provided in `zona-01-vf-pci-map.json`,
with guest VF1–50 only (VF0 reserved). This mapping is specific to that host,
not portable to other DPUs/hosts. Explicit registration mapping/PF overrides
the local file. Allocation chooses the first free VF and preserves it on retry.
The old hard-coded VF0–21 mapping is removed; VF22–50 use explicit/discovered
mapping too. Nonempty legacy `vf-alloc.txt` requires explicit migration.

## Verification

```bash
bash cs-network-extension/dpu-eswitch/run-tests.sh
bash cs-network-extension/dpu-eswitch/run-proxy-tests.sh
docker exec doca-devel meson compile -C /build/eswitch-management
docker exec doca-devel meson test -C /build/eswitch-management --print-errorlogs
docker build -f application/eswitch_management/Dockerfile \
  -t eswitch-management:3.4.0-cloudstack .
```

Offline tests cover lifecycle, replay/update/delete, two shared-WAN VRs,
malformed payloads, unsupported rules, restore, VF50 and injected failures.
See [IMPLEMENTATION.md](IMPLEMENTATION.md) for payload provenance and rollout.

Limits: 64 VS/VR slots (VS6 consumes one VS), global rule/resource capacities.
No StaticNat/LB/VPN/VPC/multi-host L2 or DPU DHCP/DNS. ICMP Echo NAT remains
Arm. Existing TCP/UDP CT and ingress/egress DOCA offload semantics remain;
validate actual hardware hits under controlled traffic after deployment.
