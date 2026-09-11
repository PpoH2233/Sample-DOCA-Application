# Minimal software TX reproducer (DOCA 3.4)

## VF10 quick PoC

This repository includes `run_vf10_poc.sh`, a guarded wrapper for the default
software-injection experiment:

```text
fresh Arm-generated ARP reply
  -> parent/proxy DPDK TX queue
  -> DOCA Flow EGRESS root in the eSwitch
  -> verified c1pf0vf10 representor
  -> VF10-backed VM
```

This path deliberately uses the DOCA Flow switch-manager proxy port rather than
a DOCA ETH SF. An SF is the application endpoint for the alternative DOCA ETH
TXQ datapath; it is not required for DOCA Flow software TX reinjection.

Build the reproducer as described in `Build in doca-dev`. Start an ingress-only
capture in the VF10 VM, stop all other owners of the parent PF, then run on the
DPU Arm:

```bash
sudo ./run_vf10_poc.sh \
  --vm-mac 7e:83:a5:77:11:06 \
  --gateway-mac 02:00:00:65:00:01 \
  --vm-ip 192.168.0.10 \
  --gateway-ip 192.168.0.1 \
  --confirm-exclusive-owner
```

Replace all addresses with the actual isolated test tuple. The wrapper verifies
root access, installed DOCA Common/Flow packages, executable availability and
`switchdev` state. The binary itself verifies that the supplied representor
resolves to `host=1 pf=0 vf=10` before TX. Neither component changes firmware,
creates a VF, changes eSwitch mode, or stops services.

Success has two independent proofs: `accepted=10 egress_hit=10` in the DPU log
proves software TX traversed the eSwitch EGRESS entry, while ten matching frames
in the VM capture prove delivery through VF10. A zero VM capture must not be
reported as delivery even if the hardware counter passes.

To test only **software TX -> EGRESS entry**, use the new
[`sw-egress` COUNT + DROP experiment](EGRESS_ENTRY_TEST.md). It prints baseline,
per-packet counter deltas and an explicit `EGRESS RESULT: PASS/FAIL`; it does not
require VM traffic or guest delivery.

## Experiment B: hardware ingress versus software TX

### No VM script: use `TX_PROBE_PATH=hw-arp`

