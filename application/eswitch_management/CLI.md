# eswitchctl command contract

This document is the contract between a control client and the
`eswitch-management` daemon. It is written to be implementable directly against
the Unix control socket: a Python, CloudStack or other adapter does not need to
start another DOCA or DPDK process, and does not need to shell out to
`eswitchctl`.

Everything below is normative unless marked as an example.

Contract revision: `doca34-shared-vs-multivr-v28`. This revision includes
logical router-links, Arm router-link forwarding, NAT44 for TCP/UDP/ICMP Echo,
private DOCA Flow LPM promotion, TCP/UDP DOCA Flow CT promotion, and route-plan
aware control transactions.

## 1. Grammar

Commands are resource-first:

```text
<resource> [sub-resource] <action> [--option value ...]
```

The complete canonical command set:

| # | Command | Kind |
| --- | --- | --- |
| 1 | `status` | query |
| 2 | `tx-debug` | query |
| 3 | `vs create --id <id>` | mutation |
| 4 | `vs delete --id <id>` | mutation |
| 5 | `vs show [--id <id>]` | query |
| 6 | `vs port attach --id <id> --port <port-id> [--mode access\|trunk] [--vlan <1-4094>]` | mutation |
| 7 | `vs port detach --id <id> --port <port-id>` | mutation |
| 8 | `port show` | query |
| 9 | `fdb show [--id <id>]` | query |
| 10 | `vr create --id <id>` | mutation |
| 11 | `vr delete --id <id>` | mutation |
| 12 | `vr show --id <id>` | query |
| 13 | `vr port attach --id <id> --port <port-id> --name <name>` | mutation |
| 14 | `vr port detach --id <id> --interface <name>` | mutation |
| 15 | `vr switch attach --id <id> --switch-id <vs-id> --name <name>` | mutation |
| 16 | `vr switch detach --id <id> --interface <name>` | mutation |
| 17 | `vr interface set --id <id> --interface <name> --mac <mac>` | mutation |
| 18 | `vr ip add --id <id> --interface <name> --address <ip/prefix>` | mutation |
| 19 | `vr ip del --id <id> --interface <name> --address <ip/prefix>` | mutation |
| 20 | `vr route add --id <id> --prefix <cidr> --via <ip> --interface <name>` | mutation |
| 21 | `vr route del --id <id> --prefix <cidr>` | mutation |
| 22 | `vr route show --id <id>` | query |
| 23 | `vr nat enable --id <id> --interface <name> --address <interface\|ip> --port-range <first-last>` | mutation |
| 24 | `vr nat disable --id <id>` | mutation |
| 25 | `vr nat show --id <id>` | query |
| 26 | `link create --id <link-id>` | mutation |
| 27 | `link delete --id <link-id>` | mutation |
| 28 | `link show --id <link-id>` | query |
| 29 | `vr link attach --id <vr-id> --link-id <link-id> --name <name>` | mutation |
| 30 | `vr link detach --id <vr-id> --interface <name>` | mutation |

Grammar rules:

- Options are named, `--key value`, and each option may appear at most once.
- Option order is not significant.
- An unknown option, a duplicate option, a missing required option, an
  unexpected trailing token, or an unknown action is a syntax error.
- Canonical forms accept named options only. Positional values are accepted by
  the deprecated flat aliases in section 7, not by the canonical forms.
- Tokens are separated by spaces or tabs.

### 1.1 Argument types

| Type | Rule |
| --- | --- |
| `<id>` for `vs` | Decimal or `0x`-prefixed integer, `1..65535`. `0` means "unassigned" internally and is rejected by `vs create` and `vs delete`. |
| `<id>` for `vr` | Decimal integer, `1..65535`. `0` is rejected by the parser. |
| `<port-id>` | Runtime DPDK port ID, `0..65535`. IDs are assigned at probe time and are **not** stable across daemon restarts. Discover them with `port show`. |
| `<vs-id>` | An existing vSwitch ID. |
| `<link-id>` | Logical point-to-point router-link ID, `1..65535`. A link accepts exactly two endpoints in different VRs. |
| `<name>` | Router interface name, 1..31 characters from `[A-Za-z0-9_-]`. Unique within one VR. |
| `<mac>` | `xx:xx:xx:xx:xx:xx`, lowercase or uppercase hex. Must be unicast (low bit of the first octet clear) and non-zero. Must be unique across all RIFs. |
| `<ip/prefix>` | IPv4 host address with prefix length, for example `10.0.0.1/24`. Must be a usable unicast address: not `0.0.0.0/x`, not loopback, not multicast or reserved, not the network or broadcast address of its own prefix when the prefix is shorter than `/31`. |
| `<cidr>` | IPv4 prefix with zero host bits, for example `10.0.0.0/8` or `0.0.0.0/0`. |
| `<ip>` for `--via` | IPv4 next hop. Must be on-link for the named interface and not the interface's own address. |
| `<first-last>` | NAT port range. `first >= 1024` and `first <= last`. Applies to TCP and UDP ports and to ICMP Echo identifiers. |
| `--address interface` | For `vr nat enable` only: use the uplink RIF's own address. |

The `--name` option names a **new** interface, so it is used only by
`vr port attach`, `vr switch attach` and `vr link attach`. Every command that refers to an
**existing** interface uses `--interface`. Supplying the wrong one is a syntax
error.

Router-link endpoints use the Arm dataplane in phase 1. Both endpoint
addresses must be distinct members of the same prefix. A static route using a
router-link must name the peer endpoint address as `--via`; arbitrary neighbor
discovery and hardware LPM/CT promotion across a router-link are not performed.

## 2. Transport

- Socket type: Unix domain `SOCK_STREAM`.
- Default path: `/run/eswitch-management/control.sock`, mode `0660`.
- Override for development: `ESWITCH_CONTROL_SOCKET=/path/to/socket`.
- Encoding: ASCII/UTF-8 text.
- One command per connection. The client writes the command terminated by
  `\n`, then half-closes the write side. Bytes after the first `\n` are
  ignored.
