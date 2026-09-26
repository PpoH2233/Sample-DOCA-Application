# Arm versus authorized CT: SCP and scaling experiment

## Prerequisite: prove offload before measuring speed

Use an isolated test topology and destination. The same binary, PF rule,
egress policy, VLANs, VM CPU/RAM, NIC MTU, SSH cipher, lcore and endpoints must
be used in both modes. Keep private LPM off for this comparison.

Restart the daemon with these environments (exports must reach the daemon,
not just `eswitchctl`; containers/services need their corresponding env config):

| Mode | Environment |
| --- | --- |
| A: Arm baseline | `ESWITCH_HW_ROUTING=0 ESWITCH_HW_CT=0` |
| B: authorized CT | `ESWITCH_HW_ROUTING=0 ESWITCH_HW_CT=1 ESWITCH_HW_CT_CAPACITY=64` |

Before/while/after one PF SSH connection, capture:

```bash
./eswitchctl status | grep -E '^(tx_revision|nat_dataplane|ct_authorization|ct_retry|port_forward_rules|guest_egress|egress_acl|ipv4_routing)'
```

Require v42. For B, require `hw_ct_active > 0` and promotions increasing during
traffic, no growing failures and no PF exception fallback. A failed B offload
run is an Arm fallback diagnostic, not a valid fast-path benchmark.
`hw_ct_full` now counts FULL only; `ct_no_memory` counts allocation failures.
`NAT CT PROMOTION FAILED` logs the failed stage and exponential backoff
(250 ms up to 8 seconds). `ct_retry_suppressed` counts packets that skipped an
attempt during that backoff. Last-error fields are historical, not current state.

## Prepare a payload on the VM, outside the measured interval

Check free disk space first. Create a dedicated directory and a 2 GiB payload:

```bash
bench_dir=$(mktemp -d /tmp/eswitch-perf.XXXXXX)
dd if=/dev/zero of="$bench_dir/payload.bin" bs=1M count=2048 status=progress
stat -c '%s' "$bench_dir/payload.bin"
sha256sum "$bench_dir/payload.bin"
printf '%s\n' "$bench_dir/payload.bin"
```

Copy the reported path, size and hash. Zeros are acceptable because SSH
compression is explicitly disabled in both modes. Generation/checksum time
is not included in transfer time. Avoid creating the file during the test.

## Download from VM to the external SSH client through port forwarding

Run on the external client, not BF3. Pulling the file is the requested VM-to-
client data direction; PF incoming packets/ACKs also exercise reverse traffic.

```bash
python3 scp_benchmark.py \
  --remote ubuntu@161.246.6.38:/tmp/eswitch-perf.REPLACE/payload.bin \
  --port 3333 --label arm --bytes 2147483648 \
  --sha256 REPLACE_WITH_VM_HASH --runs 5 --output arm-1.csv
```

The script is in `application/eswitch_management/diagnostics/`; copy it to the
SSH client or run it from that directory on the client. It needs only Python 3
and the existing `scp` executable. It never changes BF3 configuration.

After switching only CT mode and restarting the daemon, repeat with
`--label ct --output ct-1.csv`. The script performs one uncounted warmup then
five measured downloads with new SSH connections. SHA/size validation occurs
after the timer stops. It uses a fresh temporary local directory for each run
and removes only its own downloaded payload. It refuses to overwrite CSVs.
Normal SSH host-key verification stays enabled; use `--identity` if needed.

For a single manual transfer on a Linux client:

```bash
/usr/bin/time -f 'elapsed=%e seconds' \
  scp -P 3333 -o Compression=no -o ControlMaster=no -o ControlPath=none \
  ubuntu@161.246.6.38:/tmp/eswitch-perf.REPLACE/payload.bin ./payload-test.bin
sha256sum ./payload-test.bin
```

Transfers under 30 seconds measure the initial offload interval; also use a
payload/connection lasting over 60 seconds to include the current 30-second
batch lease revocation/re-promotion cost. Do not hide that cost in results.

## Observe CPU and network alongside transfer time

On BF3, record the process ID and run `pidstat` if sysstat is installed:

```bash
pgrep -x eswitch-management
pidstat -u -p DAEMON_PID 1
```

Record aggregate per-core CPU as well (`mpstat -P ALL 1`), VM/client CPU,
link rates, retransmissions (`ss -ti` on VM/client), disk utilization and
before/after daemon counters. DPDK polling may keep process CPU near 100%
even when hardware is doing the forwarding: do not interpret CPU% alone as
proof of no benefit. Lower Arm packet-counter growth per transmitted GiB,
more throughput under contention and better latency under load are useful
evidence. Installed CT state alone is not a hardware packet-hit counter.

## Experimental order and interpretation

Use A-B-B-A blocks (five measured downloads per block), identical warmup and
quiet background traffic. Report median throughput/time and min/max or IQR;
do not select just the fastest run. Compute:

- speedup = median Arm elapsed / median CT elapsed;
- throughput = file bytes * 8 / elapsed / 1e9 Gbit/s;
- compare loss/retransmissions and daemon Arm packet deltas for the same bytes.

SCP measures end-to-end performance, including SSH encryption, disk and
client/VM scheduling. If one SSH core or disk is saturated, equal SCP times
do not disprove a networking improvement. Complement it with an isolated
iperf3 TCP test (normal and reverse direction) to remove file/encryption
bottlenecks. Test 1, 4, 8, 16 simultaneous sessions across several VMs; report
aggregate throughput and per-VM fairness. Do not exceed CT capacity without
labeling the run as an intentional mixed hardware/Arm capacity test.

An improvement is credible only when integrity, policy deny/revocation tests,
offload admission and sustained lease behavior all pass. If B falls back,
collect the stage/error log first; do not describe the run as hardware speedup.

## Rollback

Cleanly stop the daemon, restore `ESWITCH_HW_CT=0`, restart with the same
persistent config. Do not modify policy/topology between A and B except for
separate correctness tests. Remove the dedicated payload directory manually
only after confirming its exact path and that it holds test data only.
