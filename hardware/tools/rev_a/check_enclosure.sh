#!/usr/bin/env bash
set -euo pipefail

case_dir=$(cd "$(dirname "$0")/../../rev_a/case" && pwd)
case_tools=$(cd "$(dirname "$0")/../case" && pwd)
source_file="$case_dir/cicala_enclosure.scad"
pcb_z=$(python3 - "$source_file" <<'PY'
from pathlib import Path
import re
import sys
match = re.search(r'^pcb_z\s*=\s*([\d.]+);', Path(sys.argv[1]).read_text(), re.M)
if not match:
    raise SystemExit('Expected an explicit PCB mounting height in the enclosure')
print(match[1])
PY
)
python3 "$case_tools/export_component_bounds.py" \
    "$case_dir/../pcb/cicala_rev_a.kicad_pcb" \
    "$case_dir/pcb_component_bounds.scad" --check --pcb-z "$pcb_z"
task_tmp_dir=$(mktemp -d "${TMPDIR:-/tmp}/cicala-case-check.XXXXXX")
trap 'rm -rf "$task_tmp_dir"' EXIT

if [[ -n "${OPENSCAD_BIN:-}" ]]; then
    openscad_bin="$OPENSCAD_BIN"
elif command -v openscad >/dev/null 2>&1; then
    openscad_bin=$(command -v openscad)
elif [[ -x /Applications/OpenSCAD.app/Contents/MacOS/OpenSCAD ]]; then
    openscad_bin=/Applications/OpenSCAD.app/Contents/MacOS/OpenSCAD
else
    echo "OpenSCAD is required; set OPENSCAD_BIN when it is not on PATH" >&2
    exit 1
fi

openscad_command=("$openscad_bin")
if [[ $(uname -s) == Darwin && $(uname -m) == arm64 ]] && \
    arch -x86_64 "$openscad_bin" --version >/dev/null 2>&1; then
    openscad_command=(arch -x86_64 "$openscad_bin")
fi

parts=(
    assembly exploded section top_shell base retainer category_cap next_cap lens steel_skin
    light_pipe button_stop pcb_reference coupon_buttons coupon_buttons_assembly coupon_usb coupon_lens
    coupon_boss
)

for part_name in "${parts[@]}"; do
    "${openscad_command[@]}" \
        --hardwarnings \
        --check-parameters true \
        -D "part=\"$part_name\"" \
        -o "$task_tmp_dir/$part_name.stl" \
        "$source_file"
    case "$part_name" in
        assembly|exploded|section|pcb_reference|coupon_buttons_assembly) ;;
        *) python3 "$case_tools/check_mesh.py" "$task_tmp_dir/$part_name.stl" ;;
    esac
done

fit_checks=(pcb_panel pcb_retainer panel_retainer panel_shell jst_base usb_plug
            flex_shell flex_retainer components_case components_cell
            harness_case harness_components button_stop caps_motion)
for fit_check in "${fit_checks[@]}"; do
    fit_log="$task_tmp_dir/fit_$fit_check.log"
    if "${openscad_command[@]}" --hardwarnings \
        -D "part=\"fit_$fit_check\"" \
        -o "$task_tmp_dir/fit_$fit_check.stl" "$source_file" >"$fit_log" 2>&1; then
        echo "Unexpected solid intersection: $fit_check" >&2
        exit 1
    fi
    python3 - "$fit_log" <<'PY'
from pathlib import Path
import sys
log = Path(sys.argv[1]).read_text()
if "Current top level object is empty." not in log or "ERROR:" in log or "WARNING:" in log:
    raise SystemExit(log)
PY
done

"${openscad_command[@]}" --hardwarnings -D 'part="fit_button_stop_contact"' \
    -o "$task_tmp_dir/button_stop_contact.stl" "$source_file" \
    >"$task_tmp_dir/button_stop_contact.log" 2>&1
python3 - "$case_tools" "$task_tmp_dir/button_stop_contact.stl" <<'PY'
import sys
sys.path.insert(0, sys.argv[1])
from check_mesh import check
check(sys.argv[2], expected_solids=None)
PY

echo "OpenSCAD: ${#parts[@]} selectors, printable meshes, ${#fit_checks[@]} empty fit checks and positive downward-stop contact passed"