- Maximum request line: 510 bytes of command text plus the `\n` terminator. A
  longer line is a syntax error, never a truncated command.
- The server writes the whole response and closes the connection. Response
  completion is signalled by EOF, not by a length prefix. A client must read
  until EOF; responses can exceed one `recv()`.
- Maximum response: 128 KiB. Output is truncated at that limit rather than
  split across connections.
- There is no authentication in the protocol. Access control is filesystem
  permissions on the socket, so the socket's group membership is the
  authorization boundary. Anything that can open the socket can reconfigure the
  data plane.

Reference client:

```python
import socket


def eswitch_request(command: str,
                    path: str = "/run/eswitch-management/control.sock") -> str:
    with socket.socket(socket.AF_UNIX, socket.SOCK_STREAM) as client:
        client.connect(path)
        client.sendall((command + "\n").encode())
        client.shutdown(socket.SHUT_WR)

        chunks = []
        while chunk := client.recv(16384):
            chunks.append(chunk)
    return b"".join(chunks).decode()


def eswitch_call(command: str) -> tuple[str, list[str]]:
    """Returns (status_line, payload_lines), or raises on ERR."""
    lines = eswitch_request(command).splitlines()
    if not lines or not lines[0].startswith(("OK", "ERR")):
        raise RuntimeError(f"malformed response: {lines!r}")
    if lines[0].startswith("ERR"):
        raise RuntimeError(lines[0])
    return lines[0], lines[1:]
```

## 3. Response envelope

Every response begins with a status token on the first line.

Success:

```text
OK
<zero or more payload lines>
```

The `vr` query commands carry attributes on the status line itself:

```text
OK vr=<id> dataplane=<state>
<zero or more payload lines>
```

The current router state token is
`ARM_LPM_ROUTER_LINK_NAT_MVP`. Treat it as an opaque capability/status string,
not as a stable enum: later releases may report more hardware offload stages.

Mutations return only the status line. `vr` mutations return a status line with
a dataplane note:

```text
OK configuration committed; private-vs-dataplane=ARM_LPM router-link-dataplane=ARM public-dataplane=ARM_NAT_MVP
```

Failure, for L2 commands and for the daemon-level checks:

```text
ERR code=<doca_error_t integer> message=<DOCA error description>
[Usage: <canonical form>]
```

Failure, for `vr` commands and for the two L2 policy errors listed in section
5.3:

```text
ERR <human readable reason>
```

Parsing rules for a client:

- Test the first three bytes. `OK` and `ERR` are the only status tokens.
- Payload lines are `key=value` pairs separated by single spaces, except
  `port show`, which is prose, and `vs show`, which uses a bracketed list.
- Preserve the whole first line. `vr` queries and `link show` place object
  attributes on the `OK ...` status line rather than in a payload line.
- Treat unknown keys as forward-compatible additions and ignore them.
- Never parse the `message=` text; branch on `code=` when present, otherwise on
  the command and the reason string.

`code=` is the numeric `doca_error_t`. The values this contract can return:

| Meaning | `doca_error_t` | HTTP analogue |
| --- | --- | --- |
| Invalid syntax, invalid argument value, or `vs` ID `0` | `DOCA_ERROR_INVALID_VALUE` | 400 |
| vSwitch, port or FDB object not found | `DOCA_ERROR_NOT_FOUND` | 404 |
| vSwitch ID already exists | `DOCA_ERROR_ALREADY_EXIST` | 409 |
| Port already owned by a vSwitch or reserved by a VR; vSwitch still bound to a VR | `DOCA_ERROR_IN_USE` | 409 |
| Port cannot be assigned, such as the reserved system SF | `DOCA_ERROR_NOT_SUPPORTED` | 409 |
| vSwitch or member capacity reached | `DOCA_ERROR_NO_MEMORY` | 507 |
| Hardware programming or persistence failure | driver-specific | 500 |

`vr` responses and the two policy errors in section 5.3 omit `code=` and carry a
reason string instead; `vr` before router initialization answers
`ERR router control is not initialized` (internally `DOCA_ERROR_BAD_STATE`).

An `ERR` without a `code=` field is a model-level rejection: the request was
well formed but the desired configuration is invalid.

### 3.1 Exit status of `eswitchctl`

`eswitchctl` is a thin transport over the same contract.

| Situation | Exit status |
| --- | --- |
| `OK` response | 0 |
| `--help` / `-h` | 0 |
| `ERR` response | 1 |
| Local syntax error, socket missing, connection or transport failure | 1 |

## 4. Client-side validation

`eswitchctl` validates the grammar of every command locally before it opens the
socket, so misuse is reported while the daemon is stopped:

```text
Invalid command or arguments.
Usage: vs port detach --id <id> --port <port-id>

Usage: eswitchctl <resource> [sub-resource] <action> [options]
...
```

The daemon applies the same grammar to raw-socket requests, so any command a
raw client can form is a command `eswitchctl` would also accept, and the
reverse. The daemon returns the canonical usage line even when the request used
a deprecated alias:

```text
ERR code=2 message=Invalid input
Usage: vs port detach --id <id> --port <port-id>
```

Local validation covers grammar only: command names, option names, option
counts, numeric ranges, IPv4 and MAC syntax, and the `vr` model constraints
that are independent of hardware. Existence, ownership and capacity are decided
by the daemon.

When the socket is missing or refuses the connection:

```text
eSwitch Management control socket is not available: /run/eswitch-management/control.sock
The daemon may not be running. Check it with:
  systemctl status eswitch-management
```

## 5. Commands

### 5.1 `status`

Daemon health and object counts.

```bash
eswitchctl status
```

```text
OK
service=eSwitch Management state=running uptime=120s
config=/var/lib/eswitch-management/eswitch.conf
ports=8 assignable=7 assigned=3 available=4 vswitches=1 fdb=2
```

`ports` includes the reserved system SF; `assignable` excludes it. Ports
reserved by a VR are counted as assigned and excluded from `available`.

