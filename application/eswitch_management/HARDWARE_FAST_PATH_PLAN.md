# Hardware fast path implementation and review plan

Implementation is performed directly in this repository. Preserve the working
software dataplane and existing configuration/control compatibility.

## Current implementation status

- Implemented behind `ESWITCH_HW_ROUTING=1`: private RIF selector, local-IP
  separation, IPv4 eligibility/exception control, VR-keyed DOCA Flow LPM,
  resolved adjacency rewrite, TTL decrement and direct egress-gate forwarding.
- Implemented: incremental add/update/remove, neighbor aging, VF detach and
  RIF/config invalidation, counters/status, and portable route-plan tests.
- Implemented: successful hot-path packet logging is disabled by default and
  can be enabled temporarily with `ESWITCH_PACKET_DEBUG=1`.
- Kept on Arm intentionally: unresolved/missing routes, local router traffic,
  TTL <= 1, options/fragments/invalid IPv4, public uplinks and all NAT.
- Deferred: DOCA Flow CT/NAT promotion and multi-worker software slow path.
- Required before production enablement: BF3 hardware pipe-create, traffic,
  exception, mutation and performance smoke tests.

## 1. Hardware routing

Add opt-in DOCA Flow accelerated IPv4 routing, default disabled until BF3 smoke tests pass. Separate ingress identity and local destination/exception classification from transit routing. Use verified DOCA 3.4 LPM and header modification APIs, following the local shipped samples. Keep VR isolation and ingress authorization explicit. Route selection must preserve longest-prefix and connected/static/default semantics of router_forward.c. Separate route selection from resolved neighbor MAC/port adjacency; connected subnets require destination-IP adjacency while gateway routes resolve the next-hop IP.

Punt local IPs, TTL <= 1, options/fragments, unsupported traffic, missing routes/neighbors and hardware failures to the existing software path. Never bypass required NAT with a plain routing fast path. Route/neighbor promotion may initially target eligible private-to-private IPv4 only; expose the supported scope honestly. Hardware rewrite decrements TTL once, repairs checksum, writes both MACs and forwards to the resolved destination. Keep SF return for software-generated packets.

## 2. Ownership and invalidation

Stable entry cookies must survive asynchronous commits and removals. Do not free/reuse state before removal completion. Invalidate before route, RIF/MAC, port ownership, neighbor, NAT policy or VS changes become effective; include failure rollback. Bound memory and counters. Hardware aging must not expire active flows based solely on software packet timestamps. Do not turn normal hardware table exhaustion into service shutdown. Avoid per-packet hardware queries or full table rebuilds.

## 3. Hardware TCP/UDP NAT

Implement only against actual doca_flow_ct.h and sample signatures. Initialize CT after Flow init and before port start, capability-gated and opt-in. Software policy/port allocation remains authoritative. Promote original/reply tuples together after neighbor/route resolution. Include VR/zone identity. Pending rules retain software fallback. Exclude unsupported fragments/options, exceptional TCP packets and ICMP NAT initially. Ensure checksum/TTL correctness, session counters/aging and teardown/invalidation cannot leak stale NAT mappings. If this cannot be fully verified with available dependencies, leave CT disabled and document the exact remaining gap; do not claim an implemented feature from scaffolding alone.

## 4. Observability and scale

Expose configured mode, active hardware entries, promotions, misses/fallbacks, add/remove failures and scope in status/tx-debug. Avoid hot-path printf for successful packets by default; allow explicit diagnostic logging. Keep software single-owner until deliberate worker partitioning is implemented. Do not add unproven concurrency. Account for new counters separately from the shared ethernet_switch application.

## 5. Verification and delivery

Run existing portable router, ARP, ICMP, forwarding, NAT tests under warnings-as-errors and ASan/UBSan. Add meaningful tests for eligibility, route overlap, VR isolation, neighbor change, invalidation and commit failure wherever portable. Verify DOCA-dependent code via Linux full build when available; otherwise syntax-check against local headers only with limitations clearly recorded. Do not fabricate SDK stubs to claim a full build.

Record reproducible BF3 build and smoke commands: baseline off; enable hardware; two-VS ping and TCP/UDP; NAT curl and ICMP software fallback; route/MAC/neighbor change; disable/re-enable; hardware counters increase while Arm routed counters stop increasing for eligible steady traffic. Benchmark on isolated endpoints only. No deployment, firmware mutation, git commit/push or external messaging in this task.

## Execution constraints

Exact skill paths (they are outside .kiro):
- /Users/ppanpru/.agents/skills/doca-common/SKILL.md
- /Users/ppanpru/.agents/skills/doca-flow/SKILL.md
- /Users/ppanpru/.agents/skills/doca-bf3-deployment/SKILL.md

Read applicable AGENTS.md and the named doca-common, doca-flow and doca-bf3-deployment skills (and relevant companions). Header snapshot: .deps/doca-3.4.0/include; DPDK .deps/dpdk/include; sample tree ../doca-samples/samples/doca_flow. Use apply_patch for edits. Existing PERFORMANCE.md is an untracked prior deliverable: preserve and update accurately. Work in this repository only; temporary test binaries may live in /tmp. No blanket trust of external MCP tools. Report concrete files changed, tests run, unsupported scope and unresolved risks for GPT review.
