# eSwitch Management

Router integration is in progress: [router/README.md](router/README.md) records
implemented control commands, readiness, SDK prerequisites, and the VF 11–15
test scope. L2 membership/FDB lives in `l2/`, shared hardware steering in
`pipeline/`, socket transport in `control/`, and VR configuration in `router/`.
Router commands persist desired configuration. Addressed private vs-link RIFs
support gateway ARP, local ICMP and Arm longest-prefix routing between private
vSwitches. Public Arm routing and stateful TCP/UDP/ICMP Echo NAT are active.
Private-to-private IPv4 routing has an opt-in DOCA Flow LPM fast path;
optional DOCA Flow CT promotion offloads established TCP/UDP NAT sessions.
ICMP and all CT misses remain on the Arm slow path.
The default VF scope is now `7-15`; explicit settings override that default.
Existing build directories retain their Meson option: use `meson configure
/build/eswitch-management -Dvf_scope=7-15` and rebuild. An exported
`ESWITCH_VF_SCOPE` still overrides the compiled default.

`eswitch-management` is the single owner of the BlueField eSwitch, DOCA Flow
runtime, parent device, selected VF representors and one Arm system-SF
representor. The SF is reserved infrastructure: it is never shown as an
available tenant port and cannot be attached to a VS or VR. `eswitchctl` sends
local control commands over `/run/eswitch-management/control.sock`; it never
initializes DPDK or DOCA itself.

Commands are resource-first, `<resource> [sub-resource] <action> --options`, for
example `vs port attach --id 10 --port 1`. The version-1 flat verbs such as
`vs-create` and `list-port-available` still work but are deprecated;
[CLI.md](CLI.md) is the command contract and holds the alias table.

At startup every discovered DPDK port is **unassigned**. The root pipe has a
DROP miss action, so an unassigned VF or uplink cannot exchange traffic through
this application. A port belongs to at most one virtual switch.

## Data path

With `ESWITCH_HW_ROUTING=1`, packets addressed to a private RIF MAC first
separate local router IPs from transit traffic. Valid unfragmented IPv4 with a
20-byte header and TTL greater than one enters a VR-keyed hardware LPM table.
Resolved private adjacencies rewrite source/destination MAC, decrement TTL and
forward directly through the target egress gate. LPM misses, unresolved
neighbors, options, fragments, invalid checksums, TTL exceptions, public
uplinks and first packets of NAT flows continue to the Arm slow path. With
both `ESWITCH_HW_ROUTING=1` and `ESWITCH_HW_CT=1`, a successfully translated
TCP/UDP first packet installs a bidirectional CT entry. Later packets take
`LPM miss -> CT` outbound or `uplink -> CT` inbound; a CT hit performs NAT in
hardware and selects a post-CT L2/TTL adjacency. A miss still reaches Arm.
ICMP Echo NAT intentionally remains on Arm. The default is off until the
target BF3 passes the smoke test below.
Successful per-packet traces are disabled by default; set
`ESWITCH_PACKET_DEBUG=1` temporarily for packet-level diagnosis.
Broadcast ARP arriving on a router uplink is rate-limited in DOCA Flow before
RSS with `ESWITCH_UPLINK_ARP_PPS` (default 256 packets/s) and
`ESWITCH_UPLINK_ARP_BURST` (default 64 packets). Set the rate to `0` to disable
the hardware classifier. DOCA Flow 3.4 does not expose ARP TPA/opcode as match
fields, so the hardware meter deliberately preserves a bounded broadcast
sample and the Arm parser remains responsible for exact target/opcode checks.
Unicast ARP replies, IPv4 and other uplink traffic bypass the meter. If the
meter cannot be admitted, the uplink fails open to the existing Arm path and
status reports `uplink_arp_classifier=fallback-arm`. A ready classifier also
reports `hw_drops` from the color-pipe miss counter; this value must increase
when the offered broadcast-ARP rate exceeds the configured meter.
`ESWITCH_HW_ROUTE_CAPACITY` selects a power-of-two capacity from 64 to 1024
(default 1024). The requested capacity is included when actions memory is
reserved before Flow ports start, in addition to the existing L2/SF action
pool. If the LPM allocation is still too large,
the application retries smaller tables; if none can be admitted it remains
available on the Arm dataplane with `hw_state=fallback-arm`.