### 5.2 `tx-debug`

The `status` output followed by cumulative hardware counters for the Arm/SF
return path. Intended for diagnosis, not for steady-state polling.

```bash
eswitchctl tx-debug
```

`destination_fdb_misses` counts packets that reached `ESW_DEST_FDB` without
matching a learned destination. Directed Arm-generated unicast bypasses that
lookup; broadcast ARP probes still use it. Each `egress_port` line counts
packets forwarded by that port's gate and packets rejected by its
split-horizon rule. Each `sf_context_tag` line identifies one private VLAN
context programmed for an Arm/SF return path, its vSwitch, whether it is
directed or flood, its target port, and its hit count.

Capture one sample before and one after a small known burst and compare
deltas. For directed traffic the selected context and its target port's
`forward_hits` must increase by the same count. A context hit without a
target-port hit localizes the failure to the context-to-egress chain; a root
hit without the expected context hit localizes it to the private VLAN
classifier.

The SF return classifier marks VLAN header 0 as valid before matching its TCI.
That declaration is required for the private context tag to participate in the
hardware match; filling `eth_vlan[0].tci` alone does not put the VLAN header in
the match format.

### 5.3 `vs create` / `vs delete`

```bash
eswitchctl vs create --id 100
eswitchctl vs delete --id 100
```

`vs create` creates an empty vSwitch. Creating an existing ID returns `ERR`.

`vs delete` flushes the vSwitch FDB, removes classifier entries, destroys its
flood group, releases all member ports, and deletes the vSwitch. Deleting a
vSwitch that a VR still uses is refused:

```text
ERR vSwitch is attached to a VR; detach it first with: vr switch detach --id <vr-id> --interface <name>
```

### 5.4 `vs port attach` / `vs port detach`

```bash
eswitchctl vs port attach --id 100 --port 1
eswitchctl vs port attach --id 32774 --port 0 --mode trunk --vlan 6
eswitchctl vs port detach --id 100 --port 1
```

The default is an untagged access membership. A trunk membership requires one
VLAN ID in the range 1-4094. A physical port may have multiple trunk
memberships, but a given `(port,VLAN)` belongs to exactly one vSwitch. Access
and trunk memberships cannot coexist on one port. A port reserved directly by
a VR cannot be attached.

`attach` performs, in order:

1. Add a root classifier entry. Access ingress must be untagged; trunk ingress
   matches VLAN ID and pops the 802.1Q header.
2. Write
   `(vswitch_id << 16) | ingress_port_id` into packet metadata.
3. Add a flood member through a per-membership egress gate. A trunk gate pushes
   the configured VLAN before forwarding to the physical port.

`detach` performs, in order:

1. Remove the port's root classifier entry, stopping new ingress.
2. Remove only that port's member entry from the flooding HASH pipe.
3. Remove only FDB entries whose learned egress is the detached port.
4. Mark the port available.

The vSwitch and its empty flood group survive detaching the last port. With
zero members, unknown traffic is dropped; with one member, its egress gate
drops a packet returning to its own ingress.

#### VLAN WAN on physical p0

This example creates VLAN 6 as a WAN broadcast domain on p0 (assume `port
show` reports the parent as DPDK port 0), attaches it to VR 101, and enables
outbound NAT. Frames are untagged inside VS 32774 and carry tag 6 only on the
physical wire:

```bash
eswitchctl vs create --id 32774
eswitchctl vs port attach --id 32774 --port 0 --mode trunk --vlan 6

eswitchctl vr switch attach --id 101 --switch-id 32774 --name uplink
eswitchctl vr interface set --id 101 --interface uplink \
  --mac 02:00:00:65:80:06
eswitchctl vr ip add --id 101 --interface uplink \
  --address 161.246.6.38/16
eswitchctl vr route add --id 101 --prefix 0.0.0.0/0 \
  --via 161.246.6.254 --interface uplink
eswitchctl vr nat enable --id 101 --interface uplink \
  --address interface --port-range 20000-60999
```

For another WAN VLAN on the same p0, create another VS and attach port 0 with
the other VLAN. Reusing `(port 0, VLAN 6)` in a second VS is rejected.

The same VLAN-6 vSwitch can be used as a shared WAN bridge by multiple VRs.
Each VR gets its own RIF MAC, address, default route, neighbor state, NAT zone,
and SF/hardware routing context:

```bash
eswitchctl vr create --id 1
eswitchctl vr switch attach --id 1 --switch-id 32774 --name wan6
eswitchctl vr interface set --id 1 --interface wan6 --mac 02:00:00:01:06:01
eswitchctl vr ip add --id 1 --interface wan6 --address 161.246.6.23/16
eswitchctl vr route add --id 1 --prefix 0.0.0.0/0 \
  --via 161.246.6.254 --interface wan6
eswitchctl vr nat enable --id 1 --interface wan6 \
  --address interface --port-range 20000-39999

eswitchctl vr create --id 2
eswitchctl vr switch attach --id 2 --switch-id 32774 --name wan6
eswitchctl vr interface set --id 2 --interface wan6 --mac 02:00:00:02:06:01
eswitchctl vr ip add --id 2 --interface wan6 --address 161.246.6.38/16
eswitchctl vr route add --id 2 --prefix 0.0.0.0/0 \
  --via 161.246.6.254 --interface wan6
eswitchctl vr nat enable --id 2 --interface wan6 \
  --address interface --port-range 40000-60999
```

Port ranges do not need to be disjoint when the VRs use distinct public IPs,
but separating them makes captures and operational debugging easier.

### 5.5 `vs show`

```bash
eswitchctl vs show
eswitchctl vs show --id 100
```

```text
OK
vs=100 ports=[1:access,2:access]
vs=32774 ports=[0:trunk/vlan=6]
```

Without `--id` this is a collection read: `OK` plus one line per vSwitch, or
`(empty)` when none exist. With `--id` it is a single-object read: `OK` plus
exactly one line, or `ERR code=<DOCA_ERROR_NOT_FOUND> ...` when that ID does
not exist. `--id 0` is treated as "no filter" and lists everything.

