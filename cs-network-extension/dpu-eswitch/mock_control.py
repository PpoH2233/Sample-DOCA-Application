"""Apache-2.0. Stateful control-protocol mock, enforcing dependency ordering.

Used by both subprocess wrapper tests and direct fault-injection tests.
"""
import json
from pathlib import Path
import re
import shlex
from dpu_eswitch import Error, fields


class Mock:
    def __init__(self, path=None):
        self.path = Path(path) if path else None
        self.state = json.loads(self.path.read_text()) if self.path and self.path.exists() else {
            "switches": {}, "routers": {}, "port_offset": 0, "commands": []}
        self.fail = None

    def run(self, command):
        self.state["commands"].append(command)
        if self.fail and self.fail(command):
            raise Error("ERR injected hardware/persistence failure")
        tokens = shlex.split(command)
        opts = dict(zip(tokens[3::2], tokens[4::2])) if tokens[:2] == ["vs", "port"] else {}
        for index, token in enumerate(tokens):
            if token.startswith("--") and token != "--all":
                opts[token[2:]] = tokens[index + 1]
        v = opts.get("id")
        switches, routers = self.state["switches"], self.state["routers"]
        if command == "status":
            result = f"service=eSwitch Management state=running routers={len(routers)}"
        elif command in ("port show", "port show --all"):
            offset = self.state["port_offset"]
            result = f"DPDK port {offset} (uplink/parent)\n" + "\n".join(
                f"DPDK port {offset + vf + 1} (host=1 pf=0 vf={vf})" for vf in range(51))
        elif tokens[:2] == ["vs", "show"]:
            result = "\n".join(f"vs={k} name={s['name']} ports=[{','.join(s['members'])}]"
                               for k, s in switches.items() if not v or k == v)
        elif tokens[:2] == ["vs", "create"]:
            if v in switches:
                raise Error("ERR vSwitch exists")
            switches[v] = {"name": opts.get("name", "-"), "members": []}
            result = ""
        elif tokens[:2] == ["vs", "delete"]:
            if switches[v]["members"] or any(any(r.get("switch") == v for r in vr["rifs"].values()) for vr in routers.values()):
                raise Error("ERR switch has dependencies")
            switches.pop(v)
            result = ""
        elif tokens[:2] == ["vs", "port"]:
            member = opts["port"] + (":trunk/vlan=" + opts["vlan"] if opts.get("mode") == "trunk" else ":access")
            if tokens[2] == "attach":
                if any(m.split(":")[0] == opts["port"] for s in switches.values() for m in s["members"]):
                    raise Error("ERR port owned")
                switches[v]["members"].append(member)
            else:
                switches[v]["members"] = [m for m in switches[v]["members"] if m.split(":")[0] != opts["port"]]
            result = ""
        elif tokens[:2] == ["vr", "create"]:
            if v in routers:
                raise Error("ERR VR already exists")
            routers[v] = {"rifs": {}, "routes": {}, "nat": None, "ingress": {}, "egress": {}, "pf": {}}
            result = ""
        else:
            if v not in routers:
                raise Error("ERR VR not found")
            vr = routers[v]
            name = opts.get("interface", opts.get("name"))
            if tokens[:2] == ["vr", "show"]:
                result = "\n".join(f"{n} ifindex={i} type=vs-link " + " ".join(f"{k}={val}" for k, val in r.items())
                                   for i, (n, r) in enumerate(vr["rifs"].items(), 1))
            elif tokens[:2] == ["vr", "delete"]:
                if vr["rifs"] or vr["routes"] or vr["nat"] or vr["pf"] or vr["ingress"] or vr["egress"]:
                    raise Error("ERR VR dependencies")
                routers.pop(v)
                result = ""
            elif tokens[:2] == ["vr", "switch"]:
                if tokens[2] == "attach":
                    if opts["switch-id"] not in switches:
                        raise Error("ERR VS not found")
                    vr["rifs"][name] = {"switch": opts["switch-id"], "mac": "-", "address": "-"}
                else:
                    if vr["rifs"][name]["address"] != "-" or name in vr["ingress"] or name in vr["egress"]:
                        raise Error("ERR RIF dependencies")
                    vr["rifs"].pop(name)
                result = ""
            elif tokens[:2] == ["vr", "interface"]:
                vr["rifs"][name]["mac"] = opts["mac"]
                result = ""
            elif tokens[:2] == ["vr", "ip"]:
                if tokens[2] == "del" and (vr["nat"] or vr["pf"] or vr["routes"]):
                    raise Error("ERR IP dependencies")
                vr["rifs"][name]["address"] = opts["address"] if tokens[2] == "add" else "-"
                result = ""
            elif tokens[:2] == ["vr", "route"]:
                if tokens[2] == "show":
                    result = "\n".join(f"static {p} via={r['via']} ifindex=2" for p, r in vr["routes"].items())
                elif tokens[2] == "add":
                    vr["routes"][opts["prefix"]] = opts
                    result = ""
                else:
                    vr["routes"].pop(opts["prefix"])
                    result = ""
            elif tokens[:2] == ["vr", "nat"]:
                if tokens[2] == "show":
                    result = "nat=disabled" if not vr["nat"] else f"nat=enabled interface=uplink address={vr['rifs']['uplink']['address'].split('/')[0]} ports={vr['nat']}"
                else:
                    vr["nat"] = opts["port-range"] if tokens[2] == "enable" else None
                    result = ""
            elif tokens[:2] == ["vr", "port-forward"]:
                if tokens[2] == "show":
                    result = "\n".join(f"rule={rid} interface=uplink public={vr['rifs']['uplink']['address'].split('/')[0]}:{r['public-port']} protocol={r['protocol']} private={r['private-ip']}:{r['private-port']}" for rid, r in vr["pf"].items())
                elif tokens[2] == "add":
                    if opts["rule-id"] in vr["pf"]:
                        raise Error("ERR duplicate PF ID")
                    vr["pf"][opts["rule-id"]] = opts
                    result = ""
                else:
                    vr["pf"].pop(opts["rule-id"])
                    result = ""
            elif tokens[1] in ("ingress", "egress"):
                direction, resource, action = tokens[1:4]
                policies = vr[direction]
                if resource == "policy":
                    if action == "set":
                        policies.setdefault(name, {"rules": {}})["default"] = opts["default"]
                    elif action == "delete":
                        if policies[name]["rules"]:
                            raise Error("ERR remove rules first")
                        policies.pop(name)
                    result = ""
                else:
                    if name not in policies:
                        raise Error("ERR egress policy not configured")
                    rules = policies[name]["rules"]
                    if action == "show":
                        lines = [f"{direction} interface={name} default={policies[name]['default']}"]
                        for rid, r in rules.items():
                            signature = {k: val for k, val in r.items() if k in ("action", "protocol", "source", "destination", "icmp-type", "icmp-code")}
                            signature.setdefault("source", "0.0.0.0/0")
                            signature.setdefault("destination", "0.0.0.0/0")
                            if "port-range" in r:
                                a, _, b = r["port-range"].partition("-")
                                signature["port"] = f"{a}-{b or a}"
                            lines.append(f"rule={rid} " + " ".join(f"{k}={val}" for k, val in signature.items()))
                        result = "\n".join(lines)
                    elif action == "add":
                        if opts["rule-id"] in rules:
                            raise Error("ERR duplicate rule ID")
                        rules[opts["rule-id"]] = opts
                        result = ""
                    else:
                        rules.pop(opts["rule-id"])
                        result = ""
            else:
                raise Error(f"ERR unsupported mock command: {command}")
        if self.path:
            self.path.parent.mkdir(parents=True, exist_ok=True)
            self.path.write_text(json.dumps(self.state))
        return "OK\n" + result + "\n"
