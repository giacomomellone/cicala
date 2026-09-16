#!/usr/bin/env bash
set -euo pipefail
pcb_dir=$(cd "$(dirname "$0")/../../rev_b/pcb" && pwd)
report_dir=${CICALA_PCB_REPORT_DIR:-build/hardware-rev-b-ci}
mkdir -p "$report_dir"
kicad_cli=${KICAD_CLI:-kicad-cli}
"$kicad_cli" sch erc --severity-all --exit-code-violations \
  -o "$report_dir/erc.rpt" "$pcb_dir/cicala_rev_b.kicad_sch"
"$kicad_cli" pcb drc --schematic-parity --refill-zones --severity-all \
  --exit-code-violations -o "$report_dir/drc.rpt" "$pcb_dir/cicala_rev_b.kicad_pcb"
