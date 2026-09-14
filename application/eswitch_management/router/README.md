# Router implementation status

This implementation provides private gateway ARP, local IPv4 ICMP echo,
IPv4 routing between vSwitch RIFs, and stateful TCP/UDP plus ICMP Echo NAT
through one public port per VR. Longest-prefix lookup, neighbor handling and NAT sessions currently
run on Arm; DOCA Flow performs ingress classification and directed SF return.
Private addressed RIFs report `ACTIVE_ARM_LPM`. An addressed public RIF reports
`ACTIVE_ARM_UPLINK`, or `ACTIVE_ARM_NAT` while its NAT policy is enabled. No API
stub reports fabricated hardware-CT success.

## Private gateway ARP through the Arm system SF

The old parent-PF TX probe has been replaced by the same topology proven by the
SF-to-VF10 PoC. RX ARP dispatch and the portable 60-byte ARP response builder
are unchanged. The return path is multi-VS capable and fail-closed.

Software path:
1. Validate gateway request and current VS ownership.
2. Learn the requesting VM source MAC in the normal VS FDB.
3. Bind a directed private context VLAN to that VS, target port and virtual
   RIF MAC in `ESW_SF_RETURN`.
4. Build a padded 60-byte Ethernet ARP reply.
5. Replace the wire source with the actual SF MAC, insert the private context
   VLAN, and send through an `AF_PACKET/SOCK_RAW` socket bound to the actual
   Arm SF endpoint (`ESWITCH_SF_IFACE`, default `enp3s0f0s0`).

Hardware path (DEFAULT domain):
```text
actual Arm SF -> system-SF representor
  -> ESW_INGRESS_CLASSIFIER: SF port -> ESW_SF_RETURN
     -> known directed context VLAN: pop VLAN, rewrite source to virtual RIF MAC,
        set pkt_meta=(VS << 16) | SF_port
        -> ESW_EGRESS_GATE_<VF> -> VF -> VM
     -> known flood context VLAN
        -> ESW_DEST_FDB miss -> per-VS flood selector
     -> unknown context VLAN: DROP
```

The private VLAN never leaves the SF return pipe. The SF root entry and
SF-return miss both fail closed, and the SF cannot be attached as a tenant
port. One flood tag per RIF preserves local-IP delivery and broadcast ARP
probes; directed tags are reused per `(VS, target port)`. When the configured
RIF MAC changes, the next gateway packet removes every old context for that VS
and binds the new RIF MAC without a daemon restart.

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

VM: `sudo arping -I ens6 -c 5 192.168.0.1`, followed by
`ping -c 5 192.168.0.1`, alongside
`sudo tcpdump -eni ens6 -nn 'arp or icmp'`. Acceptance: the VM receives ARP and
ICMP echo replies whose
source MAC is the configured RIF MAC; `status` increments `arp_sf_tx_sent` and
`icmp_sf_tx_sent`, and shows increasing `local_ip_hits`. Then verify ordinary L2 forwarding,
reject unknown SF source MACs, remove the gateway IP and verify replies stop.
A successful `sendto()` is not delivery proof; the VM capture is authoritative.

For `VM ping gateway`, `ESW_LOCAL_IP` matches `(VS metadata, RIF destination
MAC, IPv4)` and sends the packet to the Arm RSS queue. The Arm handler validates
the IPv4 and ICMP checksums, rejects fragments and non-echo traffic, builds the
echo reply, and reuses `ESW_SF_RETURN` for delivery to the VM.

## IPv4 routing between private vSwitches

An IPv4 packet addressed to the ingress RIF MAC is delivered to Arm by
`ESW_LOCAL_IP`. The router selects the ingress VR from the vSwitch attachment,
performs longest-prefix matching across connected and static routes in that VR,
and chooses the route's egress RIF. Connected routes use the destination IP as
the next hop; static routes use their configured gateway.

The router learns on-link neighbors from validated ARP requests and replies. On
a neighbor miss it sends a rate-limited ARP request through the egress RIF and
drops the current packet. A retry is forwarded after resolution.
Forwarding rewrites destination/source Ethernet addresses, decrements IPv4 TTL,
recomputes the IPv4 header checksum, and transmits through a directed SF
context for the neighbor's learned port. Route miss, TTL expiry, IPv4
fragments, ICMP messages other than Echo, unsupported protocols and invalid
IPv4 fail closed; ICMP error generation is a later milestone.

```text
VM-A -> VS-A -> ingress RIF -> Arm LPM -> egress RIF
     -> directed SF context -> VM-B egress gate -> VM-B
```

This is a correctness milestone, not hardware LPM: `status` explicitly reports
`ipv4_routing=arm-lpm`. Moving the same route/adjacency model into per-VR DOCA
Flow LPM and exact-neighbor pipes is the next acceleration step.

### Two-VM acceptance test

Use the actual DPDK port IDs reported for the two VF representors. The example
creates VS 100 for VM-A and VS 200 for VM-B, then attaches both networks to VR
101. Connected `/24` routes are derived from the two interface addresses; no
static route command is needed.

