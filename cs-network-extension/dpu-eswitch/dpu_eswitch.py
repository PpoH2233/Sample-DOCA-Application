#!/usr/bin/env python3
"""Apache-2.0. CloudStack desired-state adapter; Python standard library only.

The daemon remains the sole DOCA owner. Mutations are serialized across the
DPU, not just a network, because VS6 and the guest VF pool are shared.
"""
import contextlib
from datetime import datetime, timezone
import fcntl
import ipaddress as ip
import json
import os
from pathlib import Path
import re
import shutil
import socket
import subprocess
import sys
import tempfile


class Error(Exception):
    pass


def check(value, message):
    if not value:
        raise Error(message)


def number(value, low, high, field):
    check(not isinstance(value, bool) and re.fullmatch(r"[0-9]+", str(value)), f"Invalid {field}")
    value = int(value)
    check(low <= value <= high, f"{field} must be {low}..{high}")
    return value


def address(value):
    return str(ip.IPv4Address(value))


def network(value):
    return ip.IPv4Network(value, strict=False)


def boolean(value):
    check(type(value) is bool or value in ("true", "false"), "Expected boolean")
    return value is True or value == "true"


def ports(value):
    match = re.fullmatch(r"([0-9]+)(?:[:-]([0-9]+))?", str(value))
    check(match, f"Invalid port span {value}")
    first = number(match[1], 1, 65535, "port")
    last = number(match[2] or match[1], first, 65535, "port")
    return first, last


def port_text(value):
    first, last = ports(value)
    return str(first) if first == last else f"{first}-{last}"


def read(path, default):
    return json.loads(path.read_text()) if path.exists() else default


def write(path, value):
    path.parent.mkdir(parents=True, exist_ok=True)
    fd, name = tempfile.mkstemp(prefix=".desired-", dir=path.parent)
    try:
        with os.fdopen(fd, "w") as stream:
            json.dump(value, stream, sort_keys=True)
            stream.flush()
            os.fsync(stream.fileno())
        os.replace(name, path)
        fd = os.open(path.parent, os.O_RDONLY | os.O_DIRECTORY)
        try:
            os.fsync(fd)
        finally:
            os.close(fd)
    finally:
        if os.path.exists(name):
            os.unlink(name)


def fields(line):
    return dict(re.findall(r"([\w-]+)=([^\s]+)", line))


def audit(command, network_id, outcome):
    path = Path(os.environ.get("DPU_ESWITCH_LOG_FILE", "/var/log/cloudstack/extensions/dpu-eswitch/dpu-eswitch.log"))
    # Log callback identity/outcome only, never the payload/SSH credentials.
    with contextlib.suppress(OSError):
        path.parent.mkdir(parents=True, exist_ok=True)
        with path.open("a") as stream:
            stream.write(f"{datetime.now(timezone.utc).isoformat()} command={command!r} network={network_id!r} result={outcome!r}\n")


class Control:
    def __init__(self, details, timeout=60):
        self.path = os.environ.get("ESWITCH_CONTROL_SOCKET") or details.get("control.socket", "/run/eswitch-management/control.sock")
        self.binary = os.environ.get("ESWITCHCTL") or details.get("eswitchctl.path") or shutil.which("eswitchctl")
        self.timeout = max(5, float(timeout))

    def run(self, command):
        check(len(command.encode()) <= 510 and "\n" not in command, "Invalid control command length")
        try:
            if self.binary:
                result = subprocess.run([self.binary, *command.split()], text=True, capture_output=True,
                                        timeout=self.timeout, env=dict(os.environ, ESWITCH_CONTROL_SOCKET=self.path))
                check(result.returncode == 0, result.stdout.strip() or result.stderr.strip())
                output = result.stdout
            else:
                with socket.socket(socket.AF_UNIX, socket.SOCK_STREAM) as client:
                    client.settimeout(self.timeout)
                    client.connect(self.path)
                    client.sendall((command + "\n").encode())
                    client.shutdown(socket.SHUT_WR)
                    chunks = []
                    while chunk := client.recv(16384):
                        chunks.append(chunk)
                    output = b"".join(chunks).decode()
            check(output.startswith("OK"), f"{command}: {output.strip()}")
            return output
        except (OSError, subprocess.SubprocessError) as exc:
            raise Error(f"Daemon not reachable on {self.path}: {exc}") from exc


