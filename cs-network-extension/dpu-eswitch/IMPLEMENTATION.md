# CloudStack implementation and rollout handoff

Implemented 2026-10-01 according to [BLUEFIELD_CLOUDSTACK_PLAN.md](BLUEFIELD_CLOUDSTACK_PLAN.md).
`dpu-eswitch-wrapper.sh` invokes adjacent stdlib-only `dpu_eswitch.py`.
The daemon gains persistent optional VS names and `port show --all` inventory;
no DOCA APIs or dataplane pipelines were changed.

## Baseline

- Git base `b47e854a089bc75317df2604b5e017d1dc22affe`.
- DOCA common/flow3.4.0112 on Arm and in doca-devel.
- Production service `eswitch-management-container.service` uses image
  `eswitch-management:3.4.0`, VF scope0–50; v51 runtime.
- LPM ready, admitted capacity64; CT ready, admitted capacity2048,
  `ct_authorization=exact-ingress-session`, no active CT connections.
- VS100 owns VF0, VS99 owns VF1/VF2, VS212 owns VF5–7. VR1/VR2 use legacy
  router links and VR212 has an old public port-link.
- The installed wrapper predates repository vs-link changes. Deploy wrapper
  and daemon/client together; a new wrapper requires `port show --all` and
  named VS creation, which the old daemon cannot accept.

VF0 currently has traffic in VS100. Validate that it has moved from VM use to
host cloudbr0 and that the bridge has only one route to physical VLAN6 before
resetting the production config. The plan requires stopping infrastructure
provisioning when VF0 belongs to a VM.

## Payload source

