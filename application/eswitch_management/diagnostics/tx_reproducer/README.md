# Minimal software TX reproducer (DOCA 3.4)

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
