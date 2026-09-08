# Router implementation status

This change introduces the control-plane foundation and a private gateway ARP
responder. It does **not** implement IPv4 router forwarding or ICMP echo replies.
Interfaces with IPs report `PENDING_DATAPLANE`, and
daemon status reports `router_dataplane=NOT_IMPLEMENTED`. Public ports are
reserved but remain root-miss DROP. Private L2 forwarding remains active.
No API stub returns a fabricated hardware success.

## Private gateway ARP milestone

The root classifier now forwards to `ESW_ARP_DISPATCH`. Untagged ARP goes
through the existing clone path: exactly one Arm copy plus the normal private
L2 path. Non-ARP still uses the source guard. This deliberately retains L2
broadcast behavior, including gateway requests, rather than introducing trap
and L2 reinjection in this milestone. A known source continues generating ARP
copies; learning does not disable the responder. Source copies are never TXed.

The Arm handler validates Ethernet/IPv4 ARP request fields and source MAC
consistency, selects the RIF by ingress vSwitch and exact target IP, and builds
a 60-byte padded response. Each reply uses a fresh mbuf. Management now opts
into `switch,hws,expert` (the standalone ethernet_switch app keeps its default).
Parent TX queue 0 injects the reply with host-order metadata
`(target_dpdk_port_id << 16) | 0xffff`. `ESW_CONTROL_TX`, an EGRESS root control
pipe, matches ARP EtherType and the exact metadata, then forwards to that
probed VF using `FWD_PORT`. Flow metadata matches use big-endian values.
This bypasses the DEFAULT-domain ingress classifier and L2 split horizon.
There are no TX rules targeting the parent; current VS ownership and RIF
configuration are checked by the Arm handler before generating a reply.
The low 16 bits of RX metadata are a valid ingress port, never `0xffff`, so
this TX namespace cannot collide with L2 metadata for any vSwitch ID. The
forwarding rule no longer depends on the software-origin parser sentinel.
Invalid tagged TX hits `control_tx_invalid`; a separate lower-priority
software-origin DROP rule counts `control_tx_sw_untagged`. Zero on that
secondary rule alone is not proof that no software TX entered EGRESS.
Unmatched hardware traffic retains the
EGRESS domain's default forwarding behavior. All TX rules must commit before
the daemon serves commands. A successful TX enqueue transfers mbuf ownership;
failed TX frees it without blocking/retrying in the manager loop.
Replies are bounded to 100 attempts/second globally; status exposes reply,
TX-drop and rate-drop counters. This bounds reply work, not incoming ARP RSS
load. Hardware policing is not part of this milestone.

Only addressed `vs-link` interfaces respond. Public port-link ARP, VLAN-tagged
ARP, local ICMP and IPv4 forwarding remain unsupported. Configured gateway
MACs are excluded from learning on their vSwitch; this is not full hardware
anti-spoof enforcement. No address/IP config migration is needed.

After rebuilding/restarting the single test daemon with the existing test
state, run from the VM attached to switch 100:

```sh
arping -I <vm-interface> -c 3 192.168.0.1
```

Expected for VR 101: replies advertise `02:00:00:65:00:01`. Repeat after the
source is learned; replies must continue. Daemon logs `ARP TX ENQUEUED: vs=100 ...`
and `eswitchctl status` increments `arp_tx_enqueued`. Status also exposes
`control_tx port=<DPDK-ID> hw_packets=N` and `control_tx_invalid hw_packets=N`.
For the current port-2 VM, the port-2 counter must increase and invalid must
remain zero. Counter query failures are shown as errors, never as zero packets.
Capture on the VM to confirm delivery; neither TX acceptance nor a forwarding
rule hit alone proves receipt by the guest:

```sh
sudo tcpdump -eni ens6 -nn arp
sudo arping -I ens6 -c 3 192.168.0.1
ip neigh show dev ens6
```

If enqueued increases but no TX hardware counter changes, check parent TX and
metadata/EGRESS entry installation. If invalid increases, inspect the injection
tag, protocol and port mapping. If the correct forwarding counter increases
without guest replies, investigate the VF/host/VM receive path next.

### TX debug output

#### Root-boundary investigation (debug version 2)

`status` and the periodic snapshot now query `control_tx_root_miss` directly
from the EGRESS pipe. The miss counter is enabled before pipe creation; a
failure to configure it fails startup, and a read failure is printed as an
error rather than zero. No catch-all forwarding entry is introduced and the
existing miss destination is unchanged.

In the test container, after rebuilding and restarting with the same test
socket/state, collect a baseline and then run guest arping:

