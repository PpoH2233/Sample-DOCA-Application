# eSwitch Management

`eswitch-management` is the single owner of the BlueField eSwitch, DOCA Flow
runtime, parent device and all VF representors. `eswitchctl` sends local control
commands over `/run/eswitch-management/control.sock`; it never initializes
DPDK or DOCA itself.

At startup every discovered DPDK port is **unassigned**. The root pipe has a
DROP miss action, so an unassigned VF or uplink cannot exchange traffic through
this application. A port belongs to at most one virtual switch.

## Data path

```text
endpoint
   -> root classifier: physical ingress port
      -> write pkt_meta = (vswitch_id << 16) | ingress_port_id
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
the daemon creates an empty versioned file. Every successful `vs-create`,
`vs-delete`, `vs-port-attach`, and `vs-port-detach` rewrites it atomically:

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
meson setup /tmp/eswitch-management-build
meson compile -C /tmp/eswitch-management-build
meson test -C /tmp/eswitch-management-build eswitch-state
```

After source-only changes, run only the `meson compile` command. Run
`meson setup --reconfigure /tmp/eswitch-management-build` after changing
`meson.build`.

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
  -f application/eswitch_management/Dockerfile \
  -t eswitch-management:3.4.0 .
```

The base images can be changed without editing the Dockerfile:

```bash
sudo docker build \
  --build-arg DOCA_DEVEL_IMAGE=nvcr.io/nvidia/doca/doca:devel-3.4.0 \
  --build-arg DOCA_RUNTIME_IMAGE=nvcr.io/nvidia/doca/doca:full-rt-3.4.0 \
  -f application/eswitch_management/Dockerfile \
  -t eswitch-management:3.4.0 .
```

Create the host directories and start the production container without an
interactive shell:

```bash
sudo install -d -m 0755 /run/eswitch-management
sudo install -d -m 0750 /var/lib/eswitch-management

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

The `/run/eswitch-management` bind mount publishes only the Unix control
socket. The `/var/lib/eswitch-management` bind mount preserves `eswitch.conf`
when the container is removed and recreated. It must not be shared with a
second running eSwitch manager.

Run the CLI already included in the image:

```bash
sudo docker exec eswitch-management eswitchctl status
sudo docker exec eswitch-management eswitchctl list-port-available
```

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
sudo /tmp/eswitch-management-build/eswitch-management -l 0 -- 03:00.0
```

In another shell:

```bash
/tmp/eswitch-management-build/eswitchctl status
/tmp/eswitch-management-build/eswitchctl list-port-available
/tmp/eswitch-management-build/eswitchctl vs-create --id 10
/tmp/eswitch-management-build/eswitchctl vs-port-attach --id 10 --port 1
/tmp/eswitch-management-build/eswitchctl vs-port-attach --id 10 --port 2
/tmp/eswitch-management-build/eswitchctl vs-list
/tmp/eswitch-management-build/eswitchctl show-fdb
/tmp/eswitch-management-build/eswitchctl vs-port-detach --id 10 --port 2
/tmp/eswitch-management-build/eswitchctl vs-delete --id 10
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
