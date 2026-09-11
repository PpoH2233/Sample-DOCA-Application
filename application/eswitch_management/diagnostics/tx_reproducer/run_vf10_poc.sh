#!/usr/bin/env bash
set -euo pipefail

usage() {
	cat <<'EOF'
Usage: sudo ./run_vf10_poc.sh \
  --vm-mac <mac> --gateway-mac <mac> \
  --vm-ip <ipv4> --gateway-ip <ipv4> \
  --confirm-exclusive-owner

Options:
  --parent-bdf <bdf>    Parent PF BDF (default: 03:00.0)
  --binary <path>       Reproducer binary
  --log <path>          Log file (default: /tmp/eswitch-vf10-poc.log)
  --lcore <id>          DPDK lcore (default: 0)

This diagnostic temporarily owns the selected eSwitch and installs root
steering rules. Stop the management daemon, OVS-DPDK, and every other DPDK/
DOCA Flow owner of the parent PF before supplying --confirm-exclusive-owner.
The script never changes firmware, creates VFs, changes switchdev mode, or
stops services automatically.
EOF
}

parent_bdf=03:00.0
binary=/build/eswitch-tx-reproducer/eswitch-tx-reproducer
log=/tmp/eswitch-vf10-poc.log
lcore=0
vm_mac=
gateway_mac=
vm_ip=
gateway_ip=
exclusive_owner=false

while (($#)); do
	case "$1" in
		--parent-bdf) parent_bdf=${2:?missing value}; shift 2 ;;
		--binary) binary=${2:?missing value}; shift 2 ;;
		--log) log=${2:?missing value}; shift 2 ;;
		--lcore) lcore=${2:?missing value}; shift 2 ;;
		--vm-mac) vm_mac=${2:?missing value}; shift 2 ;;
		--gateway-mac) gateway_mac=${2:?missing value}; shift 2 ;;
		--vm-ip) vm_ip=${2:?missing value}; shift 2 ;;
		--gateway-ip) gateway_ip=${2:?missing value}; shift 2 ;;
		--confirm-exclusive-owner) exclusive_owner=true; shift ;;
		-h|--help) usage; exit 0 ;;
		*) printf 'Unknown option: %s\n' "$1" >&2; usage >&2; exit 2 ;;
	esac
done

if [[ -z "$vm_mac" || -z "$gateway_mac" || -z "$vm_ip" || -z "$gateway_ip" ]]; then
	printf 'ERROR: VM/gateway MAC and IPv4 arguments are all required.\n' >&2
	usage >&2
	exit 2
fi
if [[ "$exclusive_owner" != true ]]; then
	printf 'ERROR: stop every other owner of the PF, then pass --confirm-exclusive-owner.\n' >&2
	exit 2
fi
if ((EUID != 0)); then
	printf 'ERROR: run as root; DOCA/DPDK device access requires it.\n' >&2
	exit 2
fi
if [[ ! -x "$binary" ]]; then
	printf 'ERROR: binary is not executable: %s\n' "$binary" >&2
	printf 'Build it with Meson as documented in README.md.\n' >&2
	exit 2
fi

for module in doca-common doca-flow; do
	if ! version=$(pkg-config --modversion "$module"); then
		printf 'ERROR: pkg-config cannot resolve %s.\n' "$module" >&2
		exit 2
	fi
	printf 'PREFLIGHT: %s=%s\n' "$module" "$version"
done

full_bdf=$parent_bdf
[[ "$full_bdf" == 0000:* ]] || full_bdf=0000:$full_bdf
short_bdf=${full_bdf#0000:}

if ! eswitch_state=$(devlink dev eswitch show "pci/$full_bdf" 2>&1); then
	printf 'ERROR: cannot query eSwitch pci/%s: %s\n' "$full_bdf" "$eswitch_state" >&2
	exit 2
fi
printf 'PREFLIGHT: %s\n' "$eswitch_state"
if [[ "$eswitch_state" != *"mode switchdev"* ]]; then
	printf 'ERROR: pci/%s is not in switchdev mode; no setting was changed.\n' "$full_bdf" >&2
	exit 2
fi

printf 'PREFLIGHT: target selector=pci/%s,c1pf0vf10\n' "$short_bdf"
printf 'PREFLIGHT: the binary must print host=1 pf=0 vf=10 before programming TX.\n'
printf 'PREFLIGHT: start this capture inside the VM before TX begins:\n'
printf "  sudo tcpdump -Q in -eni <vm-iface> -nn -vv 'arp and ether src %s'\n" "$gateway_mac"

set +e
TX_PROBE_PATH=sw \
TX_PROBE_VF=10 \
TX_PROBE_VM_MAC=$vm_mac \
TX_PROBE_GATEWAY_MAC=$gateway_mac \
TX_PROBE_VM_IP=$vm_ip \
TX_PROBE_GATEWAY_IP=$gateway_ip \
"$binary" \
	-l "$lcore" --file-prefix=eswitch-vf10-poc -- \
	--rep "pci/$short_bdf,c1pf0vf10" --expert-mode \
	--log-level 60 --sdk-log-level 60 2>&1 | tee "$log"
probe_rc=${PIPESTATUS[0]}
set -e

printf '\nRESULT SUMMARY (%s):\n' "$log"
grep -E 'PORT MAP|PROBE CONFIG|PROBE SUMMARY|CHECK FAILED|PROBE ERROR' "$log" || true
exit "$probe_rc"
