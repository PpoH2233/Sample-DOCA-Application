# Performance rollout

## Implemented: indexed software NAT

Established outbound, reverse and public-port ownership lookups use three
8192-bucket intrusive hash indices instead of scanning 4096 sessions.
Full tuple comparisons preserve collision correctness and VR isolation.
Session creation still scans for a free slot; maintenance still scans for
expiry. Worst-case bucket traversal remains linear, not a constant-time
guarantee. Capacity is unchanged at 4096 sessions. Additional fixed memory
is approximately 72 KiB before alignment (bucket heads and per-slot links).
The table remains single-owner; do not share it between workers.

Expiry and flush unlink all three indices before slot reuse. Packet parsing,
checksum validation, protocol support and translation semantics are unchanged.
The portable NAT regression now fills the table, verifies both directions
for every session, flushes and repeats, in addition to existing tests.

SF context TX now sends a 16-byte Ethernet/VLAN prefix and the original
EtherType/payload using synchronous sendmsg scatter/gather. This removes one
malloc/free and one payload copy per SF return. It is NOT zero-copy or
batched TX; socket acceptance is still not guest receipt. The VLAN tag,
actual SF source and hardware RIF source rewrite remain unchanged.

The RX error path frees all remaining packets from an already received burst.

## Implemented: opt-in private IPv4 hardware LPM

`ESWITCH_HW_ROUTING=1` installs a DOCA Flow 3.4 LPM fast path for resolved
private vSwitch adjacencies. VR identity is an exact metadata key; IPv4
destination is the LPM key. Hardware rewrites both Ethernet addresses,
decrements TTL and forwards through the resolved egress gate. Connected
neighbors are promoted as /32 entries; private static routes are promoted only
when their gateway is resolved. Neighbor MAC changes use in-place entry
updates, and route/RIF/port/neighbor changes reconcile incrementally.
The HWS action-memory reservation now adds the LPM requirement to the existing
64 KiB L2/SF reservation and scales that addition with
`ESWITCH_HW_ROUTE_CAPACITY`.
The LPM constructor retries smaller power-of-two capacities on resource
exhaustion and falls back to the existing Arm dataplane if hardware routing
cannot be admitted, so this optional optimization no longer prevents startup.
The VR exact-match dimension uses `meta.u32[1]`, the metadata field supported
for combined EM+LPM matching by BF3/DOCA Flow; `pkt_meta` remains dedicated to
the existing vSwitch and ingress-port identity.
Hardware-routing mode reserves at least 512 KiB of per-port actions memory.
The budget includes both the requested LPM capacity and two action-bearing
entries per possible SF-return context, so LPM templates do not starve the
dynamic return rewrite and eligibility entries created after traffic resolves
a NAT session or neighbor. A private neighbor is promoted only after a required
gateway ARP reply has successfully entered the SF return path; a failed bind is
left on the Arm slow path and retried by the next request instead of consuming
another LPM action first.
Hardware eligibility is committed after the per-RIF selector. If its HWS
control entry cannot be allocated, the selector remains installed and its
fallback sends router traffic to RSS/Arm; SF return, gateway ARP, local ICMP and
software routing remain available while later synchronization retries the
eligibility entry.

Local router IPs, TTL <= 1, options, fragments, invalid IPv4/checksum state,
LPM misses, unresolved neighbors, public port-links and NAT stay on Arm. The
feature defaults off. Hardware CT is still not initialized. No throughput
claim is made until the BF3 smoke and benchmark are complete.

Successful per-packet slow-path traces and the periodic SF TX snapshot are
disabled by default. Set `ESWITCH_PACKET_DEBUG=1` only for diagnosis; errors,
status counters and explicit `tx-debug` queries remain available when it is
off. This avoids formatting and terminal I/O in the steady-state manager loop.

## Not yet implemented / no production-performance claim

L2 hardware steering is retained. Unsupported routing and NAT execute on Arm.
Hardware CT promotion, multiple workers, batched TX, route/neighbor indices,
neighbor-miss buffering and control-plane isolation remain work items. A
successful capability probe is not an initialized CT dataplane.

Hardware promotion must precede port start with verified CT initialization,
then stage bidirectional entries only after route, neighbor and policy
resolution. Misses and unsupported packets must keep the software path.
Policy/route/RIF/neighbor changes must invalidate hardware entries before
software session/port reuse. Aging must account for hardware traffic, not
only software last_seen. Validate TTL, fragments, checksums, TCP lifecycle
and ICMP separately before enabling any protocol's promotion.

## Validation

Portable regression:

```sh
cc -std=c11 -Wall -Wextra -Werror -fsanitize=address,undefined \
  application/eswitch_management/router/router_nat.c \
  application/eswitch_management/router/router_nat_test.c \
  -o /tmp/eswitch-nat-index-test
/tmp/eswitch-nat-index-test
```

On the DOCA 3.4 Arm build environment:

```sh
pkg-config --modversion doca-common doca-flow
ninja -C /build/eswitch-management
meson test -C /build/eswitch-management --print-errorlogs
ldd /build/eswitch-management/eswitch-management
```

Check the actual executable name if the build uses a different name.
Verify libdoca_flow linkage, then test two VSs: gateway ARP/ICMP before and
after inter-VS traffic, bidirectional routed TCP/UDP/ICMP, outbound NAT and
reverse NAT, policy flush, session expiry and interface/MAC changes.
Capture SF and guest frames to verify VLAN removal and source rewrite.

Use isolated lab peers, not a public gateway, for load tests. Compare the
same build flags/configuration before and after this change at 1, 256,
1024 and 4096 concurrent sessions. Record pps, throughput, loss, p50/p99
latency, Arm CPU, session creation rate and hardware counter deltas.
Separate steady-state traffic from connection churn and cold ARP.
The full Linux/aarch64 DOCA 3.4.0112 build and portable tests have been run in
the NVIDIA development container. No BF3 pipe-create smoke or throughput
measurement has been performed from this macOS development machine.
