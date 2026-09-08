# Software TX entry into EGRESS

Revision `egress-entry-v1`, selected with `TX_PROBE_PATH=sw-egress`.

This experiment asks only whether software-generated packets hit an EGRESS
entry. It is not a fix for the previous zero-hit result.

```text
application generates 10 fresh 60-byte ARP frames, no TX metadata
  -> verified parent DPDK port 0 / TX queue 0
  -> EGRESS root: match-all -> COUNT + DROP

external ingress -> DEFAULT root: COUNT + DROP
```

No VF forwarding action is installed in this mode. The sample still opens and
validates the same parent and VF10 to keep device bring-up comparable. The MAC/IP
environment variables describe the generated frame; EGRESS does not match them.
No guest capture or ARP generator is needed. Stop VM ping/arping and other test
traffic. Stop the daemon and any other owner of PCI `03:00.0` before running:
the test uses that eSwitch, so use the existing isolated test environment.

## Build in doca-dev after syncing source

```bash
cd /doca_devel/Sample-DOCA-Application/application/eswitch_management/diagnostics/tx_reproducer
meson setup --reconfigure /build/eswitch-tx-reproducer .
meson compile -C /build/eswitch-tx-reproducer
meson test -C /build/eswitch-tx-reproducer --print-errorlogs
strings /build/eswitch-tx-reproducer/eswitch-tx-reproducer | rg 'egress-entry-v1'
```

For a first build use `meson setup /build/eswitch-tx-reproducer .` without
`--reconfigure`. The daemon build directory does not build this executable.

## Run on DPU Arm in doca-dev

```bash
TX_PROBE_PATH=sw-egress \
TX_PROBE_VF=10 \
TX_PROBE_VM_MAC=7e:83:a5:77:11:06 \
TX_PROBE_GATEWAY_MAC=02:00:00:65:00:01 \
TX_PROBE_VM_IP=192.168.0.10 \
TX_PROBE_GATEWAY_IP=192.168.0.1 \
/build/eswitch-tx-reproducer/eswitch-tx-reproducer \
  -l 0 --file-prefix=eswitch-tx-reproducer -- \
  --rep 'pci/03:00.0,c1pf0vf10' --expert-mode \
  --log-level 50 --sdk-log-level 50 \
  > /tmp/eswitch-egress-entry.log 2>&1
probe_rc=$?
printf 'probe_exit=%s\n' "$probe_rc"
rg -n 'PORT MAP|PROBE CONFIG|BASELINE|PATH SAMPLE|EGRESS RESULT|CHECK FAILED|PROBE ERROR' \
  /tmp/eswitch-egress-entry.log
```

Expected configuration: `path=sw-egress; EGRESS=match-all->COUNT+DROP;
revision=egress-entry-v1`.

Expected result:

```text
EGRESS RESULT: PASS accepted=10 egress_delta=10 ingress_delta=0 action=DROP guest_delivery=NOT_APPLICABLE
```

The test waits five seconds without TX, reads baseline counters, sends one
packet per second and samples EGRESS/ingress, then waits three seconds before
the final query. PASS requires ten accepted packets, ten EGRESS hits since the
baseline, no ingress hits during transmission/settling, and no interruption or
API failure. Nonzero EGRESS before sending invalidates the run. Query errors
abort explicitly; zero is never substituted for a failed query.

- accepted=10 / egress_delta=0: still no observed EGRESS traversal, even without
  a VF forwarding action. Investigate TX injection/root attachment next.
- egress_delta=10 / ingress_delta=0: EGRESS traversal confirmed in this isolated
  test; repeat old `sw` mode separately to test the VF forwarding action.
- ingress_delta>0: ambiguous/contaminated run, not proof of TX entering DEFAULT.
- No EGRESS RESULT: inspect CONFIG/PORT/ENTRY/PROBE errors; setup or query failed.

The counter is hardware evidence for entry hits, not packet capture. Attribution
requires the isolated environment and no other transmitter on the parent. Guest
silence is expected because the successful EGRESS action is DROP.

Source has been reviewed locally; SDK compilation and the hardware result must
be verified in doca-dev. The authoring machine has no DOCA SDK/DPU.
