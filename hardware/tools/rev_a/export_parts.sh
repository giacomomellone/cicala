#!/usr/bin/env bash
set -euo pipefail
case_dir=$(cd "$(dirname "$0")/../../rev_a/case" && pwd)
case_tools=$(cd "$(dirname "$0")/../case" && pwd)
openscad_bin=${OPENSCAD_BIN:-openscad}
openscad_command=("$openscad_bin")
if [[ $(uname -s) == Darwin && $(uname -m) == arm64 ]] &&
    arch -x86_64 "$openscad_bin" --version >/dev/null 2>&1; then
    openscad_command=(arch -x86_64 "$openscad_bin")
fi
out="$case_dir/exports"
mkdir -p "$out"
for part_name in top_shell base retainer button_stop category_cap next_cap lens steel_skin \
                 light_pipe coupon_buttons coupon_usb coupon_lens coupon_boss; do
    for format in stl 3mf; do
        "${openscad_command[@]}" --hardwarnings -D "part=\"$part_name\"" \
            -o "$out/$part_name.$format" "$case_dir/cicala_enclosure.scad"
    done
    python3 "$case_tools/check_mesh.py" "$out/$part_name.stl"
done
for part_name in lens_cut steel_cut; do
    "${openscad_command[@]}" --hardwarnings -D "part=\"$part_name\"" \
        -o "$out/$part_name.svg" "$case_dir/cicala_enclosure.scad"
done
echo "Exported thirteen STL/3MF parts and two full-size cutting outlines"
