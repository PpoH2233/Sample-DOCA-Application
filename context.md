# BlueField handoff: DOCA software TX does not enter EGRESS

## Objective

Find and prove why `eswitch-management` receives an ARP request and enqueues a
correct reply with `rte_eth_tx_burst()`, but that reply neither hits the DOCA
Flow EGRESS root nor appears in the VF10 VM.

Run this work on the BlueField Arm side in `doca-dev`. Continue until a positive
control identifies the failing boundary. TX acceptance and ethdev `opackets`
are not delivery proof.

## Test identity

- Parent PCI function: `03:00.0`
- Minimal reproducer ports: parent `0`, target `1`
- Target: `c1pf0vf10`, verified as `host=1 pf=0 vf=10`
- VM interface/MAC/IP: `ens6`, `7e:83:a5:77:11:06`, `192.168.0.10`
- Gateway MAC/IP: `02:00:00:65:00:01`, `192.168.0.1`
- Observed versions: DOCA Flow `3.4.0112`, DPDK `26.03.0-doca4`
- Flow mode: `switch,hws,hairpinq_num=4,expert`

Recheck identities on the target. DPDK numeric IDs can change after probing a
different representor set.

## Ownership rule

Stop `eswitch-management` and every other owner of PCI `03:00.0` before running
the standalone reproducer. Different EAL prefixes and sockets do not isolate the
same eSwitch. Use the existing isolated VF10 environment. Do not change
firmware, BlueField mode, or saved production configuration during this test.

Inspect `git status` before editing and preserve the current diagnostic changes.

## Evidence already established

The daemon builds a valid 60-byte ARP reply:

```text
Ethernet dst=7e:83:a5:77:11:06 src=02:00:00:65:00:01 type=0x0806
ARP opcode=reply
sender=192.168.0.1 / 02:00:00:65:00:01
target=192.168.0.10 / 7e:83:a5:77:11:06
```

In the daemon, `arp_seen`, `arp_built`, `arp_tx_enqueued`, and parent `opackets`
increase together. Software drop counters and parent `oerrors` remain zero, but
`egress_enter`, `arp_probe_hit`, and `tx_probe_drop` remain zero. VM `tcpdump`
sees no reply.

The standalone `sw-egress` test used:

```text
fresh software TX -> EGRESS root match-all -> COUNT + DROP
```

It reported ten accepted packets, `parent_opackets_delta=10`, no errors, and
zero EGRESS/ingress hits. This puts the failure before the ARP selector, VF
forwarding action, and VM receive path.

The `sw-egress-matrix` test retained the same EGRESS entry and tried:

```text
plain fresh mbuf                                      -> 0 hits
fresh mbuf with mbuf->port=0                         -> 0 hits
port 0 plus TX metadata flag/value 0                 -> 0 hits
port 0 plus TX metadata flag/value 1                 -> 0 hits
```

All 12 packets were accepted. Therefore `mbuf->port` and the tested TX metadata
values are not sufficient fixes for a newly allocated mbuf.

`processed=2 failure=0` confirms that both diagnostic Flow entries completed
installation. `PROBE ERROR: Bad State` is deliberately returned after the
counter assertion fails. The `already detached` warnings happen during cleanup.

## Required next test: actual RX mbuf reinjection

The installed NVIDIA `flow_switch_to_wire` sample re-transmits an mbuf received
from RSS. The daemon currently frees the received request and constructs a fresh
reply mbuf. `TX_PROBE_PATH=rx-reinject` now tests whether an actual RX mbuf
carries context that the fresh allocation lacks:

```text
VF10 broadcast ARP request
  -> DEFAULT exact VF/MAC/ARP selector
  -> parent RSS queue 0
  -> re-transmit the same received rte_mbuf on parent TX queue 0
  -> EGRESS root match-all -> COUNT + DROP
```

Packets 1-5 retain received mbuf state. Packets 6-10 additionally receive TX
metadata value 1 and `RTE_MBUF_DYNFLAG_TX_METADATA`.

### Build in doca-dev

After syncing the current repository to the BlueField/container:

