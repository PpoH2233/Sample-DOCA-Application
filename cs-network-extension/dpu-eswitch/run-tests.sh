#!/bin/bash
# Apache-2.0. Isolated offline adapter tests and fault injection.
set -eu
cd "$(dirname "$(readlink -f "$0")")"
exec python3 -m unittest -v test_adapter
