# Router implementation status

This change introduces the control-plane foundation and a private gateway ARP
responder. It does **not** implement IPv4 router forwarding or ICMP echo replies.
Interfaces with IPs report `PENDING_DATAPLANE`, and
daemon status reports `router_dataplane=NOT_IMPLEMENTED`. Public ports are
reserved but remain root-miss DROP. Private L2 forwarding remains active.
No API stub returns a fabricated hardware success.

## Private gateway ARP / TX Plan A

Plan A replaces the old metadata TX implementation. RX ARP dispatch and the
portable 60-byte ARP response builder are unchanged. DEFAULT-domain L2
forwarding is unchanged. This is a single-pair diagnostic, not multi-VR TX.

Software path:
1. Validate gateway request and current VS ownership.
2. Require exact configured test VS and VM MAC selectors.
3. Resolve the request's ingress endpoint using current inventory (never VF
   arithmetic). Commit one Ethernet ARP probe rule for gateway MAC -> VM MAC.
4. Allocate fresh mbuf, copy padded ARP reply, set no TX offload/metadata flag.
5. Send on the actual parent TX queue; free only on enqueue failure.

Hardware path (all BASIC pipes in EGRESS):
```text
TX_A_EGRESS_ROOT: match all + entry counter
  -> TX_A_ARP_PROBE: EtherType ARP + source/destination MAC + counter -> VF
       miss -> TX_A_DROP: match all + entry counter -> DROP
```

DROP and probe pipes are created before the root. The probe starts empty.
Its one tuple is locked after successful programming until daemon restart.
A changed gateway MAC or moved VM therefore requires restarting this test
daemon; do not modify topology during a measurement. Software validates
current ownership/address on every reply, including after address removal.
Other pairs never generate TX; unmatched EGRESS traffic is dropped. Do not
connect the existing DEFAULT L2 path to this diagnostic EGRESS root.

Launch in doca-dev with the production PF owner stopped:
```sh
export ESWITCH_CONTROL_SOCKET=/run/eswitch-router-test/control.sock
export ESWITCH_STATE_FILE=/var/lib/eswitch-router-test/eswitch.conf
export ESWITCH_TX_PROBE_VS=100
export ESWITCH_TX_PROBE_VM_MAC=7e:83:a5:77:11:06
./eswitch-management -l 0 -- 03:00.0
```
Use canonical decimal VS and lowercase colon-separated MAC. Unset/malformed
selectors disable replies (fail closed). Keep the established VF scope; the
selectors never change the set of probed VFs.

VM: `sudo arping -I ens6 -c 5 192.168.0.1`, alongside
`sudo tcpdump -eni ens6 -nn arp`. Capture a baseline and another
`eswitchctl status` after waiting at least two seconds for counter sampling.

- `egress_enter`: packet reached the root, independent of metadata/header.
- `arp_probe_hit`: exact Ethernet tuple selected the VF.
- `tx_probe_drop`: packet missed the probe.
- `unavailable` is not zero; probe is unarmed until the first selected request.
- Flow entry counters are sampled by maintenance once per aging interval.
  `status` and `tx-debug` read that cache; neither performs Flow miss queries
  or steering dumps. Parent ethdev stats remain read-only on demand.
- Entry-query behavior still needs hardware validation. Removing miss-query
  avoids the newly suspected call but does not prove the earlier crash cause.
- Packet logs are sampled; enqueue and parent statistics are not delivery proof.

Acceptance: selected VM gets the RIF MAC in an ARP reply; root and probe
counters increase, drop stays flat in an otherwise quiet test. Then verify
ordinary known-unicast and broadcast between the two L2 test VMs, reject other
VS/VM pairs, remove gateway IP and verify replies stop. ICMP replies and router
IPv4 forwarding remain outside this milestone.

If root stays flat despite parent TX increases, inspect software-TX/root
binding rather than metadata. If root increases and drop increases, inspect
Ethernet matching. If probe increases without guest capture, investigate the
VF/host/VM path. Hardware counters may include other traffic: compare deltas.

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
RIF-scoped neighbors, LPM/adjacency programming, trusted control TX, CT/NAT
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
