# Authorized NAT CT fast path (v42)

## Scope and safety

Use an isolated BF3 test VF/VS and test destination, not production traffic.
Preserve `eswitch.conf` and `eswitch.conf.router` and record startup logs/status.
This change does not alter persisted CLI syntax or require recreating topology.
Enable `ESWITCH_HW_CT=1` before starting the daemon. Hardware routing may remain
disabled (`ESWITCH_HW_ROUTING=0`). Both guest and public interfaces must be
`type=vs-link`; a dedicated `port-link` WAN remains Arm-only in this phase.

Build and run the existing Meson test suite on BF3:

```bash
meson compile -C /build/eswitch-management
meson test -C /build/eswitch-management --print-errorlogs
ldd /build/eswitch-management/eswitch-management | grep libdoca_flow
pkg-config --modversion doca-flow
```

Confirm the binary/library/firmware pairing before diagnosing CT errors.
Start with CT capacity 64 for the isolated smoke, then increase only after the
single-connection test passes. Admission reserves two control entries per
connection. Capacity is not a promise that the device has enough resources.

## Expected pipeline

```text
VS ingress -> ARP dispatch -> exact authorized session gate
  hit: valid nonfragment IPv4, TTL > 1, exact VS/port/MAC/5-tuple
       -> connection-private CT zone -> NAT -> adjacency/TTL/VLAN -> egress
  miss: local delivery -> guest ACL -> Arm policy/route/neighbor/NAT
       -> forward packet -> commit CT -> commit bidirectional admission
```

ARP, ICMP, fragments, options, malformed IPv4 and TTL exceptions retain Arm
handling. Policy admission still occurs on Arm even when the ACL policy is in
hardware fallback. A PF session is not promoted on the incoming SYN alone:
its first correctly routed guest reply must pass the established-session check.

## One SNAT connection

1. Record `eswitchctl status` before traffic. Require revision v42,
   `hw_ct_state=ready` and `ct_authorization=exact-ingress-session`.
2. Use a known reachable TCP/UDP service, rather than assuming the upstream
   gateway has an open TCP port. Transfer a large file over one TCP connection.
3. During the first 20 seconds, compare `hw_ct_active`, `hw_ct_promotions`,
   `hw_ct_failures`, `guest_egress checked`, `nat_out` and `nat_in` deltas.
   Active/promotions must rise without failure. Arm packet counters should grow
   much more slowly than the transfer, but these are not hardware hit counters.
4. Capture at guest and upstream: verify IP/port NAT, MACs, VLAN 6 on trunk,
   no leaked context VLAN and a single TTL decrement. Verify checksums.
5. With CT disabled, repeat the same transfer, VM count, payload, CPU affinity
   and topology. Compare throughput, Arm CPU, latency and loss; do not infer
   a speedup from successful entry installation alone.

## Egress policy and revocation

Keep management access outside the tested guest policy.

1. Under default deny, allow a test TCP destination/port. Verify its connection
   can promote after Arm authorization even if the ACL is Arm fallback.
2. Try a denied port with no existing session: it must never promote/forward.
3. While the allowed transfer is active, delete its allow rule or insert a
   higher-priority deny. The command must revoke admission and CT before commit.
   New traffic must be denied; already transmitted/in-flight packets are not
   recalled. A deletion failure must return an error, not commit the new policy.
4. Restore allow and start a new connection. Check promotion resumes.
5. Change a route, RIF MAC, NAT address or VS membership. Old CT entries must
   disappear before the mutation takes effect. Repeat after neighbor MAC changes.

## Port forwarding

With a PF rule to a listening SSH/HTTP service and guest default deny, connect
from a test external client. Verify the guest has a correct return route.
The first guest reply must be accepted only as the exact existing PF session;
later packets may use CT. Verify both SNAT reply and DNAT incoming tuples.
An unsolicited guest flow from the same private service port to another remote
tuple must still be denied. Delete the PF rule while active: old CT/session
admission must be revoked. Test TCP and UDP separately.

## Isolation, exceptions and resource pressure

- Repeat identical private tuples on different VRs/RIFs: no cross-connection hit.
- Change source MAC/ingress VF while retaining IP/ports: no admission bypass.
- Send TTL 1, IPv4 options and fragments of an authorized tuple: verify Arm
  semantics, not hardware decrement/forward bypass.
- Transfer for over 60 seconds. CT has a 30-second batch lease (maintenance scan
  granularity applies); revocation must leave the software NAT session alive.
  It is reauthorized/re-promoted by subsequent Arm traffic. This is a temporary
  bounded-lifetime strategy, not hardware activity-based aging.
- Exhaust a small CT capacity: extra flows must continue on Arm without losing
  NAT ownership or bypassing policy. Test admission-add/removal failures before
  claiming production readiness.

## Rollback

Stop the daemon cleanly, set `ESWITCH_HW_CT=0`, and restart with the same
persistent configuration. Traffic returns to Arm without CLI changes. Do not
free software NAT owners manually while hardware entries remain referenced.

## Remaining performance work

Hardware activity counters/aging, per-session rather than batch lease eviction,
asynchronous installation, selective mutation invalidation and dedicated
port-link WAN admission remain future work. This phase does not offload ordinary
non-NAT routing behind an egress policy or ICMP NAT.

## v42 diagnostics and benchmark

PF reply exceptions now use an exact CONTROL pipe with ordinary bit masks and
an explicit miss to the guest policy ACL. Entries include ingress VS/port and
source MAC. The user-facing `pf_reply_exceptions`/`pf_reply_fallbacks` fields
retain their names. This programming change still requires BF3 validation.

CT resource failures emit a stage-specific log with packet debug disabled.
`ct_retry_backoff_ms`, `ct_retry_suppressed`, `ct_no_memory`,
`ct_last_failure_stage` and `ct_last_error` distinguish real hardware attempts
from packets staying on Arm during the 250 ms–8 second global promotion
backoff. `hw_ct_full` counts FULL only, no longer NO_MEMORY. Admission zone
writes are now included in the parent action-memory budget.

After single-flow correctness passes, follow [performance experiments](PERFORMANCE_TESTING.md)
for repeated SCP downloads and multi-VM load testing. Do not benchmark an
offload-failed run as a successful hardware fast path.
