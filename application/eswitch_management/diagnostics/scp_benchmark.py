#!/usr/bin/env python3
"""Repeat a PF download with a fresh SSH connection; no daemon/config changes."""
import argparse
import csv
import hashlib
from pathlib import Path
import subprocess
import tempfile
import time


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--remote", required=True, help="user@public-IP:/absolute/file")
    parser.add_argument("--port", type=int, default=3333)
    parser.add_argument("--label", required=True, help="arm or ct; descriptive only")
    parser.add_argument("--bytes", type=int, required=True, dest="expected_bytes")
    parser.add_argument("--sha256", required=True)
    parser.add_argument("--runs", type=int, default=5)
    parser.add_argument("--identity", type=Path)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    if args.runs < 1 or args.expected_bytes < 1 or not 1 <= args.port <= 65535:
        parser.error("runs, bytes and port must be positive/in range")
    if len(args.sha256) != 64 or any(c not in "0123456789abcdefABCDEF" for c in args.sha256):
        parser.error("sha256 must be a 64-character hex digest")
    command = ["scp", "-P", str(args.port), "-o", "Compression=no",
               "-o", "ControlMaster=no", "-o", "ControlPath=none"]
    if args.identity:
        command.extend(["-i", str(args.identity)])
    # Refuse to overwrite an earlier result. Host-key verification stays on.
    with args.output.open("x", newline="") as results:
        writer = csv.writer(results)
        writer.writerow(["label", "run", "warmup", "bytes", "seconds", "MiB_s", "Gbit_s", "sha256_ok"])
        for run in range(args.runs + 1):
            with tempfile.TemporaryDirectory(prefix="eswitch-scp-") as directory:
                destination = Path(directory) / "payload.bin"
                started = time.perf_counter()
                subprocess.run(command + [args.remote, str(destination)], check=True)
                elapsed = time.perf_counter() - started
                size = destination.stat().st_size
                digest = hashlib.sha256()
                with destination.open("rb") as payload:
                    for chunk in iter(lambda: payload.read(1024 * 1024), b""):
                        digest.update(chunk)
                valid = size == args.expected_bytes and digest.hexdigest() == args.sha256.lower()
                writer.writerow([args.label, run, int(run == 0), size,
                                 f"{elapsed:.6f}", f"{size / elapsed / 2**20:.3f}",
                                 f"{size * 8 / elapsed / 1e9:.6f}", int(valid)])
                results.flush()
                if not valid:
                    raise RuntimeError("download size/checksum mismatch; reject this benchmark")
                print(f"{args.label} run={run} warmup={run == 0} seconds={elapsed:.3f} MiB/s={size / elapsed / 2**20:.2f}")


if __name__ == "__main__":
    main()