### 5.6 `port show`

Assignable DPDK ports. An access-owned port and a port directly reserved by a
VR are hidden. The physical parent remains visible after trunk attachment so
additional VLAN memberships can be configured on the same p0.

```bash
eswitchctl port show
```

```text
OK
DPDK port 0 (uplink/parent)
DPDK port 1 (host=1 pf=0 vf=0)
```

`(none)` is returned when nothing is available. The reserved system-SF
representor is intentionally omitted and can never be attached to a VS or a VR.

For external-host VFs the Arm-side representor does not expose the x86 host's
Linux interface name. The `host/pf/vf` triple is the stable identity of the
host VF a DPDK port represents; use it, not the DPDK port ID, as the key in an
external inventory.

### 5.7 `fdb show`

```bash
eswitchctl fdb show
eswitchctl fdb show --id 100
```

```text
OK
FDB entries: total=2 filter-vs=selected
vs=100 mac=02:00:00:00:00:0a port=1 packets=42
```

The FDB is a learned table, so a filter that selects nothing is an empty
result, not a lookup miss: an unknown `--id` returns `OK` with no entry lines.
`--id 0` means "no filter".

`packets` is the learned source-guard counter used for aging. It is not the
destination-FDB hit count. FDB contents are runtime-only and are relearned
after a restart.

### 5.8 `vr` commands

The router group is documented in [router/README.md](router/README.md).
Contract-level notes:

- `vr port attach` reserves an available VF representor as a public uplink RIF.
  The MVP permits one public uplink per VR.
- `vr switch attach` binds an existing vSwitch as a router RIF. A vSwitch is a
  shared L2 broadcast domain and may be attached by multiple VRs, or by
  multiple distinctly named interfaces in one VR. Destination RIF MAC/IP
  selects the VR routing zone. RIF MACs remain globally unique and duplicate
  IPv4 addresses on the same shared vSwitch are rejected.
- `vr show` lists the VR's named RIFs with their type, identity, MAC, address
  and readiness. `vr route show` lists connected and static routes.
  `vr nat show` reports the SNAT/PAT policy.
- Detach requires no dependencies: remove the NAT policy, the static routes and
  the IP address first. `vr delete` requires that the VR has no interfaces.
- One IPv4 address per interface, and overlapping subnets inside one VR are
  rejected.
- `vr nat enable` requires an addressed public port-link or vs-link that owns
  the VR's default route, and the public address must equal that interface's
  address. A VLAN WAN uses a vs-link whose vSwitch has p0 as a trunk member.

Implemented today: private-vSwitch gateway ARP, local ICMP echo, connected and
static route LPM, neighbor discovery, IPv4 forwarding, and Arm-side TCP/UDP/ICMP
Echo NAT with uplink ARP, all through the Arm system SF. Eligible private IPv4
routes can be promoted to the DOCA Flow hardware LPM fast path. When
`ESWITCH_HW_CT=1` is combined with `ESWITCH_HW_ROUTING=1`, established TCP/UDP
NAT sessions can be promoted to bidirectional DOCA Flow CT; misses and ICMP
remain on Arm. Unsupported or resource-constrained cases fail open to the Arm
slow path. Hardware CT aging/counters and ICMP routing error generation are not
implemented. Status strings
in `status`, `vr show` and `vr nat show` report the active/fallback stage, so a
client should surface them rather than assume full offload.

#### 5.8.1 VR and interface lifecycle

Create the VR first, then attach a backing object to create a named router
interface (RIF). The interface name is the stable key used by later commands.

```bash
eswitchctl vr create --id 1
eswitchctl vr switch attach --id 1 --switch-id 100 --name SW100
eswitchctl vr port attach --id 1 --port 7 --name uplink
eswitchctl vr interface set --id 1 --interface SW100 \
  --mac 02:00:00:01:00:01
eswitchctl vr ip add --id 1 --interface SW100 \
  --address 192.168.100.1/24
eswitchctl vr ip add --id 1 --interface uplink \
  --address 161.246.6.38/16
eswitchctl vr show --id 1
```

Representative response:

```text
OK vr=1 dataplane=ARM_LPM_ROUTER_LINK_NAT_MVP
SW100 ifindex=1 type=vs-link switch=100 mac=02:00:00:01:00:01 address=192.168.100.1/24 status=ACTIVE_ARM_LPM arp=PRIVATE_GATEWAY_ENABLED
uplink ifindex=2 type=port-link host=1 pf=0 vf=7 mac=02:00:00:01:00:02 address=161.246.6.38/16 status=ACTIVE_ARM_UPLINK arp=PUBLIC_RIF_ENABLED
```

The exact `ifindex`, host/PF/VF identity and default generated MAC are runtime
values. A REST implementation must parse keys, not compare whole lines.

#### 5.8.2 Logical router-links

A router-link is a logical point-to-point connection. Create it once, then
attach one endpoint to each of two different VRs:

```bash
eswitchctl link create --id 10
eswitchctl vr link attach --id 1 --link-id 10 --name r1-r2
eswitchctl vr link attach --id 2 --link-id 10 --name r2-r1
eswitchctl vr ip add --id 1 --interface r1-r2 --address 10.10.10.1/30
eswitchctl vr ip add --id 2 --interface r2-r1 --address 10.10.10.2/30
eswitchctl link show --id 10
```

```text
OK link=10 dataplane=ARM endpoints=vr1/r1-r2,vr2/r2-r1
```

The link accepts at most two endpoints, they must belong to different VRs,
and both addressed endpoints must use distinct addresses in the same prefix.
A route through the link must use the other endpoint address as its gateway.
Router-link forwarding is Arm slow path in this revision; it is deliberately
excluded from hardware LPM and CT promotion.

#### 5.8.3 Connected and static routes

Connected routes are derived from RIF addresses and cannot be added or deleted
directly. Static routes are keyed by `(vr, prefix)`:

