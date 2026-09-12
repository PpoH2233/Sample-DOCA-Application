# Router implementation status

This change introduces the control-plane foundation and a private gateway ARP
responder. It does **not** implement IPv4 router forwarding or ICMP echo replies.
Interfaces with IPs still report `PENDING_DATAPLANE`, while daemon status
reports `router_dataplane=GATEWAY_ARP_ONLY`. Public ports are reserved but
remain root-miss DROP. Private L2 forwarding remains active.
No API stub returns a fabricated hardware success.

## Private gateway ARP through the Arm system SF

The old parent-PF TX probe has been replaced by the same topology proven by the
SF-to-VF10 PoC. RX ARP dispatch and the portable 60-byte ARP response builder
are unchanged. The return path is multi-VS capable and fail-closed.

Software path:
1. Validate gateway request and current VS ownership.
2. Learn the requesting VM source MAC in the normal VS FDB.
3. Bind the RIF source MAC to that VS in `ESW_SF_RETURN`.
4. Build a padded 60-byte Ethernet ARP reply.
5. Send the complete frame through an `AF_PACKET/SOCK_RAW` socket bound to the
   actual Arm SF endpoint (`ESWITCH_SF_IFACE`, default `enp3s0f0s0`).

Hardware path (DEFAULT domain):
```text
actual Arm SF -> system-SF representor
  -> ESW_INGRESS_CLASSIFIER: SF port -> ESW_SF_RETURN
     -> known RIF source MAC: set pkt_meta=(VS << 16) | SF_port
        -> ESW_DEST_FDB: (VS, VM destination MAC)
           -> ESW_EGRESS_GATE_<VF> -> VF -> VM
     -> unknown RIF source MAC: DROP
```

The SF root entry and SF-return miss both fail closed. The SF cannot be attached
as a tenant port. A RIF-to-VS binding is installed on the first reply and reused.
Changing an already-used RIF MAC currently requires a daemon restart so the old
hardware binding is removed; configure RIF MACs before traffic during this
milestone.

Launch in doca-dev with the production PF owner stopped:
```sh
export ESWITCH_CONTROL_SOCKET=/run/eswitch-router-test/control.sock
export ESWITCH_STATE_FILE=/var/lib/eswitch-router-test/eswitch.conf
export ESWITCH_SF_IFACE=enp3s0f0s0
ip link set dev "$ESWITCH_SF_IFACE" up
./eswitch-management -l 0 -- 03:00.0
```
The process needs `CAP_NET_RAW` (or root). In a container use host networking so
the actual SF netdev is visible. Keep the established VF scope; it never changes
which system SF is selected.

VM: `sudo arping -I ens6 -c 5 192.168.0.1`, alongside
`sudo tcpdump -eni ens6 -nn arp`. Acceptance: the VM receives a reply whose
source MAC is the configured RIF MAC; `status` increments `arp_sf_tx_sent` and
shows at least one `sf_return_contexts`. Then verify ordinary L2 forwarding,
reject unknown SF source MACs, remove the gateway IP and verify replies stop.
ICMP replies and IPv4 forwarding remain outside this milestone. A successful
`sendto()` is not delivery proof; the VM capture is authoritative.

For the `VM ping gateway` use case, this milestone completes only the first
phase: ARP resolution of the gateway IP. `ping` should populate the VM neighbor
entry with the RIF MAC, but it will not receive an ICMP echo reply until the
local-ICMP VR pipe and Arm handler are implemented in the next milestone.

## Layout

```text
eswitch_management/
  main.c / eswitch_manager.c   runtime, command dispatch, persistence coordination
  control/                    existing Unix socket transport
  l2/                         vSwitch membership and MAC learning/FDB
  pipeline/                   shared DOCA root, L2 pipes, RSS, flooding and gates
  router/
    router.c / router.h       SDK-independent model, validation, CLI parser
    router_control.c          inventory/ownership integration and transactions
    router_state.c            versioned persistent desired configuration
    router_test.c             model/isolation/persistence tests
```

## Commands implemented

Use actual DPDK IDs from `list-port-available`; VF 11 is **not necessarily** DPDK
port 11. Choose a public port whose inventory says VF 11–15. Parent is rejected
as a VR uplink because this topology requires a host VF.

