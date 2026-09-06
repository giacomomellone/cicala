#!/usr/bin/env bash
set -euo pipefail

pcb_dir=$(cd "$(dirname "$0")/cicala_rev_a" && pwd)
task_tmp_dir=$(mktemp -d "${TMPDIR:-/tmp}/cicala-pcb-check.XXXXXX")
trap 'rm -rf "$task_tmp_dir"' EXIT

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

# Reports normally die with the temp dir. CI sets this so a failing run can
# publish the ERC and DRC detail instead of only the violation count.
if [[ -n "${CICALA_PCB_REPORT_DIR:-}" ]]; then
    mkdir -p "$CICALA_PCB_REPORT_DIR"
    report_dir=$(cd "$CICALA_PCB_REPORT_DIR" && pwd)
else
    report_dir="$task_tmp_dir"
fi

export XDG_CACHE_HOME="$task_tmp_dir/cache"
mkdir -p "$XDG_CACHE_HOME"
fontconfig_file="$task_tmp_dir/fonts.conf"
cat >"$fontconfig_file" <<EOF
<?xml version="1.0"?>
<fontconfig>
  <dir>/System/Library/Fonts</dir>
  <dir>/Library/Fonts</dir>
  <cachedir>$task_tmp_dir/font-cache</cachedir>
</fontconfig>
EOF
export FONTCONFIG_FILE="$fontconfig_file"

"$kicad_cli" sch erc \
    --exit-code-violations \
    -o "$report_dir/erc.rpt" \
    "$pcb_dir/cicala_rev_a.kicad_sch"

"$kicad_cli" sch export bom \
    --exclude-dnp \
    --fields "Reference,Value,Footprint,MPN,QUANTITY,DNP" \
    --labels "Refs,Value,Footprint,MPN,Qty,DNP" \
    -o "$report_dir/bom.csv" \
    "$pcb_dir/cicala_rev_a.kicad_sch"

"$kicad_cli" pcb drc \
    --schematic-parity --refill-zones \
    --exit-code-violations \
    -o "$report_dir/drc.rpt" \
    "$pcb_dir/cicala_rev_a.kicad_pcb"

touch "$report_dir/cicala_rev_a_board.step"
"$kicad_cli" pcb export step \
    -o "$report_dir/cicala_rev_a_board.step" \
    "$pcb_dir/cicala_rev_a.kicad_pcb"

echo "KiCad: schematic ERC, BOM export, filled-board DRC, schematic parity and STEP export passed"