```bash
eswitchctl vr route add --id 1 \
  --prefix 192.168.200.0/24 --via 10.10.10.2 --interface r1-r2
eswitchctl vr route add --id 1 \
  --prefix 0.0.0.0/0 --via 161.246.6.254 --interface uplink
eswitchctl vr route show --id 1
```

```text
OK vr=1 dataplane=ARM_LPM_ROUTER_LINK_NAT_MVP
connected 192.168.100.0/24 interface=SW100
connected 10.10.10.0/30 interface=r1-r2
connected 161.246.0.0/16 interface=uplink
static 192.168.200.0/24 via=10.10.10.2 ifindex=2
static 0.0.0.0/0 via=161.246.6.254 ifindex=3
```

`ifindex` is the RIF's daemon-assigned interface ID. Resolve it to the stable
interface name using `vr show --id <id>`.

Hardware transaction rules:

- A resolved route whose egress is a private vSwitch RIF may change the DOCA
  Flow LPM plan and therefore triggers hardware synchronization.
- Routes through an uplink/port RIF or a router-link are Arm-only and do not
  trigger an unchanged hardware LPM transaction. In particular, adding a
  default route through `uplink` must not fail merely because optional HWS
  action memory is exhausted.
- A failed hardware-relevant route transaction returns `ERR` and does not
  publish the candidate configuration. Runtime resource degradation is
  visible through `status` and falls back to Arm where supported.

#### 5.8.4 NAT44 policy

Configure the addressed uplink and its default route before enabling NAT:

```bash
eswitchctl vr nat enable --id 1 --interface uplink \
  --address interface --port-range 20000-60999
eswitchctl vr nat show --id 1
```

Representative response:

```text
OK vr=1 dataplane=ARM_LPM_ROUTER_LINK_NAT_MVP
nat=enabled mode=snat-pat protocols=tcp,udp interface=uplink address=161.246.6.38 ports=20000-60999 dataplane=ARM_ACTIVE hw-ct=PENDING
```

`--address interface` resolves to the current uplink RIF address. An explicit
address is accepted only when it equals that address. The policy performs
SNAT/PAT for TCP, UDP and ICMP Echo; TCP/UDP sessions are eligible for DOCA
Flow CT promotion, while ICMP Echo remains on the Arm NAT path.
The current `protocols=tcp,udp` field describes the port-bearing session types
eligible for CT promotion; ICMP Echo support is exposed by the NAT contract and
the `nat_icmp_echo_out`/`nat_icmp_echo_in` counters in `status`.

Disable NAT before deleting the default route, uplink address or uplink RIF:

```bash
eswitchctl vr nat disable --id 1
```

## 6. Idempotency and error semantics

No mutation command is idempotent. Every mutation is create-or-fail or
delete-or-fail; query commands are safe to repeat.

| Command | Repeating it on the same object |
| --- | --- |
| `vs create`, `vr create` | `ERR`, already exists |
| `link create` | `ERR`, already exists |
| `vs delete`, `vr delete` | `ERR`, not found |
| `link delete` | `ERR`, not found or endpoints remain attached |
| `vs port attach`, `vr port attach`, `vr link attach` | `ERR`, already attached or in use |
| `vr switch attach` | `ERR` only when the interface name already exists, the vSwitch is absent, or interface capacity is exhausted; sharing a vSwitch is supported |
| `vs port detach`, `vr port detach`, `vr switch detach`, `vr link detach` | `ERR`, not attached |
| `vr ip add`, `vr route add`, `vr nat enable` | `ERR`, already exists or already enabled |
| `vr ip del`, `vr route del`, `vr nat disable` | `ERR`, not found or not enabled |
| any query | Safe to repeat; no side effects |

Consequences for an API layer that must be idempotent:

- Build idempotency on top by reading first. `vs show --id <id>` distinguishes
  "exists" from "absent" with `OK` versus `NOT_FOUND`, which is enough to make
  a PUT-style handler idempotent without racing.
- Since the daemon serializes all commands in one loop, a read-then-write pair
  cannot interleave with another command's hardware programming, but it can
  interleave with another client's command. Treat `ALREADY_EXIST` on create and
  `NOT_FOUND` on delete as success when converging toward a desired state.
- Do not retry a failed mutation blindly. Re-read state first: an `ERR` from a
  hardware or persistence stage means the request was rolled back, but a
  `BAD_STATE` or transport failure leaves the outcome unknown until read back.

Reconciliation reads, in order: `status`, `vs show`, `port show`,
`vr show --id <id>` per known VR, `vr route show --id <id>`,
`vr nat show --id <id>`, and `link show --id <id>` per known link. There is no
collection-level `vr show` or `link show` command in this revision; the API
layer must retain the known VR and link IDs in its own desired-state store.

## 7. Canonical-to-legacy mapping

The flat verbs from version 1 remain accepted, unchanged, by both `eswitchctl`
and the daemon. **They are deprecated.** Help output and all examples use the
canonical forms; new clients must not emit aliases.

| Canonical | Deprecated alias |
| --- | --- |
| `vs create --id <id>` | `vs-create --id <id>`, `vs-create <id>` |
| `vs delete --id <id>` | `vs-delete --id <id>`, `vs-delete <id>` |
| `vs show` | `vs-list` |
| `vs show --id <id>` | `vs-list --id <id>` |
| `vs port attach --id <id> --port <p>` | `vs-port-attach --id <id> --port <p>`, `vs-port-attach <id> <p>` |
| `vs port detach --id <id> --port <p>` | `vs-port-detach --id <id> --port <p>`, `vs-port-detach <id> <p>` |
| `fdb show [--id <id>]` | `show-fdb [--id <id>]`, `show-fdb <id>` |
| `port show` | `list-port-available` |
| `vr port attach --id <id> --port <p> --name <n>` | `vr port-attach --id <id> --port <p> --name <n>` |
| `vr port detach --id <id> --interface <n>` | `vr port-detach --id <id> --interface <n>` |
| `vr switch attach --id <id> --switch-id <vs> --name <n>` | `vr switch-attach --id <id> --switch-id <vs> --name <n>` |
| `vr switch detach --id <id> --interface <n>` | `vr switch-detach --id <id> --interface <n>` |
| `vr show --id <id>` | `vr show-interface --id <id>` |

