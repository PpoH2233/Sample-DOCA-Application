# eswitchctl command contract

This document is the contract between a control client and the
`eswitch-management` daemon. It is written to be implementable directly against
the Unix control socket: a Python, CloudStack or other adapter does not need to
start another DOCA or DPDK process, and does not need to shell out to
`eswitchctl`.

Everything below is normative unless marked as an example.

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
| 6 | `vs port attach --id <id> --port <port-id>` | mutation |
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
| `<name>` | Router interface name, 1..31 characters from `[A-Za-z0-9_-]`. Unique within one VR. |
| `<mac>` | `xx:xx:xx:xx:xx:xx`, lowercase or uppercase hex. Must be unicast (low bit of the first octet clear) and non-zero. Must be unique across all RIFs. |
| `<ip/prefix>` | IPv4 host address with prefix length, for example `10.0.0.1/24`. Must be a usable unicast address: not `0.0.0.0/x`, not loopback, not multicast or reserved, not the network or broadcast address of its own prefix when the prefix is shorter than `/31`. |
| `<cidr>` | IPv4 prefix with zero host bits, for example `10.0.0.0/8` or `0.0.0.0/0`. |
| `<ip>` for `--via` | IPv4 next hop. Must be on-link for the named interface and not the interface's own address. |
| `<first-last>` | NAT port range. `first >= 1024` and `first <= last`. Applies to TCP and UDP ports and to ICMP Echo identifiers. |
| `--address interface` | For `vr nat enable` only: use the uplink RIF's own address. |

The `--name` option names a **new** interface, so it is used only by
`vr port attach` and `vr switch attach`. Every command that refers to an
**existing** interface uses `--interface`. Supplying the wrong one is a syntax
error.

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


def eswitch_call(command: str) -> list[str]:
    """Returns the payload lines, or raises on ERR."""
    lines = eswitch_request(command).splitlines()
    if not lines or not lines[0].startswith(("OK", "ERR")):
        raise RuntimeError(f"malformed response: {lines!r}")
    if lines[0].startswith("ERR"):
        raise RuntimeError(lines[0])
    return lines[1:]
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

Mutations return only the status line. `vr` mutations return a status line with
a dataplane note:

```text
OK configuration committed; private-vs-dataplane=ARM_LPM public-dataplane=ARM_NAT_MVP
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
eswitchctl vs port detach --id 100 --port 1
```

A port, including the uplink, belongs to at most one vSwitch, and a port
reserved by a VR cannot be attached. One vSwitch holds at most 254 ports.

`attach` performs, in order:

1. Add one member entry to the vSwitch flooding HASH pipe. Existing members and
   learned FDB rules are untouched.
2. Add the root classifier entry that writes
   `(vswitch_id << 16) | ingress_port_id` into packet metadata.
3. Mark the port owned by the vSwitch.

`detach` performs, in order:

1. Remove the port's root classifier entry, stopping new ingress.
2. Remove only that port's member entry from the flooding HASH pipe.
3. Remove only FDB entries whose learned egress is the detached port.
4. Mark the port available.

The vSwitch and its empty flood group survive detaching the last port. With
zero members, unknown traffic is dropped; with one member, its egress gate
drops a packet returning to its own ingress.

### 5.5 `vs show`

```bash
eswitchctl vs show
eswitchctl vs show --id 100
```

```text
OK
vs=100 ports=[0,1,2]
vs=200 ports=[3,4]
```

Without `--id` this is a collection read: `OK` plus one line per vSwitch, or
`(empty)` when none exist. With `--id` it is a single-object read: `OK` plus
exactly one line, or `ERR code=<DOCA_ERROR_NOT_FOUND> ...` when that ID does
not exist. `--id 0` is treated as "no filter" and lists everything.

### 5.6 `port show`

Unassigned DPDK ports, meaning ports owned by neither a vSwitch nor a VR.

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
- `vr switch attach` binds one existing vSwitch as a private gateway RIF. A
  vSwitch belongs to at most one VR.
- `vr show` lists the VR's named RIFs with their type, identity, MAC, address
  and readiness. `vr route show` lists connected and static routes.
  `vr nat show` reports the SNAT/PAT policy.
- Detach requires no dependencies: remove the NAT policy, the static routes and
  the IP address first. `vr delete` requires that the VR has no interfaces.
- One IPv4 address per interface, and overlapping subnets inside one VR are
  rejected.
- `vr nat enable` requires an addressed public port-link that owns the VR's
  default route, and the public address must equal that interface's address.

Implemented today: private-vSwitch gateway ARP, local ICMP echo, connected and
static route LPM, neighbor discovery, IPv4 forwarding, and Arm-side TCP/UDP/ICMP
Echo NAT with uplink ARP, all through the Arm system SF. Eligible private IPv4
routes can be promoted to the DOCA Flow hardware LPM fast path; unsupported or
resource-constrained cases fail open to the Arm slow path. Not implemented:
hardware connection tracking and ICMP routing error generation. Status strings
in `status`, `vr show` and `vr nat show` report the active/fallback stage, so a
client should surface them rather than assume full offload.

## 6. Idempotency and error semantics

No command is idempotent. Every mutation is create-or-fail, delete-or-fail.

