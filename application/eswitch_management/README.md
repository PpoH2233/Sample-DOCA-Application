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

## Run manually

```bash
sudo /tmp/eswitch-management-build/eswitch-management -l 0 -- 03:00.0
```

In another shell:

```bash
/tmp/eswitch-management-build/eswitchctl status
/tmp/eswitch-management-build/eswitchctl list-port-available
/tmp/eswitch-management-build/eswitchctl vs-create 10
/tmp/eswitch-management-build/eswitchctl vs-port-attach 10 1
/tmp/eswitch-management-build/eswitchctl vs-port-attach 10 2
/tmp/eswitch-management-build/eswitchctl vs-list
/tmp/eswitch-management-build/eswitchctl show_fdb
/tmp/eswitch-management-build/eswitchctl vs-delete 10
```

Example output:

```text
OK
DPDK port 0 (uplink/parent)
DPDK port 1 (host=1 pf=0 vf=0)
DPDK port 2 (host=1 pf=0 vf=1)
```


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