```text
endpoint
   -> root classifier: physical ingress port
      -> write pkt_meta = (vswitch_id << 16) | ingress_port_id
      -> router uplink: broadcast ARP hardware meter -> color gate -> Arm RSS
         -> IPv4 TCP/UDP -> CT hit -> adjacency -> egress
                         \-> CT miss -------------> Arm RSS
         -> other non-broadcast traffic -------------------------> Arm RSS
      -> source guard
         hit  -> destination FDB -> known-unicast egress
         miss -> clone one copy to Arm RSS and continue to destination FDB
      -> destination miss -> per-vSwitch flood selector
         -> one flooding HASH pipe containing the vSwitch member ports
      -> each selected egress gate
         -> ingress == egress -> DROP
         -> otherwise         -> physical DPDK port
root miss -> DROP
```

Packets created on Arm take a separate ingress leg. Known unicast output uses
the target port already resolved by the router; broadcast probes join the VS
destination/flood path:

```text
raw Ethernet frame on actual SF (default enp3s0f0s0)
   -> system-SF representor root entry
      -> private context VLAN: pop tag, restore VS and virtual RIF source MAC
         -> directed context -> target VF egress gate -> VM
         -> flood context    -> destination FDB/flood selector -> VS members
unknown SF context VLAN -> DROP
```

Gateway replies and routed unicast do not perform a second destination-FDB
lookup after software has already resolved their egress port. ARP neighbor
probes remain broadcast through the VS flood context. This removes both the
unsupported parent-PF software-TX shortcut and the return-path dependency on a
potentially changing learned FDB entry. A successful raw-socket send only
proves that the packet entered the SF endpoint; guest capture remains the
delivery proof.
Treat this SF as a dedicated application endpoint while the daemon owns the
eSwitch; ordinary host networking on the same SF is outside this design.

The Arm copy learns `(vswitch_id, untagged VLAN 0, source MAC)`. Membership
changes update only one HASH member entry and one root classifier entry. A
learned destination uses one hardware rule keyed by `(vswitch_id, dst_mac)`;
it is not expanded per ingress port. Detaching a port removes only MACs learned
on that port, so unrelated FDB entries remain installed.

An egress gate is created lazily once per physical DPDK port and shared by all
vSwitches. It implements split horizon from the low 16 bits of `pkt_meta`, so
both known-unicast and flooded traffic can never return to their ingress port.
The current implementation supports at most 254 members in one vSwitch; this
limit keeps each flooding HASH pipe within the DOCA Flow flooding fan-out
range.

## Hardware resource model

For `P` probed ports, `M` attached memberships, `V` non-empty vSwitches and
`F` learned MAC addresses, the dynamic steering state is approximately:

```text
root classifier entries       M
system-SF root entries         1
SF flood-context entries       <= configured private RIFs used for TX
SF directed-context entries    <= active (RIF, target port) pairs
shared egress-gate pipes       <= P       (2 control entries per used port)
vSwitch flooding HASH pipes   V
flood member entries          M
destination FDB entries       F
source-guard entries/counters F
```

The important change from the original implementation is that destination FDB
state is `O(F)`, not `O(F * M)`, and attach/detach changes `O(1)` membership
rules rather than rebuilding every ingress-specific flood path. Operations are
serialized in the manager loop. When a multi-step mutation fails, the manager
attempts to restore the previous classifier, flood membership, and MAC-move
forwarding state before returning `ERR`.

## Persistent configuration

The desired topology is stored in
`/var/lib/eswitch-management/eswitch.conf` by default. Override it with
`ESWITCH_STATE_FILE=/path/to/eswitch.conf`. On the first successful startup,
the daemon creates an empty versioned file. Every successful `vs create`,
`vs delete`, `vs port attach`, and `vs port detach` rewrites it atomically:

```text
write eswitch.conf.tmp.<pid>
  -> fflush + fsync
  -> rename over eswitch.conf
  -> fsync containing directory
```

The file stores stable port identities rather than transient DPDK port IDs:

```text
# eSwitch Management persistent state
version 1
vswitch 100
member 100 parent
member 100 representor 1 0 0
member 100 representor 1 0 1
```

At startup the daemon probes current ports first, loads this file, maps
`parent` or `(host,pf,vf)` to the current DPDK port ID, then recreates the
vSwitches and attachments. Startup fails instead of silently omitting a
configured port when an identity cannot be resolved or the file is invalid.
Learned dynamic FDB entries are not persisted and are relearned from traffic.
See [eswitch.conf.example](eswitch.conf.example) for a complete example.
Manual edits are read only during startup; stop the daemon before editing the
file, then start it again. While the daemon is running, use `eswitchctl` so
hardware state and the file are committed together.