| Command | Repeating it on the same object |
| --- | --- |
| `vs create`, `vr create` | `ERR`, already exists |
| `vs delete`, `vr delete` | `ERR`, not found |
| `vs port attach`, `vr port attach`, `vr switch attach` | `ERR`, already attached or in use |
| `vs port detach`, `vr port detach`, `vr switch detach` | `ERR`, not attached |
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

Reconciliation reads, in order: `status`, `vs show`, `port show`, and
`vr show --id <id>` per known VR.

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

`vr` mutations are transactional against a candidate configuration:

1. The candidate is a full copy of the committed configuration. The parser and
   all model checks run on the candidate only, so a rejected command cannot
   modify committed state.
2. Uplink port attach/detach is applied to the pipeline.
3. Changed private RIFs have their SF bindings invalidated so the next ARP
   rebuilds them from the committed configuration.
4. Hardware routes are synchronized.
5. The candidate is persisted to `${ESWITCH_STATE_FILE}.router`.
6. Only then is the candidate published as the committed configuration, and
   stale NAT sessions for removed policies are flushed.

If any of steps 2 through 5 fails, the daemon reverses that step and the ones
before it: hardware routes are resynchronized from the committed configuration,
an added uplink is detached, and a removed uplink is reattached. The response is
`ERR` and committed state is unchanged.

The `rename` in step 5 is the commit point. After a successful `rename` the
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
| vSwitch member | `PUT /vswitches/{id}/ports/{port}` | `vs port attach --id {id} --port {port}` |
| vSwitch member | `DELETE /vswitches/{id}/ports/{port}` | `vs port detach --id {id} --port {port}` |
| Port | `GET /ports?assigned=false` | `port show` |
| FDB | `GET /fdb` | `fdb show` |
| FDB | `GET /fdb?vswitch={id}` | `fdb show --id {id}` |
| VR | `GET /routers/{id}` | `vr show --id {id}` |
| VR | `POST /routers` `{"id":N}` | `vr create --id N` |
| VR | `DELETE /routers/{id}` | `vr delete --id {id}` |
| VR uplink RIF | `PUT /routers/{id}/interfaces/{name}` `{"port":P}` | `vr port attach --id {id} --port P --name {name}` |
| VR uplink RIF | `DELETE /routers/{id}/interfaces/{name}` | `vr port detach --id {id} --interface {name}` |
| VR private RIF | `PUT /routers/{id}/interfaces/{name}` `{"vswitch":V}` | `vr switch attach --id {id} --switch-id V --name {name}` |
| VR private RIF | `DELETE /routers/{id}/interfaces/{name}` | `vr switch detach --id {id} --interface {name}` |
| RIF MAC | `PATCH /routers/{id}/interfaces/{name}` `{"mac":M}` | `vr interface set --id {id} --interface {name} --mac M` |
| RIF address | `PUT /routers/{id}/interfaces/{name}/addresses/{cidr}` | `vr ip add --id {id} --interface {name} --address {cidr}` |
| RIF address | `DELETE /routers/{id}/interfaces/{name}/addresses/{cidr}` | `vr ip del --id {id} --interface {name} --address {cidr}` |
| Route | `GET /routers/{id}/routes` | `vr route show --id {id}` |
| Route | `PUT /routers/{id}/routes/{cidr}` `{"via":G,"interface":N}` | `vr route add --id {id} --prefix {cidr} --via G --interface N` |
| Route | `DELETE /routers/{id}/routes/{cidr}` | `vr route del --id {id} --prefix {cidr}` |
| NAT | `GET /routers/{id}/nat` | `vr nat show --id {id}` |
| NAT | `PUT /routers/{id}/nat` | `vr nat enable --id {id} ...` |
| NAT | `DELETE /routers/{id}/nat` | `vr nat disable --id {id}` |

Mapping rules for an adapter:

- Translate `code=` to a status code with the table in section 3. An `ERR`
  without `code=` is a model rejection and maps to `409 Conflict` when it names
  a dependency or an existing object, otherwise `400 Bad Request`.
- Because no command is idempotent, a `PUT` handler must read before writing
  and must treat `ALREADY_EXIST` and `NOT_FOUND` as convergence, per section 6.
- Do not expose DPDK port IDs as durable resource identifiers. Key ports by
  `host/pf/vf` from `port show`, and resolve the DPDK ID immediately before
  issuing an attach.
- Router interface names are the stable, client-chosen keys for RIFs. The
  `ifindex` in `vr show` is daemon-assigned and changes when a RIF is
  recreated.
- Serialize your own writes. The daemon serializes execution, but a client that
  fans out concurrent mutations cannot predict which one observes the
  pre-change state.

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

## 11. Data plane limits

- One untagged bridge domain per vSwitch.
- At most 254 ports per vSwitch.
- At most 64 vSwitches and 64 VRs.
- One public uplink RIF per VR, one IPv4 address per RIF, one NAT policy per VR.
- NAT is SNAT/PAT for TCP, UDP and ICMP Echo, executed on the Arm cores.
  Hardware connection tracking is not initialized; the daemon probes
  `doca_flow_ct_cap_is_dev_supported()` and reports `hw_ct_state` but keeps it
  `NOT_INITIALIZED` until both directions can be installed atomically.