```bash
cd /doca_devel/Sample-DOCA-Application/application/eswitch_management/diagnostics/tx_reproducer
meson setup --reconfigure /build/eswitch-tx-reproducer .
meson compile -C /build/eswitch-tx-reproducer
meson test -C /build/eswitch-tx-reproducer --print-errorlogs
```

If the build directory is absent, use `meson setup /build/eswitch-tx-reproducer .`
once. Do not build `/build/eswitch-management`; it is a different executable.

### Run on BlueField Arm

```bash
TX_PROBE_PATH=rx-reinject \
TX_PROBE_VF=10 \
TX_PROBE_VM_MAC=7e:83:a5:77:11:06 \
TX_PROBE_GATEWAY_MAC=02:00:00:65:00:01 \
TX_PROBE_VM_IP=192.168.0.10 \
TX_PROBE_GATEWAY_IP=192.168.0.1 \
/build/eswitch-tx-reproducer/eswitch-tx-reproducer \
  -l 0 --file-prefix=eswitch-tx-reproducer -- \
  --rep 'pci/03:00.0,c1pf0vf10' --expert-mode \
  --log-level 50 --sdk-log-level 50 \
  > /tmp/eswitch-rx-reinject.log 2>&1
```

Watch readiness:

```bash
tail -f /tmp/eswitch-rx-reinject.log
```

Only after `REINJECT READY`, run on the VF10 VM:

```bash
sudo arping -b -c 10 -I ens6 192.168.0.1
```

Timeout is expected because the EGRESS action is DROP. Inspect the result:

```bash
rg -n 'PORT MAP|PROBE CONFIG|BASELINE|REINJECT READY|REINJECT BUILD|REINJECT SAMPLE|REINJECT RESULT|CHECK FAILED|PROBE ERROR' \
  /tmp/eswitch-rx-reinject.log
```

## Decision table

| Result | Conclusion and next action |
| --- | --- |
| `received<10` | RSS input did not fully exercise the test. Fix timing/mapping and repeat. |
| `original_hits=5` | Actual RX mbuf enters EGRESS while fresh mbufs fail. The fresh ARP reply lacks RX-origin context. Convert the received request into a reply in place, then verify the daemon. |
| `original_hits=0 metadata_hits=5` | RX reinjection requires the TX metadata flag/value. Apply that proven condition to daemon TX, then verify end to end. |
| `received=10 accepted=10`, both groups zero | Even sample-shaped reinjection misses EGRESS. Run the unmodified installed `flow_switch_to_wire` sample as the final environment control and compare its probe/devargs/queue setup. |
| EGRESS hits but ingress delta differs from 10 | The run is contaminated or the selector differs. Inspect and repeat quietly. |

Do not change the daemon until a positive variant establishes the required
condition. A final fix is verified only when all three occur:

```text
daemon arp_tx_enqueued increases
daemon egress_enter and arp_probe_hit increase by the same amount
VF10 tcpdump sees the unicast ARP replies
```

## If RX reinjection also fails

Use the installed source as ground truth:

```text
/opt/mellanox/doca/samples/doca_flow/flow_switch_to_wire/
```

Build and run that copy without modifying it, with `--expert-mode` and the same
parent/VF10. Preserve the full command and log. Also collect:

```bash
pkg-config --modversion doca-flow libdpdk
uname -r
ethtool -i p0
```

If the installed sample also cannot exercise EGRESS, the failing boundary is
the installed SDK/DPDK/driver/firmware or platform setup, not the ARP builder.

## Relevant project files

- `application/eswitch_management/diagnostics/tx_reproducer/tx_probe.c`
- `application/eswitch_management/diagnostics/tx_reproducer/EGRESS_ENTRY_TEST.md`
- `application/eswitch_management/diagnostics/tx_reproducer/README.md`
- `application/eswitch_management/diagnostics/DAEMON_TX_TEST.md`
- `application/eswitch_management/eswitch_manager.c`
- `application/eswitch_management/pipeline/eswitch_pipeline.c`

DOCA 3.4 documents that software TX in switch mode automatically enters the
EGRESS root. These controls verify that contract on this exact BlueField stack
and identify the application/runtime condition that makes it true.