Broadcast selector fix: according to [DOCA 3.4 Implicit Match](https://networking-docs.nvidia.com/doca/archive/3-4-0/doca-flow),
an all-ones template field is changeable. The entry must therefore also carry
`ff:ff:ff:ff:ff:ff` as the desired destination, rather than zero. `MATCH DEBUG`
prints the per-entry value before submission. The SDK-backed `probe-match`
Meson test checks this helper and verifies other modes keep zero entry fields;
it needs headers/libraries but no device access. This fixes the HW-ARP diagnostic
selector, not the original software TX problem. Rebuild and re-run HW-ARP before
drawing conclusions about ingress-to-egress forwarding.

Rebuild, then use the HW command below with `TX_PROBE_PATH=hw-arp` instead of
`hw`. After PROBE READY, run on the VF10 VM:

```bash
sudo arping -b -c 10 -I ens6 192.168.0.1
```

This requires the iputils version of arping (`-b` keeps requests broadcast).
The selector matches verified VF + VM source MAC + broadcast destination MAC +
ARP EtherType. It does not match the ARP target IP or opcode. Stop other ping/
arping tests to minimize unrelated ARP; excess counts require a quiet rerun,
not a conclusion that EGRESS is broken. Existing `hw` mode remains unchanged.
This forwards requests unchanged, **not** ARP replies; arping can time out even
when EGRESS counters pass. Only test on an isolated, non-bridging VM interface
to avoid reflected broadcast traffic. Check HW SUMMARY for selected=10/hit=10.
No software TX is performed in this mode either.

`TX_PROBE_PATH=sw` (default) retains the original software test.
`TX_PROBE_PATH=hw` disables software TX completely and uses:

```text
VF10 / VM source MAC / gateway destination MAC / EtherType 0x88b5
 -> DEFAULT selected entry + counter
 -> EGRESS root (same match-all/forward definition as SW mode) + counter
 -> verified VF10
All other ingress -> DROP
```

These are separate process runs, not simultaneous paths. EGRESS is now created
before ingress in BOTH modes so the destination exists before linking it.
Re-run SW with this build as well; counters refer to newly created entries per
run. No claim is made that this is the same live entry across restarts.

In `doca-dev`, rebuild, stop the daemon/other PF owners, and set:

```bash
meson compile -C /build/eswitch-tx-reproducer
export TX_PROBE_VF=10
export TX_PROBE_VM_MAC=7e:83:a5:77:11:06
export TX_PROBE_GATEWAY_MAC=02:00:00:65:00:01
export TX_PROBE_VM_IP=192.168.0.10
export TX_PROBE_GATEWAY_IP=192.168.0.1
TX_PROBE_PATH=hw /build/eswitch-tx-reproducer/eswitch-tx-reproducer \
  -l 0 --file-prefix=eswitch-tx-reproducer -- \
  --rep 'pci/03:00.0,c1pf0vf10' --expert-mode \
  --log-level 60 --sdk-log-level 60 > /tmp/eswitch-tx-hw.log 2>&1
```

In another container terminal, watch readiness:

```bash
tail -f /tmp/eswitch-tx-hw.log
```

Copy `send_hw_probe.py` from this directory to the VM (for example `/tmp`).
After `PROBE READY: path=hw`, run on the VM within the 30-second window:

```bash
sudo python3 /tmp/send_hw_probe.py --interface ens6 \
  --gateway-mac 02:00:00:65:00:01 \
  --expected-vm-mac 7e:83:a5:77:11:06
```

Do not run ping as the generator for HW mode: ARP/ICMP do not match this rule.
The script needs only Python's standard library and sends exactly ten 60-byte
unicast frames, one per second. The non-IP frames do not solicit kernel replies.
Use only the isolated VM interface, not a bridge that could reflect frames.
Optional capture in a second VM terminal (incoming only, to avoid confusing
locally transmitted frames with returned frames):

```bash
sudo tcpdump -Q in -eni ens6 -nn 'ether proto 0x88b5'
```

The destination MAC remains the gateway MAC; the test is EGRESS counter traversal,
not IP connectivity. Capture uses promiscuous mode but returned-frame visibility
is not guaranteed by a rule hit. A same-VF return may have additional downstream
restrictions; it does not invalidate an observed EGRESS counter hit.

After the HW process exits, re-run SW in the same container shell:

```bash
TX_PROBE_PATH=sw /build/eswitch-tx-reproducer/eswitch-tx-reproducer \
  -l 0 --file-prefix=eswitch-tx-reproducer -- \
  --rep 'pci/03:00.0,c1pf0vf10' --expert-mode \
  --log-level 60 --sdk-log-level 60 > /tmp/eswitch-tx-sw.log 2>&1
rg -n 'PROBE CONFIG|PROBE READY|HW SUMMARY|PROBE SUMMARY|CHECK FAILED|PROBE ERROR' \
  /tmp/eswitch-tx-hw.log /tmp/eswitch-tx-sw.log
```

HW success requires `ingress_selected=10 egress_hit=10`, with software TX off.
SW success still requires `accepted=10 egress_hit=10`. HW selected=0 is an
unexercised test (check generator timing/MAC/VF), not evidence about EGRESS.
HW selected=10 / EGRESS=0 isolates a cross-domain/EGRESS issue. HW 10/10 with
SW 10/0 focuses investigation on software injection. Counter success in either
run is not proof of guest delivery. Missing, excess or mismatched counts exit
nonzero with an explicit message. There is no automatic retry on hardware errors.

Negative traffic control: use a different *unicast* `--gateway-mac` in a separate
HW run; it should select zero frames and exit nonzero. This verifies the selector
is narrow, without opening another VF. Do not mix negative and positive runs.

The sections below describe the default **SW mode** unless otherwise noted.

Separate executable; does not link the management daemon, router, FDB or old TX
pipeline. `main.c` is adapted from NVIDIA's `flow_switch_to_wire_main.c`, with
its license retained. Build uses the installed SDK's `flow_common.c`,
`flow_switch_common.c`, `common.c`, `dpdk_utils.c`, and `utils.c` so device
probing and queue bring-up follow the sample in that SDK.

## Scope and safety

- Run inside `doca-dev` on the DPU, not the x86 host.
- Stop other owners of PCI `03:00.0` first. A different socket or EAL file prefix
  does **not** isolate hardware ownership. This diagnostic temporarily replaces
  steering on the selected eSwitch; it is not safe alongside production traffic.
- Open exactly one parent and one representor. Target must resolve to
  host=1, PF=0, VF=10..15 and equal `TX_PROBE_VF`.
- The sample helpers use logical port IDs 0/1. The reproducer validates parent
  port 0 through the DOCA bridge and reverse-maps port 1 to the requested VF;
  it aborts rather than using another port. Do not reuse daemon DPDK port IDs.
- No daemon configuration/state/socket is read or modified. No firmware or
  platform settings are changed. Restart the daemon after the diagnostic exits.

## Pipeline

```text
fresh 60-byte ARP reply, no TX metadata, ol_flags=0
  -> parent DPDK TX queue 0
  -> EGRESS root: match-all + non-shared counter
  -> verified VF representor (logical port 1)
  -> VM capture (external delivery proof)

wire / VF RX -> DEFAULT root: match-all + counter -> DROP
```

There is no ingress-to-egress connection and no RSS/RX forwarding loop.
The ingress DROP counter can show unrelated incoming traffic but is not a TX
success criterion. Egress miss is explicitly DROP. Reserve eight counters per
port; commit both entries and check their callbacks before any TX. Use the
sample's `switch,hws,hairpinq_num=4,expert` baseline. Keeping its hairpin setting
is not a claim that a missing hairpin setting caused the original bug.

After a five-second capture setup window, send ten packets at one per second.
Wait three seconds after sending for counters. The send loop frees only rejected
mbufs; accepted mbufs belong to DPDK. The private TX pool is freed after ethdev
queues stop/close, not while TX can still reference it.

## Build in doca-dev

From this directory in the container (not `/build/eswitch-management`):

```bash
pkg-config --modversion doca-flow libdpdk
meson setup /build/eswitch-tx-reproducer .
meson compile -C /build/eswitch-tx-reproducer
meson test -C /build/eswitch-tx-reproducer --print-errorlogs
ldd /build/eswitch-tx-reproducer/eswitch-tx-reproducer
```

Confirm `libdoca_flow` and NVIDIA DPDK resolve to the intended installed SDK.
Default SDK root is `/opt/mellanox/doca`; override with
`-Dsdk_root=/path/to/doca` if necessary. SDK development headers and sample/common
sources are required. This is an independent Meson project; rebuilding the
management daemon does not build this diagnostic.

## Run the current VM test

On the VM, start capture **before** launching the reproducer:

```bash
sudo tcpdump -eni ens6 -nn -vv 'arp and ether src 02:00:00:65:00:01'
```

Inside `doca-dev`, after stopping the other PF owner:

```bash
TX_PROBE_VF=10 \
TX_PROBE_VM_MAC=7e:83:a5:77:11:06 \
TX_PROBE_GATEWAY_MAC=02:00:00:65:00:01 \
TX_PROBE_VM_IP=192.168.0.10 \
TX_PROBE_GATEWAY_IP=192.168.0.1 \
/build/eswitch-tx-reproducer/eswitch-tx-reproducer \
  -l 0 --file-prefix=eswitch-tx-reproducer -- \
  --rep 'pci/03:00.0,c1pf0vf10' --expert-mode \
  --log-level 60 --sdk-log-level 60
```

Use VF10 here because the prior daemon reverse-mapped the VM to host=1 PF=0
VF=10. It was DPDK port 2 in that daemon, but must map to port 1 in this
single-VF test. The program prints `PORT MAP` and rejects a mismatch.
All five environment settings are required, including when asking for help.
No `eswitchctl`, gateway ping, ARP request, or L2 switch configuration is needed.

Expected wire frame: unicast ARP reply, `192.168.0.1 is-at
02:00:00:65:00:01`, Ethernet destination `7e:83:a5:77:11:06`, ARP target IP
`192.168.0.10`. This is an unsolicited reply; success is seeing it in tcpdump,
not a successful ping or guaranteed neighbor-cache update. ICMP is not implemented.

## Interpret results

| Observation | Meaning / next investigation |
| --- | --- |
| `TX ACCEPTED` stays zero | TX queue/ethdev path; no delivery claim |
| accepted=10, egress_hit=0 | Software TX has no observed match in EGRESS; investigate bring-up/domain attachment/telemetry, not the ARP match fields |
| accepted=10, egress_hit=10, VM sees zero | EGRESS was exercised; investigate forwarding target and downstream VF/VM path |
| accepted=10, egress_hit=10, VM sees ten replies | Minimal software-to-VF path confirmed; compare daemon bring-up against this baseline |

`parent_opackets_delta` is ethdev telemetry, not delivery proof.
`PROBE SUMMARY` always says `guest_delivery=UNVERIFIED`: capture must establish
delivery separately. Zero enqueues, incomplete/mismatched EGRESS counts, API
errors, and interruption exit nonzero. A successful exit establishes only ten
accepted packets and ten EGRESS hits, not guest delivery.

Negative checks: missing settings or multicast/zero MAC fail before EAL;
an out-of-scope VF setting fails before EAL; a selector/VF mapping mismatch
fails before Flow rules and TX. Do not use production VFs for negative tests.

## Local validation status

The packet-only test compares every byte (including zero padding) and rejects
malformed, zero and multicast MAC addresses. It can also run without DOCA:

```bash
cc -std=c11 -Wall -Wextra -Werror probe_packet_test.c -o /tmp/probe-packet-test
/tmp/probe-packet-test
```

The local authoring machine has no DOCA SDK or DPU. The packet test passed with
AddressSanitizer/UBSan; the hardware executable still needs compilation against
the installed 3.4.0112 headers and a run/capture in `doca-dev`. No hardware TX
success or root cause is claimed by this change.