```sh
export ESWITCH_CONTROL_SOCKET=/run/eswitch-router-test/control.sock
./eswitchctl status
# On the VM: sudo arping -I ens6 -c 5 192.168.0.1
# Wait for counter refresh, then:
./eswitchctl status
./eswitchctl tx-debug
```

`tx-debug` reports the runtime DPDK version, parent driver, switch domain/port,
queue counts, counters, and a driver steering dump. The dump is saved under
`/tmp/eswitch-tx-steering-XXXXXX` **inside the daemon's container**, using a
unique mode-0600 file. Copy the exact returned file for analysis. This is an
on-demand read-only diagnostic, but it runs on the control thread and may
temporarily pause packet polling; do not run it repeatedly under load. Dumps
may contain tenant addresses. Files are not automatically deleted. PMD dump
support/coverage varies: `FAILED` or an empty dump is not evidence of an empty
DOCA pipeline. This does not claim to replace DOCA Flow Tune.

Interpret deltas, not a single cumulative snapshot:

| Observation during isolated ARP test | Next boundary to investigate |
| --- | --- |
| Parent TX and root miss increase, all entries stay flat | EGRESS is seeing unmatched traffic; inspect metadata transport and programmed matches |
| Parent TX increases, all entry and root miss counters stay flat | Check software TX-to-root binding, driver steering and counter observability; this alone does not prove the exact cause |
| Correct VF rule increases but no guest reply | Investigate forwarding after the rule: VF/host/VM path |
| Any counter query fails | Resolve observability first; do not interpret the failure as zero |

The miss counter can include other EGRESS traffic; correlation with the five
test packets is necessary. Root cause remains unconfirmed until the DPU
measurements and steering dump identify the failing boundary. Existing logs
confirm valid reply bytes and parent TX accounting, not guest delivery.

No extra flag is required for the current test build:

- `TX RULE READY`: committed EGRESS rule, exact metadata and host/PF/VF mapping.
- `ARP TX BUILD`: parent/queue/target, packet length, segment count, metadata and
  mbuf flags. `ARP TX FRAME` dumps the complete 60-byte generated ARP frame.
  Detailed packet logs cover the first three ARP requests, then at most once
  per second globally. They include MAC/IP addresses; handle logs accordingly.
- `ARP TX SKIP`: truncated frame or request rejected by gateway/ARP validation.
- `ARP TX DROP`: target validation, rate limit, allocation, append or enqueue
  failure. Status keeps exact cumulative stage counts even when logs suppress
  repeated events. `arp_built` means response bytes were constructed, not sent.
- `TX DEBUG SNAPSHOT`: cumulative status at most every five seconds when ARP
  activity changes, including a trailing snapshot after traffic stops.
- `parent_tx`: DPDK ethdev `opackets`, `obytes`, `oerrors`, or a query error.
  These may include other traffic and depend on PMD counter support; neither
  ethdev counters nor Flow hit counters prove guest receipt. Compare two
  snapshots and confirm with guest tcpdump. No TX statistics are reset.

For target DPDK port 2 the new host-order TX metadata must be `0x0002ffff`.
The reply Ethernet destination must be the requester's MAC, source the private
RIF MAC, EtherType `0806`, and ARP opcode `0002`. Successful enqueue transfers
ownership; the code never reads or frees the mbuf afterward. No retries or
extra diagnostic packets are injected. Restart the daemon after rebuilding;
old and new TX metadata formats must not be mixed.
`ping` can populate the neighbor cache but will not receive ICMP Echo Reply yet.
Also test ordinary ARP between two VMs to verify L2 behavior remains intact,
and remove the gateway IP to confirm the responder stops. Also test another
test VF and a different VS to verify replies do not leak. Hardware acceptance
requires testing on the DPU: portable ARP/router tests do not exercise these
Flow rules. Keep the production daemon stopped while the test owns the PF;
different sockets do not isolate eSwitch ownership.

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
meson setup /tmp/eswitch-management-build application/eswitch_management -Dvf_scope=11-15
meson compile -C /tmp/eswitch-management-build
meson test -C /tmp/eswitch-management-build --print-errorlogs
```

The Meson command above runs from `Sample-DOCA-Application`. For container
builds, use that directory as context and `--build-arg VF_SCOPE=11-15`.
For runtime set `ESWITCH_VF_SCOPE=11-15` explicitly. Existing build trees retain
their old option: reconfigure before testing. Check inventory identifies only
VF indexes 11–15 plus the parent needed by the shared manager. Never infer VF
index from a DPDK port ID. Use a separate test state/socket path so unrelated
saved memberships are not replayed. No hardware test was run by this change.
