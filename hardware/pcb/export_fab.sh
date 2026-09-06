#!/usr/bin/env bash
# Fabrication and assembly outputs for the Rev A board.
#
# Writes into cicala_rev_a/exports/. Everything here is generated; the KiCad
# project is the source. Run hardware/pcb/check_rev_a.sh first: this script
# does not gate on DRC.
set -euo pipefail

pcb_dir=$(cd "$(dirname "$0")/cicala_rev_a" && pwd)
out="$pcb_dir/exports"
board="$pcb_dir/cicala_rev_a.kicad_pcb"
sch="$pcb_dir/cicala_rev_a.kicad_sch"

if [[ -n "${KICAD_CLI_BIN:-}" ]]; then
    kicad_cli="$KICAD_CLI_BIN"
elif command -v kicad-cli >/dev/null 2>&1; then
    kicad_cli=$(command -v kicad-cli)
elif [[ -x /Applications/KiCad/KiCad.app/Contents/MacOS/kicad-cli ]]; then
    kicad_cli=/Applications/KiCad/KiCad.app/Contents/MacOS/kicad-cli
else
    echo "KiCad CLI is required; set KICAD_CLI_BIN when it is not on PATH" >&2
    exit 1
fi

task_tmp_dir=$(mktemp -d "${TMPDIR:-/tmp}/cicala-fab.XXXXXX")
trap 'rm -rf "$task_tmp_dir"' EXIT
export XDG_CACHE_HOME="$task_tmp_dir/cache"
mkdir -p "$XDG_CACHE_HOME"
fontconfig_file="$task_tmp_dir/fonts.conf"
cat >"$fontconfig_file" <<FC
<?xml version="1.0"?>
<fontconfig>
  <dir>/System/Library/Fonts</dir>
  <dir>/Library/Fonts</dir>
  <cachedir>$task_tmp_dir/font-cache</cachedir>
</fontconfig>
FC
export FONTCONFIG_FILE="$fontconfig_file"

rm -rf "$out/gerbers" "$out/drill"
mkdir -p "$out/gerbers" "$out/drill" "$out/assembly"

"$kicad_cli" pcb export gerbers \
    --no-protel-ext --subtract-soldermask --use-drill-file-origin \
    --layers F.Cu,In1.Cu,In2.Cu,B.Cu,F.Paste,B.Paste,F.SilkS,B.SilkS,F.Mask,B.Mask,Edge.Cuts \
    -o "$out/gerbers/" "$board"

"$kicad_cli" pcb export drill \
    --format excellon --drill-origin plot --excellon-separate-th \
    --generate-map --map-format gerberx2 \
    -o "$out/drill/" "$board"

"$kicad_cli" pcb export pos --format csv --units mm --side both \
    --use-drill-file-origin -o "$out/assembly/cicala_rev_a_pos.csv" "$board"

"$kicad_cli" sch export bom --exclude-dnp \
    --fields "Reference,Value,Footprint,MPN,Manufacturer,QUANTITY,DNP" \
    --labels "Refs,Value,Footprint,MPN,Manufacturer,Qty,DNP" \
    --group-by "Value,Footprint,MPN" \
    -o "$out/assembly/cicala_rev_a_bom.csv" "$sch"

"$kicad_cli" pcb export pdf --layers F.Fab,F.SilkS,Edge.Cuts \
    --mode-single --scale 0 --black-and-white --exclude-value \
    -o "$out/assembly/assembly_top.pdf" "$board"
"$kicad_cli" pcb export pdf --layers B.Fab,B.SilkS,Edge.Cuts --mirror \
    --mode-single --scale 0 --black-and-white --exclude-value \
    -o "$out/assembly/assembly_bottom.pdf" "$board"

"$kicad_cli" pcb export ipc2581 --units mm \
    -o "$out/cicala_rev_a.zip" "$board"

"$kicad_cli" pcb export step --subst-models --no-unspecified \
    -o "$out/cicala_rev_a_board.step" "$board"

echo "Fabrication outputs written to hardware/pcb/cicala_rev_a/exports/"
echo "  gerbers/    four copper layers, paste, silk, mask and the board outline"
echo "  drill/      Excellon, plated and non-plated separately, with maps"
echo "  assembly/   pick-and-place, BOM and both assembly drawings"
echo "  cicala_rev_a.zip        IPC-2581 for assemblers that prefer it"
echo "  cicala_rev_a_board.step 3D model for the enclosure fit check"