## Build on the BlueField DOCA development container

```bash
cd /mnt/doca-dev/Sample-DOCA-Application/application/eswitch_management
meson setup /tmp/eswitch-management-build -Dvf_scope='0-6,10-20'
meson compile -C /tmp/eswitch-management-build
meson test -C /tmp/eswitch-management-build eswitch-state
```

After source-only changes, run only the `meson compile` command. Run
`meson setup --reconfigure /tmp/eswitch-management-build` after changing
`meson.build`. Change an existing build directory's scope without recreating
it, then compile again:

```bash
meson configure /tmp/eswitch-management-build -Dvf_scope='10-20'
meson compile -C /tmp/eswitch-management-build
```

## Multi-stage production container

The container build uses the DOCA 3.4 development image only for compiling.
The final image is based on `full-rt-3.4.0` and contains the two installed
binaries, without Meson, the compiler, headers, source tree or debugger.

Build on the BlueField so Docker selects the Arm64 variants of both NGC base
images. The build context must be the `Sample-DOCA-Application` root because
the eSwitch Management Meson project reuses source modules from sibling
directories:

```bash
cd /mnt/doca-dev/Sample-DOCA-Application
sudo docker build \
  --build-arg VF_SCOPE='0-6,10-20' \
  -f application/eswitch_management/Dockerfile \
  -t eswitch-management:3.4.0 .
```

`VF_SCOPE` limits which VF representors are opened and passed to
`doca_dpdk_port_probe_with_representors()`. It accepts `all`, one index such as
`4`, or comma-separated indexes/ranges such as `0-6,10-20`. The parent DPDK
port and exactly one Arm system SF representor are always probed. Startup fails
closed when no SF or more than one SF is discovered. The image stores this as
its default scope; a deployment may override it without rebuilding:

```bash
sudo docker run ... \
  --env ESWITCH_VF_SCOPE='10-20' \
  --env ESWITCH_SF_IFACE='enp3s0f0s0' \
  --env ESWITCH_HW_ROUTING='1' \
  --env ESWITCH_HW_CT='1' \
  --env ESWITCH_HW_CT_CAPACITY='4096' \
  --env ESWITCH_UPLINK_ARP_PPS='256' \
  --env ESWITCH_UPLINK_ARP_BURST='64' \
  eswitch-management:3.4.0 -l 0 -- 03:00.0
```

The persisted `eswitch.conf` must not contain member VFs outside the selected
scope. Such a configuration is rejected during restore instead of silently
dropping the missing membership.

The base images can be changed without editing the Dockerfile:

```bash
sudo docker build \
  --build-arg VF_SCOPE='0-6,10-20'
  -f application/eswitch_management/Dockerfile \
  -t eswitch-management:3.4.0 .
```

Create the host directories and start the production container without an
interactive shell:

```bash
sudo install -d -m 0755 /run/eswitch-management
sudo install -d -m 0750 /var/lib/eswitch-management
sudo ip link set dev enp3s0f0s0 up

sudo docker run -d \
  --name eswitch-management \
  --restart unless-stopped \
  --privileged \
  --network host \
  --ulimit memlock=-1:-1 \
  --mount type=bind,src=/dev/hugepages,dst=/dev/hugepages \
  --mount type=bind,src=/run/eswitch-management,dst=/run/eswitch-management \
  --mount type=bind,src=/var/lib/eswitch-management,dst=/var/lib/eswitch-management \
  eswitch-management:3.4.0 \
  -l 0 -- 03:00.0
```

`--network host` exposes the actual Arm SF netdev inside the container and
`--privileged` supplies the raw-packet capability required by `AF_PACKET`.
Override `ESWITCH_SF_IFACE` when the actual SF netdev name is not
`enp3s0f0s0`. Bring that interface UP before starting the daemon.

The `/run/eswitch-management` bind mount publishes only the Unix control
socket. The `/var/lib/eswitch-management` bind mount preserves `eswitch.conf`
when the container is removed and recreated. It must not be shared with a
second running eSwitch manager.

Run the CLI already included in the image:

```bash
sudo docker exec eswitch-management eswitchctl status
sudo docker exec eswitch-management eswitchctl port show
```

