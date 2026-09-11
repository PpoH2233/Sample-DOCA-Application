# VF10 TX generator (DOCA Flow 3.4)

This isolated diagnostic continuously creates Ethernet frames on the DPU Arm
and dispatches them through the eSwitch to host 1, PF 0, VF 10:

```text
DPU container -> parent/proxy TX queue -> eSwitch -> c1pf0vf10 -> VM
```

It uses non-expert switch mode. Every fresh mbuf receives destination logical
port `1` through `rte_flow_dynf_metadata_set()` and the
`RTE_MBUF_DYNFLAG_TX_METADATA` flag before being sent on parent DPDK port 0.
This follows DOCA 3.4's direct destination-metadata path and intentionally does
not use the expert-mode EGRESS-root experiment.

## Safety

Run only in an isolated test environment. Stop the eSwitch management daemon,
OVS-DPDK and every other DPDK/DOCA Flow owner of parent PF `03:00.0` first. The
program does not change firmware, switchdev mode, VF allocation, or VM state.

## Build inside doca-devel on the DPU

```bash
cd <repo>/application/eswitch_management/diagnostics/vf10_tx_generator
pkg-config --modversion doca-flow libdpdk
meson setup /build/vf10-tx-generator .
meson compile -C /build/vf10-tx-generator
ldd /build/vf10-tx-generator/vf10-tx-generator | grep -E 'doca_flow|rte_'
```

## Capture in the VM

Use the interface backed by VF10:

```bash
sudo tcpdump -Q in -eni <vm-vf-interface> -nn -vv -XX 'ether proto 0x88b5'
```

## Run on the DPU

Do not pass `--expert-mode`:

```bash
VF10_TX_DEST_MAC=A6:94:27:FB:6C:38 \
VF10_TX_SOURCE_MAC=02:00:00:65:00:01 \
VF10_TX_INTERVAL_MS=1000 \
/build/vf10-tx-generator/vf10-tx-generator \
  -l 0 --file-prefix=vf10-tx-generator -- \
  --rep 'pci/03:00.0,c1pf0vf10' \
  --log-level 60 --sdk-log-level 60
```

The destination MAC in the example is taken from the successful VF10 RX
capture. Confirm it is still the VM interface MAC before running. By default the
generator sends until Ctrl-C. Set `VF10_TX_PACKET_LIMIT=10` for a bounded test.

### RX-context diagnostic matrix

The installed `flow_switch_to_wire` sample only re-transmits mbufs for which an
RX CQE supplied `RTE_MBUF_F_RX_FDIR_ID`; it does not demonstrate a completely
fresh mbuf. To isolate that difference, run exactly ten packets with:

```bash
VF10_TX_DEST_MAC=A6:94:27:FB:6C:38 \
VF10_TX_SOURCE_MAC=02:00:00:65:00:01 \
VF10_TX_INTERVAL_MS=500 \
VF10_TX_PACKET_LIMIT=10 \
VF10_TX_CONTEXT_MATRIX=1 \
/build/vf10-tx-generator/vf10-tx-generator \
  -l 0 --file-prefix=vf10-tx-generator -- \
  --rep 'pci/03:00.0,c1pf0vf10' \
  --log-level 60 --sdk-log-level 60
```

Packets 1-5 use destination metadata only. Packets 6-10 additionally reproduce
the RX-visible FDIR flag and value from logical port 1. If only sequences 6-10
reach the VM, the missing RX-origin context is the isolated cause. If neither
group arrives, the result rules out that observable mbuf difference and the
next comparator must use a real non-expert RX mbuf or an installed NVIDIA
application that creates a fresh reply. This matrix intentionally tests a PMD
behavior and is not a production recommendation to put RX flags on generated
TX packets.

The generator also prints every parent mlx5 extended statistic whose value
increased during the run. `XSTAT DELTA` names containing `phy`/`wire` indicate
that the supposedly VF-directed packet escaped through the physical port;
names containing `vport`/`representor` indicate progress toward the VF. Treat
the actual names reported by the installed PMD as authoritative because the
available xstats differ between driver releases.

Expected startup:

```text
PORT MAP: parent=0 destination=1 host=1 pf=0 vf=10 expected=1/0/10
GENERATOR READY: path=non-expert-tx-metadata ...
```

Each frame has EtherType `0x88b5`, the ASCII marker `BF3-VF10-TX`, an unsigned
64-bit sequence and a monotonic timestamp. `TX ... accepted=1` proves only that
the parent PMD accepted the mbuf. Packet visibility in the VM's ingress-only
tcpdump is the delivery proof.