Alias behaviour is identical to the canonical command, including responses and
error codes, with two differences:

1. Deprecated aliases additionally accept the version-1 positional arguments
   listed above. Canonical forms require named options.
2. A syntax error on an alias is answered with the **canonical** usage line, so
   the error output guides migration.

`vs-list --id <id>` is an extension of the alias, not version-1 behaviour:
version 1 rejected any argument to `vs-list`. It exists so a client can migrate
the verb and the filter independently.

The public CLI uses canonical `vr port attach` and `vr switch attach` commands.
The internal `router-state 1` file deliberately keeps the version-1 hyphenated
spelling so a rollback to an older daemon remains possible. Loading accepts
both spellings, including files emitted by CLI-v2 development builds.

## 8. Atomicity and rollback

The daemon is the single owner of EAL, DOCA devices, representors, Flow ports,
pipes and entries. Commands run in the same loop as FDB learning, so Flow
mutations are serialized: there is no concurrent command execution and no
partial interleaving between two clients' commands.

L2 mutations are staged, programmed, then persisted:

1. Validate the request against the current model.
2. Program hardware. On a failure inside a multi-step sequence, the completed
   steps of that sequence are undone; for example, a failed classifier
   insertion removes the flood member added immediately before it.
3. Persist the topology to the state file with create-temp, `fsync`, `rename`,
   `fsync` on the directory.
4. Publish the in-memory model.

A persistence failure triggers a topology rollback and returns `ERR`.

`vr` and `link` mutations are transactional against a candidate
configuration:

1. The candidate is a full copy of the committed configuration. The parser and
   all model checks run on the candidate only, so a rejected command cannot
   modify committed state.
2. Existing hardware CT entries and the software NAT session table are flushed
   before a topology/routing change so no session can retain a stale zone,
   adjacency or egress decision.
3. Uplink port attach/detach is applied to the pipeline.
4. Changed private RIFs have their SF bindings invalidated so the next ARP
   rebuilds them from the committed configuration.
5. The current and candidate hardware-safe route plans are compared. Hardware
   routes are synchronized only when that plan changes; Arm-only uplink,
   default and router-link routes do not force an HWS transaction.
6. The candidate is persisted to `${ESWITCH_STATE_FILE}.router`.
7. Only then is the candidate published as the committed configuration.

If any of steps 3 through 6 fails, the daemon reverses the corresponding
topology and hardware-plan changes: hardware routes are resynchronized when
the plan changed, an added uplink is detached, and a removed uplink is
reattached. The response is `ERR` and committed configuration is unchanged.
Flushed CT/NAT sessions are ephemeral and cannot be restored; clients should
expect active connections to reconnect after any successful or attempted
router mutation that reaches the transaction stage.

The `rename` in step 6 is the commit point. After a successful `rename` the
daemon never rolls back only the in-memory copy, because that would diverge
from the on-disk state a restart would restore.

Restart behaviour:

- L2 topology restores first, then router state.
- Restore uses stable parent or host/PF/VF identity. DPDK port IDs are never
  persisted, so they may differ after a restart.
- Router restore fails if an attachment is missing, out of probe scope, or
  already owned by L2. It publishes nothing on failure.
- Learned FDB entries are runtime-only and are relearned.

## 9. API resource mapping

A REST or CloudStack adapter can map the grammar directly. The command's
resource path becomes the URL path, and `--id` becomes a path segment.

| Resource | Method and path | Command |
| --- | --- | --- |
| Service | `GET /status` | `status` |
| Service | `GET /diagnostics/tx` | `tx-debug` |
| vSwitch | `GET /vswitches` | `vs show` |
| vSwitch | `GET /vswitches/{id}` | `vs show --id {id}` |
| vSwitch | `POST /vswitches` `{"id":N}` | `vs create --id N` |
| vSwitch | `DELETE /vswitches/{id}` | `vs delete --id {id}` |
| vSwitch member | `PUT /vswitches/{id}/ports/{port}` `{"mode":"access"}` | `vs port attach --id {id} --port {port}` |
| vSwitch trunk member | `PUT /vswitches/{id}/ports/{port}` `{"mode":"trunk","vlan":6}` | `vs port attach --id {id} --port {port} --mode trunk --vlan 6` |
| vSwitch member | `DELETE /vswitches/{id}/ports/{port}` | `vs port detach --id {id} --port {port}` |
| Port | `GET /ports?assigned=false` | `port show` |
| FDB | `GET /fdb` | `fdb show` |
| FDB | `GET /fdb?vswitch={id}` | `fdb show --id {id}` |
| Router link | `POST /router-links` `{"id":L}` | `link create --id L` |
| Router link | `GET /router-links/{linkId}` | `link show --id {linkId}` |
| Router link | `DELETE /router-links/{linkId}` | `link delete --id {linkId}` |
| VR | `GET /routers/{id}` | `vr show --id {id}` |
| VR | `POST /routers` `{"id":N}` | `vr create --id N` |
| VR | `DELETE /routers/{id}` | `vr delete --id {id}` |
| VR uplink RIF | `PUT /routers/{id}/interfaces/{name}` `{"port":P}` | `vr port attach --id {id} --port P --name {name}` |
| VR uplink RIF | `DELETE /routers/{id}/interfaces/{name}` | `vr port detach --id {id} --interface {name}` |
| VR private RIF | `PUT /routers/{id}/interfaces/{name}` `{"vswitch":V}` | `vr switch attach --id {id} --switch-id V --name {name}` |
| VR private RIF | `DELETE /routers/{id}/interfaces/{name}` | `vr switch detach --id {id} --interface {name}` |
| VR router-link RIF | `PUT /routers/{id}/interfaces/{name}` `{"routerLink":L}` | `vr link attach --id {id} --link-id L --name {name}` |
| VR router-link RIF | `DELETE /routers/{id}/interfaces/{name}` | `vr link detach --id {id} --interface {name}` |
| RIF MAC | `PATCH /routers/{id}/interfaces/{name}` `{"mac":M}` | `vr interface set --id {id} --interface {name} --mac M` |
| RIF address | `PUT /routers/{id}/interfaces/{name}/addresses/{cidr}` | `vr ip add --id {id} --interface {name} --address {cidr}` |
| RIF address | `DELETE /routers/{id}/interfaces/{name}/addresses/{cidr}` | `vr ip del --id {id} --interface {name} --address {cidr}` |
| Route | `GET /routers/{id}/routes` | `vr route show --id {id}` |
| Route | `PUT /routers/{id}/routes/{cidr}` `{"via":G,"interface":N}` | `vr route add --id {id} --prefix {cidr} --via G --interface N` |
| Route | `DELETE /routers/{id}/routes/{cidr}` | `vr route del --id {id} --prefix {cidr}` |
| NAT | `GET /routers/{id}/nat` | `vr nat show --id {id}` |
| NAT | `PUT /routers/{id}/nat` `{"interface":N,"address":"interface","portFirst":20000,"portLast":60999}` | `vr nat enable --id {id} --interface N --address interface --port-range 20000-60999` |
| NAT | `DELETE /routers/{id}/nat` | `vr nat disable --id {id}` |

