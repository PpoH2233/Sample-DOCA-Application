# eswitchctl CLI and control protocol

This document is the command contract between a local control client and the
`eswitch-management` daemon. A future Python or CloudStack adapter should use
the Unix socket protocol directly. It does not need to start another DOCA or
DPDK process.

## Help

Help is local and works while the daemon is stopped:

```bash
eswitchctl --help
eswitchctl -h
```

## Connection

- Socket type: Unix domain `SOCK_STREAM`
- Default path: `/run/eswitch-management/control.sock`
- Override for development: `ESWITCH_CONTROL_SOCKET=/path/to/socket`
- Encoding: ASCII/UTF-8 text
- One command per connection
- Client terminates the request with `\n`
- Server writes the response and closes the connection

Example Python transport:

```python
import socket


def eswitch_request(command: str) -> str:
    with socket.socket(socket.AF_UNIX, socket.SOCK_STREAM) as client:
        client.connect("/run/eswitch-management/control.sock")
        client.sendall((command + "\n").encode())
        client.shutdown(socket.SHUT_WR)

        chunks = []
        while chunk := client.recv(16384):
            chunks.append(chunk)
    return b"".join(chunks).decode()
```

## Response envelope

Success always starts with:

```text
OK
```

Mutation commands normally return only that line. Query commands append zero
or more text lines.

Failure always starts with:

```text
ERR code=<doca_error_t integer> message=<DOCA error description>
```

An `ERR` means the caller must not assume the requested state. For a hardware
programming or rollback error, reconcile with `status`, `vs-list`, and
`list-port-available` before retrying.

`eswitchctl` exits with status `0` after an `OK` response or local `--help`.
It exits with status `1` for an `ERR` response, invalid local invocation,
connection failure, or transport failure.

## Client-side errors

`eswitchctl` validates command names, option names, argument counts, and
16-bit numeric values before opening the socket. Invalid input prints the full
help locally, even when the daemon is stopped:

```text
Invalid command or arguments.

Usage: eswitchctl <command> [arguments]
...
```

A raw socket client also receives command-specific syntax from the daemon:

```text
ERR code=2 message=Invalid input
Usage: vs-port-detach --id <id> --port <port-id>
```

When the socket path is missing or refuses the connection, `eswitchctl`
reports that the daemon may not be running:

```text
eSwitch Management control socket is not available: /run/eswitch-management/control.sock
The daemon may not be running. Check it with:
  systemctl status eswitch-management
```

## Commands

### `status`

Shows daemon health and object counts.

```bash
eswitchctl status
```

Example:

```text
OK
service=eSwitch Management state=running uptime=120s
config=/var/lib/eswitch-management/eswitch.conf
ports=7 assigned=3 available=4 vswitches=1 fdb=2
```

### `vs-create --id <id>`

Creates an empty virtual switch. Valid IDs are `1..65535`; `0` means no
assignment internally and is rejected.

```bash
eswitchctl vs-create --id 100
```

Creating an existing ID returns `ERR`.

### `vs-delete --id <id>`

Flushes the vSwitch FDB, removes classifier entries, destroys its flood group,
releases all member ports, and deletes the vSwitch.

```bash
eswitchctl vs-delete --id 100
```

### `vs-port-attach --id <vs-id> --port <port-id>`

Attaches one available DPDK port to a vSwitch. A port, including the uplink,
can belong to only one vSwitch.

```bash
eswitchctl vs-port-attach --id 100 --port 1
```

Successful attachment performs:

1. Add one member entry to the vSwitch flooding HASH pipe. Existing members
   and learned FDB rules are unchanged.
2. Add the root classifier entry that writes
   `(vswitch_id << 16) | ingress_port_id` to packet metadata.
3. Mark the port as owned by the vSwitch.

If the classifier operation fails, the newly added flood member is removed.

### `vs-port-detach --id <vs-id> --port <port-id>`

Detaches a member DPDK port and returns it to the available pool.

```bash
eswitchctl vs-port-detach --id 100 --port 1
```

Successful detachment performs:

1. Remove the port's root classifier entry, stopping new ingress.
2. Remove only that port's member entry from the flooding HASH pipe.
3. Remove only FDB entries whose learned egress is the detached port.
4. Mark the port available. Other members and their FDB entries are retained.

The vSwitch and its empty flood group continue to exist when its last port is
detached. With zero members unknown traffic is dropped; with one member its
egress gate drops a packet returning to the same ingress.

### `vs-list`

Lists all vSwitch IDs and their DPDK port membership.

```bash
eswitchctl vs-list
```

Example:

```text
OK
vs=100 ports=[0,1,2]
vs=200 ports=[3,4]
```

### `show-fdb [--id <vs-id>]`

Without an ID, shows learned entries from every vSwitch. Supplying an ID
filters the output.

```bash
eswitchctl show-fdb
eswitchctl show-fdb --id 100
```

Example:

```text
OK
FDB entries: total=2 filter-vs=selected
vs=100 mac=02:00:00:00:00:0a port=1 packets=42
```

### `list-port-available`

Lists unassigned DPDK ports.

```bash
eswitchctl list-port-available
```

Example:

```text
OK
DPDK port 0 (uplink/parent)
DPDK port 1 (host=1 pf=0 vf=0)
```

The Arm-side representor does not provide the x86 host's Linux interface name.
The stable `host/pf/vf` identity identifies which host VF the DPDK port
represents.

## Backward compatibility

The named-option form is the canonical syntax for new CLI and API clients.
Version 1 also accepts the original positional forms:

```bash
eswitchctl vs-create 100
eswitchctl vs-delete 100
eswitchctl vs-port-attach 100 1
eswitchctl vs-port-detach 100 1
eswitchctl show-fdb 100
```

For port commands, named options may appear in either order:

```bash
eswitchctl vs-port-detach --port 1 --id 100
```

## Runtime model

- `eswitch-management` is the only process that owns EAL, DOCA devices,
  representors, Flow ports, pipes, and entries.
- Commands are executed by the same owner loop as FDB learning; Flow mutations
  are serialized.
- One vSwitch can contain at most 254 ports in this implementation.
- vSwitch creation and membership are committed atomically to `eswitch.conf`
  after hardware programming. A save failure triggers a topology rollback and
  returns `ERR`.
- Restarting the daemon restores topology using stable parent or host/PF/VF
  identity; DPDK port IDs are never persisted.
- Learned FDB entries remain runtime-only and are relearned after restart.
- The current data plane supports one untagged bridge domain per vSwitch.
