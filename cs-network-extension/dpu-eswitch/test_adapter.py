"""Offline lifecycle and failure tests; never connect to the production socket."""
import copy
import json
import os
from pathlib import Path
import subprocess
import tempfile
import unittest
from unittest.mock import patch
from dpu_eswitch import Adapter, Error, read, write
from mock_control import Mock


class AdapterTests(unittest.TestCase):
    def test_missing_vid_cleanup_empty_baseline(self):
        self.call("ensure-infrastructure")
        before = copy.deepcopy(self.mock.state["switches"])
        for command in ("release-ip", "shutdown-network", "destroy-network", "release-nic", "apply-fw-rules"):
            result = self.call(command, {"vlan": "", "public_ip": "203.0.113.10", "fw_rules": {"rules": []}})
            self.assertEqual(result["cleanup"], "verified-empty-tenant-baseline")
        self.assertEqual(self.mock.state["switches"], before)
        self.assertFalse((self.root / "network-42" / "desired.json").exists())

    def test_missing_vid_cleanup_refuses_uncertain_ownership(self):
        self.mock.state["switches"]["900"] = {"name": "VS900", "members": []}
        with self.assertRaisesRegex(Error, "tenant switches"):
            self.call("release-ip", {"vlan": ""})
        self.mock.state["switches"].clear()
        self.mock.run("vr create --id 900")
        with self.assertRaisesRegex(Error, "live routers"):
            self.call("shutdown-network", {"vlan": ""})
        self.mock.state["routers"].clear()
        write(self.root / "vf-alloc.json", {"nic": {"network": "43", "vf": 1}})
        with self.assertRaisesRegex(Error, "VF allocations"):
            self.call("destroy-network", {"vlan": ""})

    def test_missing_vid_cleanup_does_not_hide_real_state_or_bad_payload(self):
        with self.assertRaises(Error):
            self.call("implement-network", {"vlan": ""})
        with self.assertRaises(Error):
            self.call("apply-fw-rules", {"vlan": "", "fw_rules": {"rules": [{}]}})
        with self.assertRaises(Error):
            self.call("release-ip", {"vlan": "garbage"})
        self.mock.fail = lambda command: command == "status"
        with self.assertRaises(Error):
            self.call("release-ip", {"vlan": ""})
        self.mock.fail = None
        write(self.root / "network-42" / "desired.json", {"network_id": "42"})
        with self.assertRaises(Error):
            self.call("release-ip", {"vlan": ""})

    def test_empty_router_template_has_exact_parser_header(self):
        # The daemon reads its header and next ID directly, without skipping comments.
        lines = Path(__file__).with_name("cloudstack-eswitch.conf.router").read_text().splitlines()
        self.assertEqual(lines, ["router-state 2", "next 1"])

    def setUp(self):
        self.temp = tempfile.TemporaryDirectory(prefix="cloudstack-wrapper-")
        self.root = Path(self.temp.name)
        self.env = patch.dict(os.environ, DPU_ESWITCH_STATE_DIR=str(self.root), DPU_ESWITCH_PCI_MAP_FILE=str(self.root / "vf-pci-map.json"))
        self.env.start()
        self.mock = Mock()
        self.router = self.root / "router.state"
        self.router.write_text("router-state 2\n")
        self.payload = {"payload": {"network_id": "42", "vlan": "900", "guest_type": "isolated", "gateway": "10.90.0.1", "cidr": "10.90.0.0/24"},
                        "physical-network-extension-details": {"vf.pool": "1-50", "router.state": str(self.router), "vf.pci.map": {"1": "0000:84:00.3", "50": "0000:84:06.4"}, "vf.mac.preconfigured": "true"}, "network-extension-details": {}}

    def tearDown(self):
        self.env.stop()
        self.temp.cleanup()

    def test_operator_verified_local_host_mapping(self):
        fixture = json.loads(Path(__file__).with_name("zona-01-vf-pci-map.json").read_text())
        write(self.root / "vf-pci-map.json", fixture)
        envelope = copy.deepcopy(self.payload)
        envelope["physical-network-extension-details"].pop("vf.pci.map")
        adapter = Adapter(envelope, self.mock)
        self.assertEqual(adapter.pci(1), "0000:84:00.3")
        self.assertEqual(adapter.pci(50), "0000:84:06.4")
        self.assertEqual(len(fixture["vf.pci.map"]), 50)
        with self.assertRaises(Error):
            adapter.pci(0)
        # An explicit map is authoritative, not silently supplemented.
        adapter.d["vf.pci.map"] = {"50": "0000:85:06.4"}
        self.assertEqual(adapter.pci(50), "0000:85:06.4")
        with self.assertRaises(Error):
            adapter.pci(1)
        adapter.d.pop("vf.pci.map")
        with patch("dpu_eswitch.shutil.which", return_value=None):
            result = self.call("prepare-nic", {"nic_uuid": "local-nic", "mac": "02:00:00:00:00:21"}, envelope)
        self.assertEqual(result["vm.pci.bus.addresses"], "0000:84:00.3")
        self.assertEqual(self.mock.state["switches"]["900"]["members"], ["2:access"])

    def call(self, command, extra=None, envelope=None):
        payload = copy.deepcopy(envelope or self.payload)
        payload["payload"].update(extra or {})
        return Adapter(payload, self.mock).dispatch(command)

    def implemented(self):
        self.call("implement-network")

    def public(self):
        self.implemented()
        self.call("assign-ip", {"source_nat": "true", "public_vlan": "6", "public_ip": "203.0.113.10", "public_gateway": "203.0.113.1", "public_cidr": "203.0.113.0/24"})

    def snapshot(self, default=False):
        return {"default_egress_allow": default, "cidr": "10.90.0.0/24", "rules": [
            {"id": 999999, "type": "ingress", "protocol": "tcp", "portStart": 2222, "portEnd": 2222, "publicIp": "203.0.113.10", "sourceCidrs": ["192.0.2.0/24", "198.51.100.0/24"], "destCidrs": []},
            {"id": 2, "type": "egress", "protocol": "icmp", "icmpType": 8, "icmpCode": -1, "sourceCidrs": [], "destCidrs": ["0.0.0.0/0"]}]}

    def pf(self, **extra):
        return dict(public_ip="203.0.113.10", protocol="TCP", public_port="2222", private_port="22", private_ip="10.90.0.10", **extra)

    def test_network_lifecycle_shared_infrastructure(self):
        self.public()
        self.call("implement-network")
        self.assertEqual(self.mock.state["switches"]["6"]["members"], ["0:trunk/vlan=6", "1:trunk/vlan=6"])
        self.assertEqual(self.mock.state["switches"]["900"]["name"], "VS900")
        self.call("shutdown-network")
        self.assertEqual(set(self.mock.state["switches"]), {"6"})
        self.call("restore-network")
        self.assertIn("uplink", self.mock.state["routers"]["900"]["rifs"])
        self.call("destroy-network")
        self.call("destroy-network")
        self.assertEqual(set(self.mock.state["switches"]), {"6"})

    def test_second_router_survives_first_deletion(self):
        self.public()
        other = copy.deepcopy(self.payload)
        other["payload"].update(network_id="43", vlan="901", gateway="10.91.0.1", cidr="10.91.0.0/24")
        self.call("implement-network", envelope=other)
        self.call("assign-ip", {"source_nat": True, "public_vlan": 6, "public_ip": "203.0.113.11", "public_gateway": "203.0.113.1", "public_cidr": "203.0.113.0/24"}, other)
        self.call("destroy-network")
        self.assertIn("901", self.mock.state["routers"])
        self.assertEqual(len(self.mock.state["switches"]["6"]["members"]), 2)

    def test_vid_validation_before_mutation(self):
        for vid in (None, "899", "1000", "900;reboot"):
            with self.subTest(vid=vid), self.assertRaises((Error, ValueError)):
                self.call("implement-network", {"vlan": vid})
        self.assertEqual(self.mock.state["switches"], {})

    def test_vid_collision_and_changed_identity(self):
        self.implemented()
        with self.assertRaises(Error):
            self.call("implement-network", {"network_id": "43"})
        with self.assertRaises(Error):
            self.call("implement-network", {"vlan": "901"})

    def test_vf0_ownership_rejected_before_infrastructure_mutation(self):
        self.mock.state["switches"]["100"] = {"name": "legacy", "members": ["1:access"]}
        with self.assertRaises(Error):
            self.call("ensure-infrastructure")
        self.assertNotIn("6", self.mock.state["switches"])

    def test_public_duplicate_ownership(self):
        self.implemented()
        self.router.write_text("vr ip add --id 901 --interface uplink --address 203.0.113.10/24\n")
        with self.assertRaises(Error):
            self.call("assign-ip", {"source_nat": True, "public_vlan": 6, "public_ip": "203.0.113.10", "public_gateway": "203.0.113.1", "public_cidr": "203.0.113.0/24"})
        self.assertNotIn("6", self.mock.state["switches"])

    def test_firewall_expansion_defaults_updates_deletion(self):
        self.public()
        self.call("apply-fw-rules", {"fw_rules": self.snapshot()})
        vr = self.mock.state["routers"]["900"]
        self.assertEqual(len(vr["ingress"]["uplink"]["rules"]), 2)
        self.assertEqual(vr["ingress"]["uplink"]["default"], "deny")
        self.assertEqual(vr["egress"]["SW900"]["default"], "deny")
        ids = set(vr["ingress"]["uplink"]["rules"])
        writes = len([c for c in self.mock.state["commands"] if " rule add " in c])
        self.call("apply-fw-rules", {"fw_rules": self.snapshot()})
        self.assertEqual(len([c for c in self.mock.state["commands"] if " rule add " in c]), writes)
        self.assertEqual(set(vr["ingress"]["uplink"]["rules"]), ids)
        self.call("apply-fw-rules", {"fw_rules": self.snapshot(True)})
        self.assertEqual(vr["egress"]["SW900"]["rules"]["2"]["action"], "deny")
        self.call("apply-fw-rules", {"fw_rules": {"default_egress_allow": True, "cidr": "10.90.0.0/24", "rules": []}})
        self.assertEqual(vr["ingress"]["uplink"]["rules"], {})
        self.assertEqual(vr["egress"]["SW900"]["rules"], {})

    def test_unsupported_rule_keeps_previous_policy(self):
        self.public()
        self.call("apply-fw-rules", {"fw_rules": self.snapshot()})
        before = copy.deepcopy(self.mock.state["routers"])
        for invalid in ({"protocol": "gre"}, {"action": "deny"}, {"sourceCidrs": ["::/0"]}, {"portEnd": 99999}):
            snapshot = self.snapshot()
            snapshot["rules"][0].update(invalid)
            with self.assertRaises((Error, ValueError)):
                self.call("apply-fw-rules", {"fw_rules": snapshot})
            self.assertEqual(self.mock.state["routers"], before)

    def test_firewall_partial_failure_fences_and_retry(self):
        self.public()
        self.call("apply-fw-rules", {"fw_rules": self.snapshot()})
        committed = (self.root / "network-42/desired.json").read_text()
        self.mock.fail = lambda c: "egress rule add" in c and "--rule-id 2 " in c
        with self.assertRaises(Error):
            self.call("apply-fw-rules", {"fw_rules": self.snapshot(True)})
        for direction, name in (("ingress", "uplink"), ("egress", "SW900")):
            self.assertEqual(self.mock.state["routers"]["900"][direction][name]["rules"]["1"]["action"], "deny")
        self.assertEqual((self.root / "network-42/desired.json").read_text(), committed)
        self.mock.fail = None
        self.call("apply-fw-rules", {"fw_rules": self.snapshot(True)})

    def test_pf_replay_update_delete_and_span_validation(self):
        self.public()
        self.call("add-port-forward", self.pf())
        self.call("add-port-forward", self.pf())
        rules = self.mock.state["routers"]["900"]["pf"]
        self.assertEqual(len(rules), 1)
        updated = self.pf()
        updated["private_ip"] = "10.90.0.11"
        self.call("add-port-forward", updated)
        self.assertEqual(rules["1"]["private-ip"], "10.90.0.11")
        for field, value in (("private_port", "22:23"), ("private_ip", "10.91.0.10"), ("public_ip", "203.0.113.11"), ("protocol", "icmp")):
            invalid = self.pf()
            invalid[field] = value
            with self.assertRaises(Error):
                self.call("add-port-forward", invalid)
        self.call("delete-port-forward", updated)
        self.call("delete-port-forward", updated)
        self.assertEqual(rules, {})

    def test_pf_ranges_restore_and_partial_failure(self):
        self.public()
        rule = self.pf()
        rule.update(public_port="8000:8009", private_port="9000:9009", protocol="udp")
        self.call("add-port-forward", rule)
        self.call("apply-fw-rules", {"fw_rules": self.snapshot()})
        self.call("shutdown-network")
        self.call("restore-network")
        self.assertEqual(len(self.mock.state["routers"]["900"]["pf"]), 1)
        self.assertEqual(len(self.mock.state["routers"]["900"]["ingress"]["uplink"]["rules"]), 2)
        self.mock.fail = lambda c: "port-forward delete" in c
        with self.assertRaises(Error):
            self.call("delete-port-forward", rule)
        self.assertEqual(len(read(self.root / "network-42/desired.json", {})["port_forwards"]), 1)

    def test_cleanup_failure_not_success(self):
        self.public()
        self.mock.fail = lambda c: "nat disable" in c
        with self.assertRaises(Error):
            self.call("destroy-network")
        self.assertTrue((self.root / "network-42/desired.json").exists())
        self.assertIn("6", self.mock.state["switches"])

    def test_release_ip_preserves_shared_wan_and_egress(self):
        self.public()
        self.call("apply-fw-rules", {"fw_rules": self.snapshot()})
        self.call("add-port-forward", self.pf())
        self.call("release-ip", {"public_ip": "203.0.113.10"})
        self.call("release-ip", {"public_ip": "203.0.113.10"})
        vr = self.mock.state["routers"]["900"]
        self.assertNotIn("uplink", vr["rifs"])
        self.assertTrue(vr["egress"]["SW900"]["rules"])
        self.assertEqual(len(self.mock.state["switches"]["6"]["members"]), 2)

    def test_authoritative_snapshot_replaces_cache_after_restore(self):
        self.public()
        self.call("apply-fw-rules", {"fw_rules": self.snapshot()})
        self.call("shutdown-network")
        self.call("restore-network", {"fw_rules": {"default_egress_allow": False, "cidr": "10.90.0.0/24", "rules": []}})
        self.assertEqual(self.mock.state["routers"]["900"]["ingress"]["uplink"]["rules"], {})
        self.assertEqual(read(self.root / "network-42/desired.json", {})["firewall"]["rules"], [])

    def test_stale_pf_delete_does_not_remove_replacement(self):
        self.public()
        old = self.pf()
        self.call("add-port-forward", old)
        new = dict(old, private_ip="10.90.0.11")
        self.call("add-port-forward", new)
        self.call("delete-port-forward", old)
        self.assertEqual(self.mock.state["routers"]["900"]["pf"]["1"]["private-ip"], "10.90.0.11")

    def test_transport_failure_is_not_missing_vr_or_successful_cleanup(self):
        self.public()
        self.mock.fail = lambda c: c.startswith("vr show")
        with self.assertRaises(Error):
            self.call("shutdown-network")
        self.assertIn("900", self.mock.state["routers"])
        self.assertTrue((self.root / "network-42/desired.json").exists())

    def test_nic_allocation_replay_release_vf50_and_runtime_port_remap(self):
        self.implemented()
        data = {"nic_uuid": "nic-one", "mac": "02:00:00:00:00:11"}
        with patch("dpu_eswitch.shutil.which", return_value=None):
            result = self.call("prepare-nic", data)
            self.assertEqual(result["nic.vf.index"], "1")
            self.call("prepare-nic", data)
            self.mock.state["port_offset"] = 100
            self.mock.state["switches"]["900"]["members"] = []
            self.call("prepare-nic", data)
            self.assertEqual(self.mock.state["switches"]["900"]["members"], ["102:access"])
            self.call("release-nic", data)
            self.call("release-nic", data)
            self.payload["physical-network-extension-details"]["vf.pool"] = "50"
            result = self.call("prepare-nic", data)
            self.assertEqual(result["nic.vf.index"], "50")

    def test_vf0_pool_and_unknown_pci_rejected(self):
        self.implemented()
        self.payload["physical-network-extension-details"]["vf.pool"] = "0-50"
        with self.assertRaises(Error):
            self.call("prepare-nic", {"nic_uuid": "nic-one", "mac": "02:00:00:00:00:11"})
        self.payload["physical-network-extension-details"]["vf.pool"] = "22"
        with self.assertRaises(Error):
            self.call("prepare-nic", {"nic_uuid": "nic-one", "mac": "02:00:00:00:00:11"})
        self.assertEqual(self.mock.state["switches"]["900"]["members"], [])

    def test_entrypoint_file_payload_and_malformed_json(self):
        here = Path(__file__).resolve().parent
        payload = self.root / "payload.json"
        payload.write_text(json.dumps(self.payload))
        env = dict(os.environ, MOCK_STATE=str(self.root / "mock"), ESWITCHCTL=str(here / "mock-eswitchctl"),
                   DPU_ESWITCH_LOG_FILE=str(self.root / "wrapper.log"))
        result = subprocess.run(["bash", str(here / "dpu-eswitch-wrapper.sh"), "implement-network", str(payload), "5"], env=env, capture_output=True, text=True)
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(json.loads(result.stdout)["network.broadcast_uri"], "dpu://900")
        payload.write_text("{bad}")
        result = subprocess.run(["bash", str(here / "dpu-eswitch-wrapper.sh"), "apply-fw-rules", str(payload), "5"], env=env, capture_output=True, text=True)
        self.assertEqual(result.returncode, 1)


if __name__ == "__main__":
    unittest.main()
