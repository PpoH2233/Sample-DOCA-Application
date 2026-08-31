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
      -> destination miss -> per-vSwitch/per-ingress hardware flood group
root miss -> DROP
```

The Arm copy learns `(vswitch_id, untagged VLAN 0, source MAC)`. Membership
changes flush that vSwitch's FDB and rebuild its flood paths before opening a
new ingress classifier entry. This deliberately favors safe isolation over a
hitless topology update in version 1.

## Build on the BlueField DOCA development container

```bash
cd /mnt/doca-dev/Sample-DOCA-Application/application/eswitch_management
meson setup /tmp/eswitch-management-build
meson compile -C /tmp/eswitch-management-build
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

sudo docker run -d \
  --name eswitch-management \
  --restart unless-stopped \
  --privileged \
  --network host \
  --ulimit memlock=-1:-1 \
  --mount type=bind,src=/dev/hugepages,dst=/dev/hugepages \
  --mount type=bind,src=/run/eswitch-management,dst=/run/eswitch-management \
  eswitch-management:3.4.0 \
  -l 0 -- 03:00.0
```

The `/run/eswitch-management` bind mount publishes only the Unix control
socket. It allows a host-side client or another local container to use the
control plane without sharing ownership of the DPDK/DOCA devices.

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

Runtime vSwitch membership and learned FDB state are intentionally not
persistent in this first version. A daemon restart returns all ports to DROP.