```sh
eswitchctl vr create --id 100
eswitchctl vr port-attach --id 100 --port <actual-public-dpdk-id> --name p1
eswitchctl vr switch-attach --id 100 --switch-id 200 --name p2
eswitchctl vr interface set --id 100 --interface p1 --mac <public-vf-mac>
eswitchctl vr ip add --id 100 --interface p2 --address 192.168.0.1/24
eswitchctl vr ip add --id 100 --interface p1 --address 200.20.0.4/16
eswitchctl vr route add --id 100 --prefix 0.0.0.0/0 --via 200.20.0.1 --interface p1
eswitchctl vr show-interface --id 100
eswitchctl vr route show --id 100

eswitchctl vr route del --id 100 --prefix 0.0.0.0/0
eswitchctl vr ip del --id 100 --interface p1 --address 200.20.0.4/16
eswitchctl vr port-detach --id 100 --interface p1
eswitchctl vr ip del --id 100 --interface p2 --address 192.168.0.1/24
eswitchctl vr switch-detach --id 100 --interface p2
eswitchctl vr delete --id 100
```

vSwitch 200 must already exist. An interface name is scoped to its VR; numeric
interface IDs are stable and not reused during a configuration's lifetime.
One public uplink per VR, one router attachment per vSwitch, one IPv4 address
per interface. Connected routes are derived from addresses. Static routes
require an on-link next hop. Overlapping interface subnets inside one VR are
rejected; identical subnets in different VRs are permitted.

The generated RIF MAC is a locally administered placeholder; configure the
public VF's accepted MAC before enabling a future dataplane. NAT and admin-up
commands are explicitly rejected until that backend exists.

## Persistence and ownership

Router state is stored alongside the existing L2 state as
`${ESWITCH_STATE_FILE}.router`. Files use temporary creation, file fsync,
rename, and directory fsync. Configuration is validated on a candidate and
published only after persistence succeeds. State uses host/PF/VF identity,
not runtime DPDK IDs. On startup L2 restores first; router restore then rejects
missing, out-of-scope or L2-owned public ports and missing vSwitches.

IP removal is blocked by dependent static routes; detach is blocked until IP
and route dependencies are removed; VR deletion requires no interfaces.
Deleting a vSwitch still attached to a VR is rejected. Public reservations
are included in daemon status and excluded from available-port output.

## DOCA 3.4 backend work remaining

The local sample bundle identifies itself as `3.4.0012`. The existing Dockerfile
targets DOCA `devel-3.4.0` / `full-rt-3.4.0`. SDK headers were not found in this
Mac workspace or `/opt/mellanox`, and Docker daemon was unavailable during
implementation. No DPU build or packet test has been performed.

Remaining integration must use the installed 3.4 headers and these sample
families in `doca-samples/samples/doca_flow/`:

- `flow_lpm` / `flow_lpm_em`: non-root hardware route lookup.
- `flow_ct_tcp_actions` and CT common code: directional NAT, lifetime, callbacks.
- `applications/psp_gateway` in the sample bundle: Arm ARP and reinjection.

Required work: reserved L3 gateway dispatch, typed Arm RSS reasons, public ARP,
RIF-scoped neighbors, LPM/adjacency programming, ICMP local delivery, CT/NAT
initialization and per-VR zones, route invalidation, first-packet handling,
checksum/TTL/MTU exception paths, and durable config/hardware rollback.
Do not treat this private-ARP milestone as the completed router implementation.

## Verification and VF 11–15 build handoff

Portable model tests run without DOCA. Example from this directory's parent:

```sh
clang -std=gnu11 -Wall -Wextra -Werror -fsanitize=address,undefined \
  router/router_test.c router/router.c router/router_state.c \
  -o /tmp/eswitch-router-test
/tmp/eswitch-router-test
```

In the DOCA 3.4 environment (user-run):

```sh
meson setup /tmp/eswitch-management-build application/eswitch_management -Dvf_scope=7-15
meson compile -C /tmp/eswitch-management-build
meson test -C /tmp/eswitch-management-build --print-errorlogs
```

The Meson command above runs from `Sample-DOCA-Application`. For container
builds, use that directory as context and `--build-arg VF_SCOPE=7-15`.
For runtime set `ESWITCH_VF_SCOPE=7-15` explicitly. Existing build trees retain
their old option: reconfigure before testing. Check inventory identifies only
VF indexes 11–15 plus the parent needed by the shared manager. Never infer VF
index from a DPDK port ID. Use a separate test state/socket path so unrelated
saved memberships are not replayed. No hardware test was run by this change.