class Adapter:
    def __init__(self, envelope, control=None):
        check(isinstance(envelope, dict), "Expected JSON object")
        self.p = envelope.get("payload", {})
        self.d = envelope.get("physical-network-extension-details", {})
        self.e = envelope.get("network-extension-details", {})
        check(all(isinstance(x, dict) for x in (self.p, self.d, self.e)), "Invalid payload envelope")
        self.root = Path(os.environ.get("DPU_ESWITCH_STATE_DIR", "/var/lib/cloudstack/dpu-eswitch"))
        self.ctl = control or Control(self.d)
        self.n = str(self.p.get("network_id", ""))
        self.cache = {}

    def identify(self):
        number(self.n, 1, 2**63-1, "network_id")
        check(not self.p.get("vpc_id"), "VPC is unsupported")
        check(str(self.p.get("guest_type", "isolated")).lower() == "isolated", "Only isolated networks supported")
        self.file = self.root / f"network-{self.n}" / "desired.json"
        self.cache = read(self.file, {})
        raw = self.p.get("vlan") or self.e.get("vlan") or self.cache.get("vlan")
        self.v = number(str(raw).removeprefix("vlan://"), 900, 999, "guest VID")
        check(not self.cache or self.cache.get("vlan") == self.v, "Network VID changed; destroy its old identity first")
        for key in ("vr_id", "vswitch_id"):
            check(not self.e.get(key) or str(self.e[key]) == str(self.v), f"{key} differs from VID")
        for other in self.root.glob("network-*/desired.json"):
            check(other == self.file or read(other, {}).get("vlan") != self.v, f"VID {self.v} already allocated")
        self.cache.update(network_id=self.n, vlan=self.v)
        for key in ("gateway", "cidr"):
            if self.p.get(key):
                check(not self.cache.get(key) or self.cache[key] == self.p[key], f"{key} changed; migrate network first")
                self.cache[key] = self.p[key]

    def inventory(self):
        output = self.ctl.run("port show --all")
        parent = re.findall(r"^DPDK port (\d+) \(uplink/parent\)", output, re.M)
        check(len(parent) == 1, "Cannot resolve unique p0")
        identity = (int(self.d.get("vf.host", 1)), int(self.d.get("vf.pf", 0)))
        vfs = {int(vf): int(port) for port, host, pf, vf in re.findall(
            r"^DPDK port (\d+) \(host=(\d+) pf=(\d+) vf=(\d+)\)", output, re.M)
            if (int(host), int(pf)) == identity}
        return int(parent[0]), vfs

    def switches(self):
        return {int(v): [x for x in members.split(",") if x] for v, members in re.findall(
            r"vs=(\d+)[^\n]*?ports=\[([^]]*)\]", self.ctl.run("vs show"))}

    def ensure_vs(self, vs):
        if vs not in self.switches():
            self.ctl.run(f"vs create --id {vs} --name VS{vs}")

    def attach(self, vs, port, trunk=False):
        descriptor = f"{port}:trunk/vlan=6" if trunk else f"{port}:access"
        for other, members in self.switches().items():
            for member in members:
                if member.split(":")[0] == str(port):
                    if other == vs and member == descriptor:
                        return
                    raise Error(f"Port {port} is owned by VS{other} ({member}); migration required")
        self.ctl.run(f"vs port attach --id {vs} --port {port}" +
                     (" --mode trunk --vlan 6" if trunk else " --mode access"))

    def allocations(self):
        legacy = self.root / "vf-alloc.txt"
        check(not legacy.exists() or not legacy.read_text().strip(), "Migrate legacy VF allocations before deployment")
        return read(self.root / "vf-alloc.json", {})

    def infrastructure(self):
        parent, vfs = self.inventory()
        check(0 in vfs, "Host VF0 is not probed")
        check(all(a["vf"] != 0 for a in self.allocations().values()), "VF0 is allocated to a guest")
        # Both ownership checks precede any mutation.
        for port in (parent, vfs[0]):
            for vs, members in self.switches().items():
                for member in members:
                    if member.split(":")[0] == str(port):
                        check(vs == 6 and member == f"{port}:trunk/vlan=6", f"VS6 blocked by VS{vs}: {member}")
        self.ensure_vs(6)
        self.attach(6, parent, True)
        self.attach(6, vfs[0], True)

    def vr(self, optional=False):
        try:
            return self.ctl.run(f"vr show --id {self.v}")
        except Error as exc:
            if optional and re.search(r"(?:VR|router).*not found", str(exc), re.I):
                return ""
            raise

    def rif(self, name):
        return next((fields(line) for line in self.vr().splitlines() if line.startswith(name + " ")), {})

    def ensure_rif(self, name, vs, mac, addr):
        current = self.rif(name)
        if not current:
            self.ctl.run(f"vr switch attach --id {self.v} --switch-id {vs} --name {name}")
            current = self.rif(name)
        check(current.get("type") == "vs-link" and current.get("switch") == str(vs), f"Wrong attachment for {name}")
        if current.get("mac") != mac:
            self.ctl.run(f"vr interface set --id {self.v} --interface {name} --mac {mac}")
        if current.get("address") != addr:
            check(current.get("address", "-") == "-", f"{name} already owns another address")
            self.ctl.run(f"vr ip add --id {self.v} --interface {name} --address {addr}")

    def private(self):
        gateway = address(self.cache.get("gateway"))
        net = network(self.cache.get("cidr"))
        check(ip.IPv4Address(gateway) in net, "Gateway outside guest CIDR")
        self.ensure_vs(self.v)
        if not self.vr(True):
            self.ctl.run(f"vr create --id {self.v}")
        self.ensure_rif(f"SW{self.v}", self.v, f"02:00:00:65:{self.v >> 8:02x}:{self.v & 255:02x}", f"{gateway}/{net.prefixlen}")
        _, vfs = self.inventory()
        for allocation in self.allocations().values():
            if allocation["network"] == self.n:
                check(allocation["vf"] in vfs, "Allocated VF missing")
                self.attach(self.v, vfs[allocation["vf"]])

    def public(self, desired=None):
        desired = desired or self.cache.get("public")
        check(desired, "No SNAT/public IP assigned")
        check(str(desired.get("public_vlan")).removeprefix("vlan://") == "6", "Public VID must be 6")
        addr, gateway = address(desired.get("public_ip")), address(desired.get("public_gateway"))
        net = network(desired.get("public_cidr"))
        check(ip.IPv4Address(addr) in net and ip.IPv4Address(gateway) in net, "Public IP/gateway outside CIDR")
        check(not self.cache.get("public") or self.cache["public"] == desired, "Only one public IP per VR; release old IP first")
        mac = f"02:00:00:66:{self.v >> 8:02x}:{self.v & 255:02x}"
        state = Path(self.d.get("router.state", "/var/lib/eswitch-management/eswitch.conf.router"))
        check(state.is_file(), "Router state unavailable: cannot verify duplicate public IP/MAC")
        for line in state.read_text().splitlines():
            owner = re.search(r"--id (\d+)", line)
            if owner and int(owner[1]) != self.v:
                check(f"--address {addr}/" not in line and f"--mac {mac}" not in line, f"Public IP/MAC owned by VR{owner[1]}")
        for other in self.root.glob("network-*/desired.json"):
            check(other == self.file or read(other, {}).get("public", {}).get("public_ip") != addr,
                  "Public IP already allocated to another CloudStack network")
        self.infrastructure()
        self.ensure_rif("uplink", 6, mac, f"{addr}/{net.prefixlen}")
        if self.live_rules("ingress") is None:
            # An addressed public RIF must not be exposed before CloudStack
            # sends its first complete firewall snapshot.
            self.ctl.run(self.policy_command("ingress", "policy set") + " --default deny")
        routes = self.ctl.run(f"vr route show --id {self.v}")
        old = next((fields(x) for x in routes.splitlines() if x.startswith("static 0.0.0.0/0 ")), None)
        if old and old.get("via") != gateway:
            self.ctl.run(f"vr route del --id {self.v} --prefix 0.0.0.0/0")
            old = None
        if not old:
            self.ctl.run(f"vr route add --id {self.v} --prefix 0.0.0.0/0 --via {gateway} --interface uplink")
        first, last = ports(self.d.get("nat.port.range", "20000-60999"))
        nat = self.ctl.run(f"vr nat show --id {self.v}")
        if "nat=enabled" not in nat:
            self.ctl.run(f"vr nat enable --id {self.v} --interface uplink --address interface --port-range {first}-{last}")
        else:
            check("interface=uplink" in nat and f"address={addr}" in nat and f"ports={first}-{last}" in nat, "NAT differs from desired state")
        self.cache["public"] = desired

    def owns(self, addr):
        check(self.cache.get("public") and address(addr) == self.cache["public"]["public_ip"], "Rules require this VR's SNAT/public IP")

    def compile_fw(self, snapshot):
        check(isinstance(snapshot, dict) and isinstance(snapshot.get("rules"), list), "fw_rules requires a full rules array")
        default = boolean(snapshot.get("default_egress_allow"))
        check(network(snapshot.get("cidr")) == network(self.cache.get("cidr")), "Firewall CIDR differs from network")
        compiled, seen = {"ingress": [], "egress": []}, set()
        for rule in snapshot["rules"]:
            check(isinstance(rule, dict) and set(rule) <= {"id", "type", "protocol", "portStart", "portEnd", "icmpType", "icmpCode", "publicIp", "sourceCidrs", "destCidrs"}, "Unsupported firewall fields")
            direction = rule.get("type")
            identity = str(number(rule.get("id"), 1, 2**63-1, "firewall ID"))
            check(direction in compiled and identity and (direction, identity) not in seen, "Invalid/duplicate firewall identity")
            seen.add((direction, identity))
            protocol = str(rule.get("protocol", "")).lower()
            check(protocol in ("tcp", "udp", "icmp", "all"), "Unsupported firewall protocol")
            sources, destinations = rule.get("sourceCidrs", []), rule.get("destCidrs", [])
            check(isinstance(sources, list) and isinstance(destinations, list), "CIDRs must be arrays")
            sources = sorted(set(str(network(x)) for x in (sources or ["0.0.0.0/0"])))
            destinations = sorted(set(str(network(x)) for x in (destinations or ["0.0.0.0/0"])))
            if direction == "ingress":
                self.owns(rule.get("publicIp"))
                check(all(ip.IPv4Address(rule["publicIp"]) in network(x) for x in destinations), "Ingress destination excludes public IP")
                destinations = [rule["publicIp"] + "/32"]
            suffix = []
            if "portStart" in rule or "portEnd" in rule:
                check(protocol in ("tcp", "udp") and "portStart" in rule and "portEnd" in rule, "Ports require TCP/UDP and both bounds")
                suffix += ["--port-range", port_text(f"{rule['portStart']}-{rule['portEnd']}")]
            for key, option in (("icmpType", "--icmp-type"), ("icmpCode", "--icmp-code")):
                if rule.get(key) is not None:
                    check(protocol == "icmp", "ICMP fields require ICMP")
                    if str(rule[key]) != "-1":
                        suffix += [option, str(number(rule[key], 0, 255, key))]
            action = "allow" if direction == "ingress" or not default else "deny"
            for src in sources:
                for dst in destinations:
                    compiled[direction].append((identity, ["--action", action, "--protocol", protocol, "--source", src, "--destination", dst, *suffix]))
        for rules in compiled.values():
            check(len(rules) <= 511, "Expanded rule capacity exceeded (one slot reserved for update fence)")
            rules.sort(key=lambda x: (x[0], x[1]))
        return default, compiled

    def policy_name(self, direction):
        return "uplink" if direction == "ingress" else f"SW{self.v}"

    def policy_command(self, direction, suffix):
        return f"vr {direction} {suffix} --id {self.v} --interface {self.policy_name(direction)}"

    def live_rules(self, direction):
        if not self.rif(self.policy_name(direction)):
            return None
        try:
            output = self.ctl.run(self.policy_command(direction, "rule show"))
            return [int(x) for x in re.findall(r"^rule=(\d+)", output, re.M)]
        except Error as exc:
            if "policy not configured" in str(exc):
                return None
            raise

    def fence(self, direction):
        self.ctl.run(self.policy_command(direction, "policy set") + " --default deny")
        if 1 not in (self.live_rules(direction) or []):
            self.ctl.run(self.policy_command(direction, "rule add") + " --rule-id 1 --action deny --protocol all")

    def firewall_matches(self, default, compiled, directions):
        for direction in directions:
            if self.live_rules(direction) is None:
                return False
            output = self.ctl.run(self.policy_command(direction, "rule show"))
            policy = next((fields(line) for line in output.splitlines() if line.startswith(direction + " ")), {})
            if policy.get("default") != ("allow" if direction == "egress" and default else "deny"):
                return False
            actual = {int(f["rule"]): {k: v for k, v in f.items() if k != "rule"}
                      for line in output.splitlines() if line.startswith("rule=") and "rule" in (f := fields(line))}
            expected = {}
            for rid, (_, tokens) in enumerate(compiled[direction], 2):
                options = dict(zip((x.removeprefix("--") for x in tokens[::2]), tokens[1::2]))
                if "port-range" in options:
                    a, b = ports(options.pop("port-range"))
                    options["port"] = f"{a}-{b}"
                expected[rid] = options
            if actual != expected:
                return False
        return True

    def firewall(self, snapshot):
        default, compiled = self.compile_fw(snapshot)  # No writes until all rules validate.
        check(self.rif(f"SW{self.v}"), "Private RIF missing")
        public = bool(self.rif("uplink"))
        check(public or not compiled["ingress"], "Public RIF missing")
        directions = ["ingress", "egress"] if public else ["egress"]
        if self.firewall_matches(default, compiled, directions):
            self.cache["firewall"] = snapshot
            return
        try:
            for direction in directions:
                self.fence(direction)
            for direction in directions:
                for rid in self.live_rules(direction) or []:
                    if rid != 1:
                        self.ctl.run(self.policy_command(direction, "rule delete") + f" --rule-id {rid}")
                for rid, (_, tokens) in enumerate(compiled[direction], 2):
                    self.ctl.run(self.policy_command(direction, "rule add") + f" --rule-id {rid} " + " ".join(tokens))
            self.ctl.run(self.policy_command("egress", "policy set") + (" --default allow" if default else " --default deny"))
            for direction in directions:
                self.ctl.run(self.policy_command(direction, "rule delete") + " --rule-id 1")
        except Error:
            for direction in directions:
                with contextlib.suppress(Error):
                    self.fence(direction)
            raise
        self.cache["firewall"] = snapshot

    def live_pf(self):
        return {int(f["rule"]): f for line in self.ctl.run(f"vr port-forward show --id {self.v}").splitlines()
                if "rule" in (f := fields(line))}

    def pf_sync(self, desired):
        current, wanted = self.live_pf(), {r["id"]: r for r in desired.values()}
        for rid, live in list(current.items()):
            r = wanted.get(rid)
            same = r and live.get("interface") == "uplink" and live.get("protocol") == r["protocol"] and live.get("public") == f"{r['public_ip']}:{r['public_port']}" and live.get("private") == f"{r['private_ip']}:{r['private_port']}"
            if not same:
                self.ctl.run(f"vr port-forward delete --id {self.v} --rule-id {rid}")
                current.pop(rid)
        for rid, r in sorted(wanted.items()):
            if rid not in current:
                self.ctl.run(f"vr port-forward add --id {self.v} --rule-id {rid} --interface uplink --protocol {r['protocol']} --public-port {r['public_port']} --private-ip {r['private_ip']} --private-port {r['private_port']}")

    def pf(self, delete=False):
        self.owns(self.p.get("public_ip"))
        protocol = str(self.p.get("protocol", "")).lower()
        check(protocol in ("tcp", "udp"), "PF requires TCP/UDP")
        public, private = port_text(self.p.get("public_port")), port_text(self.p.get("private_port"))
        a, b = ports(public), ports(private)
        check(a[1]-a[0] == b[1]-b[0], "PF spans must have equal widths")
        addr, net = address(self.p.get("private_ip")), network(self.cache.get("cidr"))
        check(ip.IPv4Address(addr) in net and addr != self.cache.get("gateway") and ip.IPv4Address(addr) not in (net.network_address, net.broadcast_address), "PF target must be a guest address in this VR")
        key = f"{protocol}:{self.p['public_ip']}:{public}"
        desired = dict(self.cache.get("port_forwards", {}))
        if delete:
            old = desired.get(key)
            if old and (old["private_ip"] != addr or old["private_port"] != private):
                # A delayed revoke of the former target must not remove its
                # replacement, because upstream doesn't provide a PF ID.
                return
            desired.pop(key, None)
        else:
            used = {r["id"] for r in desired.values()}
            rid = desired[key]["id"] if key in desired else next((x for x in range(1, 65536) if x not in used), None)
            check(rid, "PF ID capacity exhausted")
            for k, r in desired.items():
                c = ports(r["public_port"])
                check(k == key or r["protocol"] != protocol or c[1] < a[0] or a[1] < c[0], "PF port spans overlap")
            desired[key] = dict(id=rid, protocol=protocol, public_ip=self.p["public_ip"], public_port=public, private_ip=addr, private_port=private)
        self.pf_sync(desired)
        self.cache["port_forwards"] = desired

    def restore_services(self):
        # CloudStack's restore_data is NIC metadata. Firewall/PF are restored
        # by their dedicated callbacks; full fw_rules always supersedes cache.
        snapshot = self.p.get("fw_rules", self.cache.get("firewall"))
        if snapshot is not None:
            self.firewall(snapshot)
        if self.cache.get("public"):
            self.pf_sync(self.cache.get("port_forwards", {}))

    def clear_policy(self, direction):
        rules = self.live_rules(direction)
        if rules is not None:
            for rid in rules:
                self.ctl.run(self.policy_command(direction, "rule delete") + f" --rule-id {rid}")
            self.ctl.run(self.policy_command(direction, "policy delete"))

    def clear_public(self):
        for rid in self.live_pf():
            self.ctl.run(f"vr port-forward delete --id {self.v} --rule-id {rid}")
        self.clear_policy("ingress")
        if "nat=enabled" in self.ctl.run(f"vr nat show --id {self.v}"):
            self.ctl.run(f"vr nat disable --id {self.v}")
        for line in self.ctl.run(f"vr route show --id {self.v}").splitlines():
            if line.startswith("static "):
                self.ctl.run(f"vr route del --id {self.v} --prefix {line.split()[1]}")
        self.detach_rif("uplink")

    def detach_rif(self, name):
        r = self.rif(name)
        if r:
            check(r.get("type") == "vs-link", "Unexpected legacy RIF; migrate first")
            if r.get("address", "-") != "-":
                self.ctl.run(f"vr ip del --id {self.v} --interface {name} --address {r['address']}")
            self.ctl.run(f"vr switch detach --id {self.v} --interface {name}")

    def teardown(self, destroy):
        if self.vr(True):
            self.clear_policy("egress")
            self.clear_public()
            self.detach_rif(f"SW{self.v}")
            self.ctl.run(f"vr delete --id {self.v}")
        if self.v in self.switches():
            for member in self.switches()[self.v]:
                self.ctl.run(f"vs port detach --id {self.v} --port {member.split(':')[0]}")
            self.ctl.run(f"vs delete --id {self.v}")
        if destroy:
            write(self.root / "vf-alloc.json", {k: a for k, a in self.allocations().items() if a["network"] != self.n})
            if self.file.exists():
                self.file.unlink()
        else:
            write(self.file, self.cache)

    def pci(self, vf):
        number(vf, 1, 50, "guest VF")
        mapping = self.d.get("vf.pci.map", {})
        if isinstance(mapping, str):
            mapping = json.loads(mapping)
        check(isinstance(mapping, dict), "vf.pci.map must be an object")
        # A locally installed, operator-verified KVM-host mapping is an
        # alternative to passing registration details on every callback.
        # Explicit registration always takes precedence, even if incomplete.
        if not mapping and not self.d.get("host.pf.pci") and "vf.pci.map" not in self.d:
            local = read(Path(os.environ.get("DPU_ESWITCH_PCI_MAP_FILE", "/etc/cloudstack/extensions/dpu-eswitch/vf-pci-map.json")), {})
            mapping = local.get("vf.pci.map", {})
            check(isinstance(mapping, dict), "Local vf.pci.map must be an object")
        check(len(set(str(x).lower() for x in mapping.values())) == len(mapping), "Duplicate VF PCI addresses")
        check("0" not in mapping, "VF0 is reserved for uplink; exclude it from guest PCI mapping")
        bdf = mapping.get(str(vf))
        if not bdf:
            pf = self.d.get("host.pf.pci", "")
            check(re.fullmatch(r"[\da-fA-F]{4}:[\da-fA-F]{2}:[\da-fA-F]{2}\.[0-7]", pf), "Provide host.pf.pci or explicit vf.pci.map")
            path = Path("/sys/bus/pci/devices") / pf / f"virtfn{vf}"
            check(path.exists(), f"Host VF{vf} BDF unavailable on Arm; provide vf.pci.map")
            bdf = path.resolve().name
        check(re.fullmatch(r"[\da-fA-F]{4}:[\da-fA-F]{2}:[\da-fA-F]{2}\.[0-7]", bdf), "Invalid VF PCI BDF")
        return bdf.lower()

    def nic(self, release=False):
        nic = str(self.p.get("nic_uuid", ""))
        check(re.fullmatch(r"[\w-]{1,128}", nic), "Missing/invalid nic_uuid")
        allocations = self.allocations()
        old = allocations.get(nic)
        check(not old or old["network"] == self.n, "NIC belongs to another network")
        _, vfs = self.inventory()
        if release:
            if not old:
                return {}
            check(old["vf"] in vfs, "Allocated VF missing")
            if any(m.split(":")[0] == str(vfs[old["vf"]]) for m in self.switches().get(self.v, [])):
                self.ctl.run(f"vs port detach --id {self.v} --port {vfs[old['vf']]}")
            allocations.pop(nic)
            write(self.root / "vf-alloc.json", allocations)
            return {"vm.pci.bus.addresses.remove": old["pci"]}
        pool = set()
        for part in str(self.d.get("vf.pool", "1-50")).split(","):
            match = re.fullmatch(r"(\d+)(?:-(\d+))?", part.strip())
            check(match, "vf.pool requires indexes/ranges, excluding VF0")
            a = number(match[1], 1, 50, "guest VF")
            pool.update(range(a, number(match[2] or a, a, 50, "guest VF") + 1))
        used = {a["vf"] for a in allocations.values()}
        busy = {int(m.split(":")[0]) for members in self.switches().values() for m in members}
        vf = old["vf"] if old else next((v for v in sorted(pool) if v in vfs and v not in used and vfs[v] not in busy), None)
        check(vf is not None and vf in pool and vf in vfs, "No free/valid guest VF")
        bdf, mac = self.pci(vf), self.p.get("mac", "")
        check(re.fullmatch(r"(?:[\da-fA-F]{2}:){5}[\da-fA-F]{2}", mac), "Invalid NIC MAC")
        self.private()
        if self.cache.get("public"):
            self.public()
        self.restore_services()
        self.attach(self.v, vfs[vf])
        # Record the reservation before MAC programming so a failed MAC update
        # can be retried/released without leaking the representor allocation.
        allocations[nic] = dict(network=self.n, vf=vf, pci=bdf, mac=mac)
        write(self.root / "vf-alloc.json", allocations)
        if shutil.which("devlink"):
            output = subprocess.run(["devlink", "port", "show"], text=True, capture_output=True, check=True).stdout
            matches = [line.split(": ")[0] for line in output.splitlines() if re.search(rf"flavour pcivf .*pfnum {self.d.get('vf.pf', 0)} vfnum {vf}(?: |$)", line)]
            check(len(matches) == 1, "Cannot identify unique devlink VF port")
            subprocess.run(["devlink", "port", "function", "set", matches[0], "hw_addr", mac], check=True)
        else:
            check(self.d.get("vf.mac.preconfigured") == "true", "devlink missing; VF MAC must be preconfigured")
        write(self.file, self.cache)
        return {"network.broadcast_domain_type": "Dpu", "network.broadcast_uri": f"dpu://{self.v}", "nic.pci.address": bdf, "nic.vf.index": str(vf), "vm.pci.bus.addresses": bdf}

    def empty_unidentified_cleanup(self, command, status):
        """A failed first implementation may have no VID to resolve at all.

        Without ownership metadata, only the globally empty tenant baseline
        proves absence. Never guess a VR from network_id or public_ip.
        """
        cleanup = command in ("release-ip", "shutdown-network", "destroy-network", "release-nic")
        snapshot = self.p.get("fw_rules")
        empty_fw = command == "apply-fw-rules" and isinstance(snapshot, dict) and snapshot.get("rules") == []
        if not (cleanup or empty_fw):
            return False
        number(self.n, 1, 2**63-1, "network_id")
        check(not self.p.get("vpc_id"), "VPC is unsupported")
        check(str(self.p.get("guest_type", "isolated")).lower() == "isolated", "Only isolated networks supported")
        file = self.root / f"network-{self.n}" / "desired.json"
        if file.exists() or self.p.get("vlan") or self.e.get("vlan"):
            return False
        check(not self.e.get("vr_id") and not self.e.get("vswitch_id"), "Cleanup has unresolved tenant identity")
        check(not any(self.root.glob("network-*/desired.json")), "Cannot prove empty cleanup: tenant cache exists")
        check(not self.allocations(), "Cannot prove empty cleanup: VF allocations exist")
        check(re.search(r"(?:^|\s)routers=0(?:\s|$)", status), "Cannot prove empty cleanup: live routers exist or count unavailable")
        check(set(self.switches()) <= {6}, "Cannot prove empty cleanup: tenant switches exist")
        if self.p.get("public_ip"):
            address(self.p["public_ip"])
        return True

    def dispatch(self, command):
        allowed = {"status", "ensure-infrastructure", "implement-network", "restore-network", "assign-ip", "release-ip", "prepare-nic", "release-nic", "apply-fw-rules", "add-port-forward", "delete-port-forward", "shutdown-network", "destroy-network"}
        check(command in allowed, f"Unsupported command: {command}")
        if command == "status":
            return {"daemon": self.ctl.run("status"), "ports": self.ctl.run("port show")}
        self.root.mkdir(parents=True, exist_ok=True)
        # Never unlink this file: all waiters must lock the same inode.
        with (self.root / "lock-dpu").open("a") as lock:
            fcntl.flock(lock, fcntl.LOCK_EX)
            status = self.ctl.run("status")
            if command == "ensure-infrastructure":
                self.infrastructure()
                return {}
            if self.empty_unidentified_cleanup(command, status):
                return {"cleanup": "verified-empty-tenant-baseline"}
            self.identify()
            if command in ("implement-network", "restore-network"):
                self.private()
                if self.cache.get("public"):
                    self.public()
                self.restore_services()
                write(self.file, self.cache)
                return {"network.broadcast_domain_type": "Dpu", "network.broadcast_uri": f"dpu://{self.v}"}
            if command == "assign-ip":
                check(boolean(self.p.get("source_nat", False)), "Additional non-SNAT public IPs unsupported")
                self.public({k: self.p.get(k) for k in ("public_ip", "public_vlan", "public_gateway", "public_cidr")})
                self.restore_services()
            elif command == "release-ip":
                if not self.cache.get("public"):
                    return {}
                self.owns(self.p.get("public_ip"))
                if self.vr(True):
                    self.clear_public()
                self.cache.pop("public", None)
                self.cache.pop("port_forwards", None)
                if self.cache.get("firewall"):
                    self.cache["firewall"]["rules"] = [r for r in self.cache["firewall"]["rules"] if r["type"] == "egress"]
            elif command in ("prepare-nic", "release-nic"):
                return self.nic(command == "release-nic")
            elif command == "apply-fw-rules":
                self.firewall(self.p.get("fw_rules"))
            elif command in ("add-port-forward", "delete-port-forward"):
                self.pf(command == "delete-port-forward")
            elif command in ("shutdown-network", "destroy-network"):
                self.teardown(command == "destroy-network")
                return {}
            write(self.file, self.cache)
            return {}


def main():
    command, network_id = "", ""
    try:
        check(len(sys.argv) >= 2, "Usage: wrapper COMMAND PAYLOAD.json TIMEOUT")
        command = sys.argv[1]
        envelope = json.loads(Path(sys.argv[2]).read_text()) if len(sys.argv) > 2 else {}
        adapter = Adapter(envelope)
        network_id = adapter.n
        adapter.ctl.timeout = max(5, float(sys.argv[3]) if len(sys.argv) > 3 else 60)
        result = adapter.dispatch(sys.argv[1])
        audit(command, network_id, "success")
        print(json.dumps({"status": "success", **result}, separators=(",", ":")))
    except (Error, ValueError, TypeError, OSError, subprocess.SubprocessError) as exc:
        audit(command, network_id, str(exc))
        print(f"ERROR: {exc}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
