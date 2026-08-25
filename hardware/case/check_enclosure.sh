#!/usr/bin/env bash
set -euo pipefail

case_dir=$(cd "$(dirname "$0")" && pwd)
source_file="$case_dir/cicala_enclosure.scad"
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
    light_pipe pcb_reference coupon_buttons coupon_buttons_assembly coupon_usb coupon_lens
    coupon_boss
)

for part_name in "${parts[@]}"; do
    "${openscad_command[@]}" \
        --hardwarnings \
        --check-parameters true \
        -D "part=\"$part_name\"" \
        -o "$task_tmp_dir/$part_name.stl" \
        "$source_file"
done

echo "OpenSCAD: ${#parts[@]} Rev A selectors rendered without warnings"
