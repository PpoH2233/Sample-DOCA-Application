#!/bin/bash
# Apache-2.0. Install this entry point AND dpu_eswitch.py on BlueField Arm.
set -eu
exec python3 "$(dirname "$(readlink -f "$0")")/dpu_eswitch.py" "$@"
