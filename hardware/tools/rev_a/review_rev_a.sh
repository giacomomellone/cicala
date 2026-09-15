#!/usr/bin/env bash
# Re-run digital hardware validation against the authoritative sources.
set -euo pipefail
repo=$(cd "$(dirname "$0")/../../.." && pwd)
cd "$repo"
report=${1:-build/hardware-review}
if (($#)); then shift; fi
mkdir -p "$report"
report=$(cd "$report" && pwd)
project="$repo/hardware/rev_a/pcb"
analysis_python=${CICALA_HW_PYTHON:-python3}
kicad_python=${KICAD_PYTHON_BIN:-python3}
models=${KICAD10_3DMODEL_DIR:-/usr/share/kicad/3dmodels}
if [[ $(uname -s) == Darwin ]]; then
    bundle=/Applications/KiCad/KiCad.app/Contents
    kicad_python=${KICAD_PYTHON_BIN:-$bundle/Frameworks/Python.framework/Versions/Current/bin/python3}
    models=${KICAD10_3DMODEL_DIR:-$bundle/SharedSupport/3dmodels}
fi
export CICALA_PCB_REPORT_DIR="$report"
bash hardware/tools/rev_a/check_rev_a.sh
"$kicad_python" hardware/tools/pcb/export_geom.py \
    "$project/cicala_rev_a.kicad_pcb" "$report/geometry.json"
"$analysis_python" hardware/tools/pcb/audit_placement.py "$report/geometry.json" \
    --board "$project/cicala_rev_a.kicad_pcb" --models-root "$models" \
    --output "$report/placement_audit.json"
"$analysis_python" hardware/tools/pcb/audit_copper.py "$report/geometry.json" \
    --output "$report/copper_audit.json"
"$analysis_python" hardware/tools/pcb/audit_usb.py "$report/geometry.json" \
    "$project/cicala_rev_a.kicad_pcb" "$report/usb_audit.json"
"$analysis_python" hardware/tools/pcb/audit_temperature.py \
    "$report/cicala_rev_a.net" > "$report/temperature_audit.json"
"$analysis_python" hardware/tools/pcb/check_rev_a_contract.py "$report/geometry.json" \
    --output "$report/contract.json" "$@"
bash hardware/tools/rev_a/check_enclosure.sh
echo "Digital hardware review passed; reports: $report"
