# VF10 RX monitor (DOCA Flow 3.4)

This diagnostic receives and prints traffic originating from the VM attached to
host 1, PF 0, VF 10.

```text
VM -> VF10 -> eSwitch DEFAULT root -> RSS queue 0 -> DPU container
```

The VF representor is a DOCA Flow source selector, not a DPDK RX queue. The app
validates that logical port 1 resolves to `host=1 pf=0 vf=10`, matches
`parser_meta.port_id=1`, forwards only that traffic to the switch-manager/proxy
RSS queue, and calls `rte_eth_rx_burst(0, 0, ...)` on the parent port. All other
eSwitch ingress is dropped for the duration of this isolated diagnostic.

## Safety

Run only on an isolated test system. The application temporarily owns PCI
`03:00.0` and installs a DEFAULT root pipe. Stop the eSwitch management daemon,
OVS-DPDK and every other DOCA Flow/DPDK owner first. The application does not
change firmware, VF allocation, VM attachment, or switchdev mode.

## Build inside doca-devel on the DPU

```bash
cd <repo>/application/eswitch_management/diagnostics/vf10_rx_monitor
pkg-config --modversion doca-flow libdpdk
meson setup /build/vf10-rx-monitor .
meson compile -C /build/vf10-rx-monitor
ldd /build/vf10-rx-monitor/vf10-rx-monitor | grep -E 'doca_flow|rte_'
```

The build intentionally uses the helper sources shipped with the installed
DOCA SDK, making the installed 3.4 headers and samples the API source of truth.

## Run

On the DPU/container:

```bash
VF10_MONITOR_SECONDS=30 \
VF10_MONITOR_MAX_PACKETS=100 \
/build/vf10-rx-monitor/vf10-rx-monitor \
  -l 0 --file-prefix=vf10-rx-monitor -- \
  --rep 'pci/03:00.0,c1pf0vf10' --expert-mode \
  --log-level 60 --sdk-log-level 60
```

Wait for:

```text
PORT MAP: parent=0 representor=1 host=1 pf=0 vf=10 expected=1/0/10
MONITOR READY: source=host1/pf0/vf10 ...
```

Then generate bounded traffic inside the VM, for example:

```bash
sudo arping -b -c 5 -I <vm-vf-interface> 192.168.0.1
```

Each selected frame prints its length, mbuf port, source/destination MAC,
EtherType and offload flags. A healthy final result has both counters above
zero:

```text
MONITOR SUMMARY: flow_selected=5 software_received=5 ...
```

`flow_selected > software_received` indicates RSS/queue loss or that the monitor
hit its packet/time limit. `flow_selected=0` means the representor selector saw
no VF10 traffic. `flow_selected>0` with `software_received=0` isolates the fault
to RSS delivery or parent RX queue setup.
