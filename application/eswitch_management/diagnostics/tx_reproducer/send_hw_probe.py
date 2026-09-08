#!/usr/bin/env python3
"""Run on the isolated Linux VM after HW PROBE READY; no Scapy dependency."""
import argparse
import socket
import struct
import time


def make_frame(source, destination, sequence):
    if len(source) != 6 or len(destination) != 6 or destination[0] & 1:
        raise ValueError("Expected six-byte MACs and unicast destination")
    return (destination + source + b"\x88\xb5" + b"ESWITCH-HW-PROBE" +
            struct.pack("!I", sequence)).ljust(60, b"\x00")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--interface", required=True)
    parser.add_argument("--gateway-mac", required=True)
    parser.add_argument("--expected-vm-mac", required=True)
    args = parser.parse_args()
    destination = bytes.fromhex(args.gateway_mac.replace(":", ""))
    expected = bytes.fromhex(args.expected_vm_mac.replace(":", ""))
    with socket.socket(socket.AF_PACKET, socket.SOCK_RAW, socket.htons(0x88b5)) as sock:
        sock.bind((args.interface, 0))
        source = sock.getsockname()[4]
        if source != expected:
            parser.error("Interface MAC does not match --expected-vm-mac; no frames sent")
        for sequence in range(1, 11):
            frame = make_frame(source, destination, sequence)
            sent = sock.send(frame)
            if sent != len(frame):
                raise RuntimeError(f"Short send: {sent}/{len(frame)}")
            print(f"HW GENERATOR: seq={sequence} bytes={sent}", flush=True)
            time.sleep(1)


if __name__ == "__main__":
    main()
