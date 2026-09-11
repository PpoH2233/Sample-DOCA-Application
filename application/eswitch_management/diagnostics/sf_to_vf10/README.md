# SF endpoint to VF10 PoC

This isolated diagnostic separates packet generation from the eSwitch manager
PF. Linux sends a 60-byte Ethernet frame through the real local SF endpoint,
and a counted DOCA Flow rule forwards frames arriving from the SF representor
to external-host VF10:

```text
AF_PACKET generator -> enp3s0f0s0 (SF endpoint)
  -> pf0sf0 representor -> DOCA Flow SF_TO_VF10 -> c1pf0vf10 -> VM
```

The Linux socket is only the packet source. Steering is performed by
`libdoca_flow` in hardware. This program creates a match on the runtime-resolved
SF logical port and EtherType `0x88b5`, attaches a non-shared counter, and uses
`DOCA_FLOW_FWD_PORT` to the VF whose topology is validated as host 1/PF 0/VF 10.

Run only in an isolated test environment with every other owner of parent PF
`03:00.0` stopped. The root miss action is DROP while the PoC is active.

## Prerequisite

Confirm that both the SF endpoint and its representor exist:

```bash
ip -d link show enp3s0f0s0
doca_caps --list-rep-devs --pci-addr 03:00.0
sudo devlink port show | grep -E 'flavour pcisf|sfnum 0|vfnum 10'
```

The application does not create or modify an SF. Bring the existing endpoint
up before starting the PoC:

```bash
sudo ip link set dev enp3s0f0s0 up
```

## Build

```bash
cd <repo>/application/eswitch_management/diagnostics/sf_to_vf10
meson setup /build/sf-to-vf10 .
meson compile -C /build/sf-to-vf10
ldd /build/sf-to-vf10/sf-to-vf10 | grep -E 'doca_flow|rte_'
```

## Capture in the VF10 VM

```bash
sudo tcpdump -Q in -eni enp5s0 -nn -vv -XX 'ether proto 0x88b5'
```

## Run on the DPU

The two representor arguments open local SF0 and external-host VF10 under the
same eSwitch parent. Confirm both identities with `doca_caps --list-rep-devs`
before running. The program reverse-maps the resulting DPDK ports and refuses
to run unless it finds exactly one SF and host 1/PF 0/VF 10.

Do not derive the representor index from the auxiliary device suffix. On this
system `parentdev mlx5_core.sf.1` is the auxiliary-device index, while
`doca_caps` and `devlink` report the eSwitch identity as `sf_index 0` / `sfnum
0`; therefore the correct representor identifier is `pf0sf0`.

```bash
SF_TX_IFACE=enp3s0f0s0 \
SF_TX_DEST_MAC=A6:94:27:FB:6C:38 \
SF_TX_PACKET_COUNT=10 \
SF_TX_INTERVAL_MS=500 \
/build/sf-to-vf10/sf-to-vf10 \
  -l 0 --file-prefix=sf-to-vf10 -- \
  --rep 'pci/03:00.0,pf0sf0' \
  --rep 'pci/03:00.0,c1pf0vf10' \
  --no-wire2wire --expert-mode \
  --log-level 60 --sdk-log-level 60
```

Expected proof:

```text
PORT MAP: parent=0 sf=<id> vf10=<id>
SF->VF10 SUMMARY: socket_sent=10 flow_hits=10 ...
```

`flow_hits=10` proves the packets entered the eSwitch from the SF and matched
the VF10 forwarding rule. The VM tcpdump must also receive ten packets before
guest delivery is considered proven.