The status response includes cumulative SF return diagnostics. A zero
`sf_ingress_hits` means the packet did not match the system-SF root entry. If
that value increases while `sf_context_hits` remains zero, the packet reached
the SF root but did not match an active private context-VLAN entry. These counters
measure hardware Flow entries; `arp_sf_tx_sent` only measures successful
submission to the Arm raw socket. `local_ip_hits` counts IPv4 packets addressed
to a private RIF and delivered to the Arm handler; `icmp_sf_tx_sent` confirms
that a validated echo reply was submitted through the SF return path. The
`ipv4_routing=arm-lpm` line reports routed packets, route failures, neighbor
misses, ARP probes, and successful routed transmissions.

The `hw_routing_configured` status line reports active LPM entries,
promotions, in-place adjacency updates, removals, failures and LPM misses.
For a steady inter-VS flow, the egress hardware counter must increase while
`routed_seen` stops increasing. Verify rollout on the BF3 with:

```bash
# Baseline behavior is unchanged.
ESWITCH_HW_ROUTING=0 /build/eswitch-management/eswitch-management -l 0 -- 03:00.0

# After stopping the baseline instance, enable the experimental fast path.
ESWITCH_HW_ROUTING=1 ESWITCH_HW_CT=1 ESWITCH_HW_CT_CAPACITY=4096 \
  /build/eswitch-management/eswitch-management -l 0 -- 03:00.0
/build/eswitch-management/eswitchctl status | grep -E 'hw_routing|hw_ct|nat_|routed_seen'
/build/eswitch-management/eswitchctl tx-debug | grep -E 'egress_port|hw_routing'
```

Test gateway ARP/ICMP, bidirectional inter-VS ping and TCP/UDP, then NAT curl
and NAT ICMP. For TCP/UDP NAT, `nat_created` and `ct_promotions` must increase,
`ct_active` must remain non-zero, and `nat_out`/`nat_in` should stop increasing
for the established flow while traffic continues. The first packet uses Arm;
subsequent eligible packets use hardware. ICMP NAT counters continue to grow
because ICMP stays on Arm. Change a neighbor MAC, detach and
reattach a VF, edit a RIF MAC and remove a route; stale hardware forwarding
must disappear. Fragments, IPv4 options and TTL 1 must still reach the slow
path. Router configuration mutations conservatively flush all CT/NAT sessions.
This initial CT implementation disables hardware aging and counters; entries
are explicitly removed at configuration change and shutdown, so capacity must
cover the maximum concurrent TCP/UDP sessions between restarts. Do not enable
this flag in production until those hardware checks pass.

Inspect startup and health status with:

```bash
sudo docker logs -f eswitch-management
sudo docker inspect --format '{{.State.Health.Status}}' eswitch-management
```

`SIGTERM` is forwarded to the application by Docker, so normal `docker stop`
executes the existing DOCA Flow, DPDK port and device cleanup path.

## Run manually

```bash
sudo install -d -m 0750 /var/lib/eswitch-management
sudo ip link set dev enp3s0f0s0 up
sudo /tmp/eswitch-management-build/eswitch-management -l 0 -- 03:00.0
```

In another shell:

```bash
/tmp/eswitch-management-build/eswitchctl status
/tmp/eswitch-management-build/eswitchctl port show
/tmp/eswitch-management-build/eswitchctl vs create --id 10
/tmp/eswitch-management-build/eswitchctl vs port attach --id 10 --port 1
/tmp/eswitch-management-build/eswitchctl vs port attach --id 10 --port 2
/tmp/eswitch-management-build/eswitchctl vs show
/tmp/eswitch-management-build/eswitchctl fdb show
/tmp/eswitch-management-build/eswitchctl vs port detach --id 10 --port 2
/tmp/eswitch-management-build/eswitchctl vs delete --id 10
```

Example output:

```text
OK
DPDK port 0 (uplink/parent)
DPDK port 1 (host=1 pf=0 vf=0)
DPDK port 2 (host=1 pf=0 vf=1)
```

See [CLI.md](CLI.md) for the complete CLI and Unix-socket protocol contract.


## systemd

After `meson install`, copy the supplied unit and configuration:

```bash
sudo install -m 0644 eswitch-management.service /etc/systemd/system/
sudo install -m 0644 eswitch-management.conf.example \
  /etc/eswitch-management.conf
sudo systemctl daemon-reload
sudo systemctl enable --now eswitch-management
```

vSwitch topology and membership survive daemon/container/BlueField restart.
Dynamic FDB state remains runtime-only and is learned again after startup.