Inspected [official NetworkExtensionElement source](https://github.com/apache/cloudstack/blob/main/framework/extensions/src/main/java/org/apache/cloudstack/framework/extensions/network/NetworkExtensionElement.java).
Downloaded file SHA256:
`f4098ffa99fb4764e5723d09dd6999521011e5b2333b8d9035069e0867ba4b11`.
This verifies upstream, not the installed Management Server version. No
CloudStack API credentials/Management Server access were available.

`applyFWRules` queries all non-revoked DB rules and sends
`fw_rules={default_egress_allow,cidr,rules}`. Rule fields:
`id,type,protocol,portStart,portEnd,icmpType,icmpCode,publicIp,sourceCidrs,destCidrs`.
Egress rules deny when default-allow and allow when default-deny.
`applyPFRules` sends `public_ip,public_port,private_ip,private_port,protocol`
without PF ID. Revoke selects `delete-port-forward`.
[payloads/](payloads/) contains source-derived fixtures, not captured callbacks.
Check one actual installed callback before enabling the offering.

CloudStack must already contain `BroadcastDomainType.Dpu`, prepare/release NIC
PCI-return handling and KVM hostdev support; no CloudStack rebuild/source
changes were made. Aggregate restore is conditional on provider-owned
DHCP/DNS/UserData, so dedicated IP/firewall/PF callbacks and daemon persistence
are also required for recovery. Wrapper cache alone is not CloudStack authority.

## Deployment after host validation

1. Obtain KVM host `virtfn* -> PCI BDF` mapping. Register `vf.pool=1-50`,
   `vf.pci.map`, `vf.host=1`, `vf.pf=0`, correct Arm SSH host and production
   socket. Reserve guest VNETs900–999 and public VID6 on CloudStack.
2. Back up the current image ID/tag, installed client/wrapper, wrapper cache
   and both daemon config files. Capture status/ports/VS/router state before
   reset. Keep backups outside active state directories.
3. In a maintenance window stop the sole eSwitch service. Replace exact
   production files `/var/lib/eswitch-management/eswitch.conf` and its `.router`
   sibling with [cloudstack-eswitch.conf](cloudstack-eswitch.conf) and
   [cloudstack-eswitch.conf.router](cloudstack-eswitch.conf.router). These create
   shared infrastructure VS6 only; guest topology comes from CloudStack.
   Move the old wrapper cache aside; do not import old VID212/network211.
4. Install the wrapper shell (0755) and helper (0644) together in
   `/etc/cloudstack/extensions/dpu-eswitch`. Deploy `dpu-eswitch.sh` on the
   Management Server. Select built image `eswitch-management:3.4.0-cloudstack`,
   or retag it to service image3.4.0 after preserving the prior image. Start
   `eswitch-management-container.service`.
5. Copy client from the actual production container:
   `docker cp eswitch-management:/usr/local/bin/eswitchctl /usr/local/bin/eswitchctl`.
   Verify `vs show --id 6`, `port show --all`, and p0/VF0 trunk6 ownership.
6. Create the offering with SourceNat/Gateway/Firewall/PortForwarding. Test
   VID900/901, public-IP assignment, allowed/denied TCP/UDP/ICMP, PF replies,
   deletion/revocation/restore, and verify packet captures + hardware counter
   deltas according to the original plan.

## Migration and rollback

Daemon reads numeric state versions1–4; writes v5 optional names such as
`vswitch 900 VS900`. Numeric-only v5 records also work. An older daemon cannot
read v5: rollback restores the backed-up old config/router/cache/wrapper/client
and image while the service is stopped. Restoring just the image is insufficient.

New wrapper cache uses atomic JSON. Nonempty old VF allocation text is rejected
instead of guessing VM/PCI ownership. Shutdown retains desired state and VF
reservations; destroy frees only that network. Cleanup failures return nonzero
and keep cache for retry. VS6/members/other VRs are never deleted by tenant cleanup.

## Verification scope

Completed: adapter24 tests, proxy22 assertions, BF3 Meson11 tests, Docker
runtime dependency checks. Candidate image `eswitch-management:3.4.0-cloudstack`
is arm64, ID
`sha256:1b5b16919d4d9f062392f2be7c4925caca7e88776ee1fcd923ebb5d77adead4b`.
Production deployment completed on 2026-10-01 after the operator confirmed
VF0 was attached to host cloudbr0 and authorized resetting old topology.
Service image tag `eswitch-management:3.4.0` now points to this image. Installed
wrapper/helper and the host client use `/run/eswitch-management/control.sock`.
The running baseline is VS6 with p0/VF0 trunk VID6, 50 available guest VFs,
zero routers/policies/sessions, LPM capacity64 ready and CT capacity2048 ready.
The installed wrapper's `ensure-infrastructure` probe passed idempotently.
Prior config, image identity, wrapper/cache and client are recoverable in
`/var/backups/eswitch-management/cloudstack-20261001-quifOi`.
The initial attempt rolled back safely because the router template had a
comment before its required header; corrected and covered by a regression test.

BF3 Meson compilation/test suite and candidate Docker build use DOCA3.4.0112;
runtime linkage includes libdoca_flow. Offline tests cover lifecycle, two
VRs sharing VS6, source/destination CIDR expansion, default egress semantics,
malformed/unsupported payloads, PF ranges/replay/update/delete, restore,
runtime port remapping, VF50 and injected control failures.

Management Server proxy installation/registration was not performed from the
DPU workspace; ensure the deployed proxy and extension details match this
checkout, particularly the KVM-host `vf.pci.map` and guest VID range900–999.
Tagged switch↔cloudbr0 capture, installed CloudStack callback testing and
hardware hit deltas remain pending the operator's offering/network tests.
Offline tests/build do not prove hardware packet hits.

## Failed-first-implementation cleanup (2026-10-01)

Network214 failed VID validation before acquiring DPU resources. Cleanup used
to call identify() first, so release-ip/shutdown also failed and CloudStack
could not release its SNAT association. The installed helper now acknowledges
missing-VID cleanup only against a verified globally empty tenant baseline
(live routers0, VS6 only, no desired caches/VF allocations/unresolved IDs).
Nonempty firewall rules, invalid nonempty VIDs and provisioning remain rejected.
This conservative fallback refuses ambiguity even if resources belong to
another tenant; known-VID cleanup follows the original ownership path.
Installed helper backup:
`/var/backups/eswitch-management/cleanup-wrapper-20261001-qyuA7p`.
Production apply-fw-rules(empty), release-ip(161.246.6.14), shutdown-network and
destroy-network probes for214 all returned verified-empty success. No daemon
restart/image rebuild was needed. CloudStack still must retry deletion itself
to update its IP/network records; no Management Server/API access was available.

## zona-01 verified VF mapping (2026-10-01)

Operator supplied PF0000:84:00.0 virtfn mapping; installed guest VF1–50 in
`/etc/cloudstack/extensions/dpu-eswitch/vf-pci-map.json` (VF0 excluded).
Wrapper uses this file only when registration supplies neither map nor host PF.
Explicit registration is authoritative. Production read-only probes resolve
VF1=0000:84:00.3 and VF50=0000:84:06.4. Tests cover local-map prepare-nic,
access attachment and returned VM PCI metadata, plus explicit override behavior.
Backup: `/var/backups/eswitch-management/vf-pci-wrapper-20261001-l8Bmp9`.
VS920 remains empty until a genuine prepare-nic callback; no fabricated NIC
allocation or live VM hotplug was performed. VM hostdev creation is not yet
verified. The previously retrieved upstream NetworkExtensionElement.prepare()
only checks executeScript() success and does not consume PCI metadata itself;
the installed CloudStack must have PCI-return/KVM-hostdev integration to use
the wrapper's result. Check installed source/logs and domain XML if prepare-nic
succeeds but VM still lacks a PCI NIC.

## Egress ACL v52 candidate

DOCA3.4.0112 headers and shipped flow_acl_sample.c verified. The failing
IP-only local-RIF ACL entries are moved to a CONTROL guard; TCP/UDP reach
the five-tuple ACL while ICMP/other protocols retain Arm enforcement. ALL
policy rules expand into two ordered TCP/UDP entries; local-IP and exact PF
reply exceptions precede policy decisions. ACL entries have NON_SHARED
counters exposed as `egress_acl ... scope=tcp-udp other_protocols=arm hw_hits=...`.
Meson11 tests pass, including projection checks. Runtime linkage verified by
Docker build. Candidate `eswitch-management:3.4.0-egress-acl-v52` built on BF3;
Production deployment was authorized and attempted at 18:31 UTC on 2026-10-01.
The ACL constructor succeeded, but all three projected entries returned failed
ADD callbacks (`status=2`); entries-process returned Bad State for VR920/RIF1.
Cleanup then aborted with `double free or corruption (fasttop)`. The precise
entry rejection and cleanup crash causes are unresolved; do not treat this
candidate as deployable or its constructor success as hardware offload proof.
The deployment health gate rolled back the image, persisted configs and CLI.
Backup: `/var/backups/eswitch-management/egress-v52-20261001-CMJfDH`.
Verified production is active on v51 with VS6/VS920, VR920 and PF rule intact;
egress remains `active_policies=0 fallback_policies=1`. The v52 candidate image
is retained separately; the production `3.4.0` tag points to the previous image
`sha256:1b5b16919d4d9f062392f2be7c4925caca7e88776ee1fcd923ebb5d77adead4b`.
Controlled allow/deny/local-gateway/PF tests with hardware hit deltas were not
run against v52 because entry admission failed. No CloudStack-owned policy
was changed to mask fallback. Resolve the failed ACL commit in a controlled
diagnostic run before another production deployment.

## Egress hardware policy v53 (2026-10-01)

SDK logging added using the installed flow_acl_main.c backend sequence.
The unchanged v52 diagnostic reproduced failed ERP_PIPE commit callbacks and
an assertion in hws_definer.c:1332 during cleanup. Exact SDK/driver cause is
not established; no claim that ACL forwarding is generally unsupported.
Logs: `/tmp/eswitch-acl-diagnostic-z3OSez/runtime.log`.

v53 implements the same ordered TCP/UDP ACL semantics with DOCA Flow CONTROL,
following the already admitted ingress CONTROL shape. Inclusive port ranges
expand into disjoint ternary prefixes. Priorities0..6 preserve lower-rule-ID
first-match order; priority7 is an explicit counted default allow/drop entry.
ALL projects to TCP and UDP at the same priority; other protocols remain Arm.
Policies with more than seven TCP/UDP-relevant rules fall back entirely to Arm.
Local-RIF and exact PF reply exceptions remain in the preceding guard, and
exact flow authorization/CT still precedes enforcement. Atomic selector
replacement and policy-change invalidation are unchanged. Status identifies
`engine=control`; hw_rules/hw_hits now include the default entry.

Meson11 tests pass, including exhaustive16-bit port-prefix matching for
single ports, near-full range, unaligned range and upper-bound65535.
BF3 validation with unchanged config admitted four entries, with
active_policies1/fallback0/failures0 and readable counters. Diagnostic40-second
run terminated without the prior ACL assertion; shutdown did report existing
device-PD EBUSY/resource-in-use diagnostics, now visible with SDK logging.
Production remains able to start afterward; this cleanup warning is unresolved.

Built and deployed `eswitch-management:3.4.0-egress-control-v53`; production
tag `eswitch-management:3.4.0` now uses image
`sha256:d8620de262671d612b534ba1b8596d604d7c6106d6f47be247a3156df2debaab`.
Backup: `/var/backups/eswitch-management/egress-v53-20261001-dLSvW6`.
Deployment gate verified v53, admitted CONTROL entries, unchanged VS topology
and router config, and copied container eswitchctl to /usr/local/bin/eswitchctl.
Production control socket remains /run/eswitch-management/control.sock.
No CloudStack-owned rule changes were needed. At uptime138 seconds hw_hits=0,
guest_egress checked=0 and CT promotions=0: hardware installation is verified,
but packet-hit/allow/deny/PF regression validation awaits operator VM traffic.
