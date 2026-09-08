# Daemon EGRESS TX recheck (DOCA 3.4)

Revision: `doca34-egress-parent-v2`. This is still a single selected VS/VM
diagnostic path, not unrestricted multi-VM router TX.

Reviewed against the local NVIDIA `flow_switch_to_wire` source and
[DOCA 3.4 Flow](https://networking-docs.nvidia.com/doca/archive/3-4-0/doca-flow).
The daemon's per-entry source/destination MACs were already supplied correctly.
The HW-ARP reproducer broadcast-template bug does not apply to this unicast TX
selector. Fresh mbufs, no metadata, completion before TX, and accepted-mbuf
ownership remain unchanged.

Changes: use the actual parent Flow handle for switch lookup, adopt the sample
`switch,hws,hairpinq_num=4,expert` mode, explicit counted DROP on EGRESS root
miss, tuple logging before entry submission, and revision in startup/status.
These align the baseline, **not** establish that NULL lookup or missing hairpin
configuration caused the old TX failure. HW ingress-to-EGRESS was confirmed;
software TX-to-EGRESS and guest delivery still require verification.

## Build the daemon, not the reproducer

Sync the changed source into doca-dev. Use the daemon source directory:

```bash
cd /doca_devel/Sample-DOCA-Application/application/eswitch_management
rg -n 'doca34-egress-parent-v2' pipeline/tx_build.h
meson setup --reconfigure /build/eswitch-management .
meson compile -C /build/eswitch-management
meson test -C /build/eswitch-management --print-errorlogs
strings /build/eswitch-management/eswitch-management | rg 'doca34-egress-parent-v2'
sha256sum /build/eswitch-management/eswitch-management
```

Rebuilding does not replace a running process. Stop the test daemon/reproducer
before restarting; never run alongside another owner of 03:00.0. Different
sockets and EAL file prefixes do not isolate the eSwitch hardware.

```bash
ESWITCH_CONTROL_SOCKET=/run/eswitch-router-test/control.sock \
ESWITCH_STATE_FILE=/var/lib/eswitch-router-test/eswitch.conf \
ESWITCH_VF_SCOPE=10-15 \
ESWITCH_TX_PROBE_VS=100 \
ESWITCH_TX_PROBE_VM_MAC=7e:83:a5:77:11:06 \
/build/eswitch-management/eswitch-management \
  -l 0 --file-prefix=eswitch-router-test -- 03:00.0 \
  > /tmp/eswitch-management-tx.log 2>&1
```

The scope excludes VF9; if saved config references an excluded VF, resolve
that explicitly rather than widening to production ports. Port IDs can change
after a scope change: verify inventory by host/PF/VF, not old DPDK numbers.
No state file is overwritten by these instructions outside normal daemon use.

On the VF10 VM, capture and generate ARP in separate terminals:

```bash
sudo tcpdump -eni ens6 -nn 'arp and ether src 02:00:00:65:00:01'
sudo arping -b -c 10 -I ens6 192.168.0.1
```

Then in another doca-dev terminal:

```bash
ESWITCH_CONTROL_SOCKET=/run/eswitch-router-test/control.sock \
  /build/eswitch-management/eswitchctl status
rg -n 'TX BUILD|TX CONFIG|TX DOMAIN|TX MATCH|TX PROBE ARMED|ARP TX|TX PLAN A SKIP' \
  /tmp/eswitch-management-tx.log
```

Status must show `tx_revision=doca34-egress-parent-v2`. That verifies the running
daemon revision, not merely the CLI version. Missing probe environment variables
leave the path unarmed and fail closed. `TX MATCH` should show gateway MAC as
source, VM MAC as destination, and the dynamically mapped VF port as target.

Expected sequence: probe armed -> ARP TX enqueued -> egress_enter increases ->
arp_probe_hit increases -> VM captures reply. Neither enqueue nor a rule hit
alone proves guest receipt. If root is zero, investigate injection/attachment;
if root increases but probe stays zero, investigate tuple and DROP counter;
if both increase but no capture, investigate downstream VF/VM delivery.
ICMP reply/routing is outside this milestone, so ping completion is not required.

Local strict compiler + ASan/UBSan checks passed for ARP building and selector
isolation. Full SDK compilation and hardware verification must run in doca-dev.