Mapping rules for an adapter:

- Translate `code=` to a status code with the table in section 3. An `ERR`
  without `code=` is a model rejection and maps to `409 Conflict` when it names
  a dependency or an existing object, otherwise `400 Bad Request`.
- Because mutation commands are not idempotent, a `PUT` handler must read
  before writing and must treat `ALREADY_EXIST` and `NOT_FOUND` as convergence,
  per section 6.
- Do not expose DPDK port IDs as durable resource identifiers. Key ports by
  `host/pf/vf` from `port show`, and resolve the DPDK ID immediately before
  issuing an attach.
- Router interface names are the stable, client-chosen keys for RIFs. The
  `ifindex` in `vr show` is daemon-assigned and changes when a RIF is
  recreated.
- The three interface `PUT` variants are a JSON `oneOf`: exactly one of
  `port`, `vswitch`, or `routerLink` must be present. For a generic interface
  `DELETE`, read the interface `type` first and issue the matching
  `vr port|switch|link detach` command.
- Serialize your own writes. The daemon serializes execution, but a client that
  fans out concurrent mutations cannot predict which one observes the
  pre-change state.

### 9.1 Request validation and command construction

Do not accept an arbitrary CLI string in the REST body. Decode a typed JSON
schema, validate it using the rules in section 1.1, then construct exactly one
canonical command. In particular:

- parse IDs and ports as integers, never as preformatted strings;
- validate interface names with `^[A-Za-z0-9_-]{1,31}$`;
- parse and canonicalize IP/CIDR values before interpolation;
- allow only the literal `interface` or a validated IPv4 address for the NAT
  address field;
- enforce `1024 <= portFirst <= portLast <= 65535`;
- reject newline, carriage return and whitespace in any scalar token.

Example REST request:

```http
PUT /routers/1/routes/0.0.0.0%2F0
Content-Type: application/json

{"via":"161.246.6.254","interface":"uplink"}
```

After validation, the adapter sends this line directly to the Unix socket:

```text
vr route add --id 1 --prefix 0.0.0.0/0 --via 161.246.6.254 --interface uplink\n
```

Success can be normalized as:

```http
HTTP/1.1 200 OK
Content-Type: application/json

{"routerId":1,"prefix":"0.0.0.0/0","via":"161.246.6.254","interface":"uplink"}
```

The adapter should read `vr route show --id 1` after mutation and return the
committed representation rather than translating the human-readable mutation
message into JSON.

### 9.2 Recommended REST mutation algorithm

For a desired-state `PUT` or `DELETE`:

1. Validate and canonicalize the HTTP input.
2. Read the owning object with `vr show`, `vr route show`, `vr nat show`,
   `vs show`, or `link show`.
3. If the desired state already exists, return the current representation
   without sending a mutation.
4. Send one canonical command through the Unix socket and read until EOF.
5. On `OK`, read back the object and return the committed state.
6. On `ERR`, re-read before deciding whether the operation converged. Map the
   error according to section 3; do not retry a hardware/resource error in a
   tight loop.
7. Apply a per-DPU write lock around steps 2 through 6. Reads may run without
   that adapter lock, but the daemon itself still processes them serially.

### 9.3 Parsing limitations relevant to an API implementation

- `port show` is prose. Parse `DPDK port <id> (host=<h> pf=<p> vf=<v>)` and
  store `host/pf/vf` as the durable identity; resolve the current DPDK ID on
  every attach.
- `vr route show` emits `ifindex`, not interface name, for static routes. Join
  it with the `ifindex` returned by `vr show`.
- `vs show` is the only collection read for configured topology. VR and
  router-link IDs must be retained by the REST service or its database.
- FDB and neighbor/NAT sessions are runtime state, not desired configuration.
  Do not recreate them through REST after restart.

## 10. Examples

Bridge two VFs, then route them:

```bash
eswitchctl port show

eswitchctl vs create --id 100
eswitchctl vs port attach --id 100 --port 1
eswitchctl vs port attach --id 100 --port 2
eswitchctl vs show --id 100
eswitchctl fdb show --id 100

eswitchctl vr create --id 1
eswitchctl vr switch attach --id 1 --switch-id 100 --name lan0
eswitchctl vr ip add --id 1 --interface lan0 --address 10.0.0.1/24
eswitchctl vr port attach --id 1 --port 0 --name wan0
eswitchctl vr ip add --id 1 --interface wan0 --address 203.0.113.2/24
eswitchctl vr route add --id 1 --prefix 0.0.0.0/0 --via 203.0.113.1 --interface wan0
eswitchctl vr nat enable --id 1 --interface wan0 --address interface \
  --port-range 20000-60999
eswitchctl vr show --id 1
```