```sh
eswitchctl vs-create --id 100
eswitchctl vs-port-attach --id 100 --port <VM-A-VF-DPDK-port>
eswitchctl vs-create --id 200
eswitchctl vs-port-attach --id 200 --port <VM-B-VF-DPDK-port>

eswitchctl vr create --id 101
eswitchctl vr switch-attach --id 101 --switch-id 100 --name lan-a
eswitchctl vr switch-attach --id 101 --switch-id 200 --name lan-b
eswitchctl vr interface set --id 101 --interface lan-a --mac 02:00:00:65:00:01
eswitchctl vr interface set --id 101 --interface lan-b --mac 02:00:00:65:00:02
eswitchctl vr ip add --id 101 --interface lan-a --address 192.168.10.1/24
eswitchctl vr ip add --id 101 --interface lan-b --address 192.168.20.1/24
eswitchctl vr route show --id 101
```

Configure VM-A as `192.168.10.10/24` with gateway `192.168.10.1`, and VM-B as
`192.168.20.10/24` with gateway `192.168.20.1`. First confirm each VM can ping
its own gateway, then run `ping 192.168.20.10` from VM-A and
`ping 192.168.10.10` from VM-B. The first routed echo may be lost while the
router resolves the destination neighbor. Acceptance requires bidirectional
ping, decremented TTL at the destination, `routed_forwarded` increasing in
`eswitchctl status`, and no cross-VR forwarding when either vSwitch is attached
to a different VR.

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
    router_icmp.c             validated local ICMP echo reply builder
    router_forward.c          per-VR IPv4 LPM and forwarding rewrite
    router_neighbor.c         per-VR/RIF ARP neighbor cache and probe control
    router_nat.c              TCP/UDP/ICMP Echo NAT sessions and checksums
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
public VF's accepted MAC before enabling its dataplane.

The NAT control plane accepts one TCP/UDP/ICMP Echo NAT policy per VR once an
addressed public port owns the default route:

```sh
eswitchctl vr nat enable --id 100 --interface p1 \
  --address interface --port-range 20000-60999
eswitchctl vr nat show --id 100
eswitchctl vr nat disable --id 100
```

The policy is persisted, validated transactionally, and blocks removal of its
uplink address/default route. `router_nat.c` implements a bounded 4096-entry
session table, collision-free translated endpoint allocation within the
configured range, TCP/UDP checksum repair, ICMP Echo Identifier translation,
reverse translation, and protocol-specific idle aging. The public
representor is classified directly to the Arm RSS path with `(VR, port)`
metadata; it is not allowed into tenant L2 learning. Public ARP is RIF-scoped.

```text
VM -> private VS/RIF -> Arm LPM -> TCP/UDP SNAT/PAT or ICMP Echo ID NAT
   -> public RIF/next-hop rewrite -> directed SF context -> uplink representor

uplink representor -> Arm reverse-session lookup -> DNAT
   -> original private RIF/VM identity -> directed SF context -> VM
```

The first outbound packet may be dropped while the public next-hop ARP entry is
resolved; retrying the connection should then create a NAT session. Reverse
traffic is accepted only when its VR, protocol, public tuple and remote tuple
match an active session, and is returned only to the private VS/port recorded by
that session. ICMP Echo Request (`type 8/code 0`) maps the original Identifier
to an Identifier from the configured port range; the corresponding Echo Reply
(`type 0/code 0`) restores it. IPv4 fragments, other ICMP message types, and
protocols other than TCP/UDP/ICMP fail closed. Hairpin NAT, static
DNAT/port-forwarding, ICMP error-message translation and hardware CT are not
implemented yet.

Example acceptance test, using a reachable target and TCP service beyond the
uplink:

```sh
# DPU
eswitchctl vr nat show --id 100
eswitchctl status | grep -E 'nat_dataplane|nat_sessions|nat_out|nat_in'

# private VM; the first attempt may only trigger next-hop ARP
ping -c 5 <remote-ip>
nc -vz -w 2 <remote-ip> <tcp-port>
nc -vz -w 2 <remote-ip> <tcp-port>

# DPU/public endpoint captures
tcpdump -eni <public-endpoint> -nn 'icmp or tcp or udp or arp'
```

Acceptance requires the public capture to show source IP equal to the public
RIF address and a source port or ICMP Echo Identifier in the configured range;
reply traffic must reach the originating VM. `nat_sessions`, `nat_out`,
`nat_in`, `nat_icmp_echo_out`, `nat_icmp_echo_in`,
`routed_forwarded`, and the target egress-gate counter should increase. A
successful SF `sendto()` alone is not delivery proof.

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

The local sample bundle identifies itself as `3.4.0012`, while the target DPU
reported `3.4.0112`. The existing Dockerfile targets DOCA `devel-3.4.0` /
`full-rt-3.4.0`. A header snapshot exists under `.deps`, but it is not a complete
Linux SDK/sysroot; the local Docker daemon was also unavailable. Therefore the
DOCA-linked binary and packet path still require build and acceptance testing
in the DPU's `doca-devel` container.

Hardware acceleration must use the installed 3.4 headers and these sample
families in `doca-samples/samples/doca_flow/`:

- `flow_lpm` / `flow_lpm_em`: non-root hardware route lookup.
- `flow_ct_tcp_actions` and CT common code: directional NAT, lifetime, callbacks.
- `applications/psp_gateway` in the sample bundle: Arm ARP and reinjection.

Remaining work: DOCA Flow CT initialization and per-VR zones, hardware
LPM/adjacency programming, CT/session synchronization, route invalidation,
first-packet promotion, ICMP error-message NAT, checksum/TTL/MTU exception
generation, and durable config/hardware rollback. The daemon probes
`doca_flow_ct_cap_is_dev_supported()` and reports the result, but deliberately
keeps `hw_ct_state=NOT_INITIALIZED` until the CT lifecycle and both directions
can be installed atomically.

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