Tear down in dependency order:

```bash
eswitchctl vr nat disable --id 1
eswitchctl vr route del --id 1 --prefix 0.0.0.0/0
eswitchctl vr ip del --id 1 --interface wan0 --address 203.0.113.2/24
eswitchctl vr port detach --id 1 --interface wan0
eswitchctl vr ip del --id 1 --interface lan0 --address 10.0.0.1/24
eswitchctl vr switch detach --id 1 --interface lan0
eswitchctl vr delete --id 1
eswitchctl vs port detach --id 100 --port 2
eswitchctl vs port detach --id 100 --port 1
eswitchctl vs delete --id 100
```

Local help works while the daemon is stopped:

```bash
eswitchctl --help
eswitchctl -h
```

Raw socket, no `eswitchctl`:

```bash
printf 'vs show --id 100\n' | socat - UNIX-CONNECT:/run/eswitch-management/control.sock
```

### 10.1 Two VRs connected by a router-link, with NAT on R1

This is the canonical acceptance topology for the REST adapter:

```text
VM1 -- VF/VS100 -- R1 -- 10.10.10.0/30 -- R2 -- VS99 -- VM2,VM3
                       |
                    uplink/NAT
                       |
                 161.246.6.254
```

The numeric DPDK port IDs below are examples. Resolve them from `port show` on
the current daemon instance.

```bash
# L2 domains and VM membership.
eswitchctl vs create --id 100
eswitchctl vs port attach --id 100 --port 1
eswitchctl vs create --id 99
eswitchctl vs port attach --id 99 --port 2
eswitchctl vs port attach --id 99 --port 3

# Routers and private gateway RIFs.
eswitchctl vr create --id 1
eswitchctl vr switch attach --id 1 --switch-id 100 --name SW100
eswitchctl vr interface set --id 1 --interface SW100 --mac 02:00:00:01:00:01
eswitchctl vr ip add --id 1 --interface SW100 --address 192.168.100.1/24

eswitchctl vr create --id 2
eswitchctl vr switch attach --id 2 --switch-id 99 --name SW99
eswitchctl vr interface set --id 2 --interface SW99 --mac 02:00:00:02:00:01
eswitchctl vr ip add --id 2 --interface SW99 --address 192.168.200.1/24

# Point-to-point router-link.
eswitchctl link create --id 10
eswitchctl vr link attach --id 1 --link-id 10 --name r1-r2
eswitchctl vr link attach --id 2 --link-id 10 --name r2-r1
eswitchctl vr interface set --id 1 --interface r1-r2 --mac 02:00:00:01:00:02
eswitchctl vr interface set --id 2 --interface r2-r1 --mac 02:00:00:02:00:02
eswitchctl vr ip add --id 1 --interface r1-r2 --address 10.10.10.1/30
eswitchctl vr ip add --id 2 --interface r2-r1 --address 10.10.10.2/30

# Routes between private networks.
eswitchctl vr route add --id 1 --prefix 192.168.200.0/24 \
  --via 10.10.10.2 --interface r1-r2
eswitchctl vr route add --id 2 --prefix 192.168.100.0/24 \
  --via 10.10.10.1 --interface r2-r1

# R1 public uplink, default route and NAT.
eswitchctl vr port attach --id 1 --port 7 --name uplink
eswitchctl vr interface set --id 1 --interface uplink --mac 02:00:00:01:00:03
eswitchctl vr ip add --id 1 --interface uplink --address 161.246.6.38/16
eswitchctl vr route add --id 1 --prefix 0.0.0.0/0 \
  --via 161.246.6.254 --interface uplink
eswitchctl vr nat enable --id 1 --interface uplink --address interface \
  --port-range 20000-60999

# R2 reaches the Internet through R1; R1 performs NAT at its uplink.
eswitchctl vr route add --id 2 --prefix 0.0.0.0/0 \
  --via 10.10.10.1 --interface r2-r1

# Read-back/acceptance checks.
eswitchctl vs show
eswitchctl link show --id 10
eswitchctl vr show --id 1
eswitchctl vr route show --id 1
eswitchctl vr nat show --id 1
eswitchctl vr show --id 2
eswitchctl vr route show --id 2
eswitchctl status
```

VM configuration used by the acceptance test:

```bash
# VM1
ip addr add 192.168.100.10/24 dev <vm-data-interface>
ip route replace default via 192.168.100.1 dev <vm-data-interface>

# VM2/VM3 use unique addresses from 192.168.200.0/24.
ip addr add 192.168.200.10/24 dev <vm-data-interface>
ip route replace default via 192.168.200.1 dev <vm-data-interface>
```

Acceptance checks are VM1-to-VM2 ping in both directions, ping to each local
gateway, and TCP/ICMP traffic from both private subnets through R1's NAT. The
first packet may be delayed by ARP/neighbor resolution; subsequent packets
should use the resolved Arm path and eligible private routes/sessions may be
promoted to hardware.

## 11. Data plane limits

- One untagged bridge domain per vSwitch.
- At most 254 ports per vSwitch.
- At most 64 vSwitches and 64 VRs.
- At most 128 logical router-links, exactly two endpoints per completed link,
  and at most 256 total RIFs across all VRs.
- At most 512 static routes. Router-link forwarding is limited to 8 logical
  hops as a loop guard.
- One public uplink RIF per VR, one IPv4 address per RIF, one NAT policy per VR.
- NAT is SNAT/PAT for TCP, UDP and ICMP Echo. The first packet is translated on
  Arm. With `ESWITCH_HW_CT=1`, TCP/UDP sessions are then installed atomically
  in both DOCA Flow CT directions; CT misses and ICMP stay on Arm.
- `ESWITCH_HW_CT_CAPACITY` is a power of two from 64 through 4096 (default
  4096), matching the software NAT table ceiling. This first implementation
  uses explicit flush at router mutation and shutdown and has no hardware aging
  or per-session CT counters.
