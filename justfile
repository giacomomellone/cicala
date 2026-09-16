# cicala — type `just` to see what you can do.
# One-time machine setup: `just setup` (plus `just fw-init` for firmware work)

set shell := ["bash", "-cu"]

python := ".venv/bin/python"
mkdocs := ".venv/bin/mkdocs"
ruff := ".venv/bin/ruff"
west := ".venv/bin/west"
prettier := "website/node_modules/.bin/prettier"
board := "esp32s3_devkitc/esp32s3/procpu"
hw := env("CICALA_HW", "breadboard")
export CICALA_HW := hw

# Rev A uses J1 native USB; the breadboard uses the DevKit UART bridge.
# Override the detected port when several boards are connected:
#   just port=/dev/cu.usbserial-0002 fw-flash

port := env("ESPTOOL_PORT", if hw == "rev_a" { shell("ls /dev/cu.usbmodem* /dev/ttyACM* 2>/dev/null | head -1 || true") } else { shell("ls /dev/cu.usbserial-* /dev/cu.SLAB_USBtoUART /dev/ttyUSB* 2>/dev/null | head -1 || true") })
portflag := if port == "" { "" } else { "--esp-device " + port }
monport := if port == "" { "" } else { "-p " + port }

# The native USB jack carries JTAG and the debug console.

usbport := env("CICALA_USB_PORT", shell("ls /dev/cu.usbmodem* /dev/ttyACM* 2>/dev/null | head -1 || true"))
usbmonport := if usbport == "" { "" } else { "-p " + usbport }

# Override the Zephyr SDK location when needed:
#   just sdk=~/somewhere/zephyr-sdk-1.0.1 fw-build

sdk := env("ZEPHYR_SDK_INSTALL_DIR", home_dir() / "Projects/zephyr-sdk-1.0.1")
export ZEPHYR_SDK_INSTALL_DIR := sdk

# Espressif's CMake integration finds esptool through PATH.

export PATH := justfile_directory() / ".venv/bin:" + env("PATH")

# qemu_xtensa runs on macOS. Linux users can select the faster native simulator:
#   just simboard=native_sim fw-test

simboard := "qemu_xtensa/dc233c"

# Zephyr's amd64 CI image runs native_sim tests on non-Linux hosts.

ci_image := "ghcr.io/zephyrproject-rtos/ci:latest"

# Pinned self-hosted Mermaid bundle for the docs site.

mermaid_version := "11.16.0"

[private]
default:
    @just --list --unsorted

# ---------------------------------------------------------------- setup

# one-time setup: python venv (tools + docs), website npm deps, git hooks
[group('setup')]
setup: hooks
    @if [ ! -d .venv ]; then (command -v python3.13 >/dev/null && python3.13 -m venv .venv) || python3 -m venv .venv; fi
    .venv/bin/pip install -q -r requirements.txt
    cd website && npm install
    @echo "ready — try: just validate · just website · just docs"
    @echo "for firmware work, also run: just fw-init"

# enable the git hooks (conventional-commit check on commit-msg)
[group('setup')]
hooks:
    git config core.hooksPath .githooks
    @echo "hooks enabled — commit messages are now checked (.githooks/commit-msg)"

# ---------------------------------------------------------------- database

# validate the question database (schema, dedup, denylist, origin refs)
[group('database')]
validate:
    {{ python }} tools/validate.py

# assign ids/dates to new questions and normalize file formatting
[group('database')]
fix:
    {{ python }} tools/validate.py --fix

# append plain-text drafts to a language corpus: just draft en drafts/en.txt
[group('database')]
draft lang +files:
    {{ python }} tools/import_drafts.py --lang {{ lang }} {{ files }}

# regenerate website/src/data/*.json from the database
[group('database')]
data:
    {{ python }} tools/build_site_data.py

# same payloads from website/placeholder-corpus/ — layout filler, not shipped content
[group('database')]
data-placeholder:
    {{ python }} tools/build_site_data.py --corpus website/placeholder-corpus

# build per-language device bundles + manifest (unsigned dev build)
[group('database')]
bundle:
    {{ python }} tools/build_bundle.py

# Arguments pass through, which is what reaches the second key:
#   just keygen --purpose firmware --out release-firmware-key.pem
# See docs/firmware_update.md for which key is which.

# generate an ed25519 signing keypair (see docs/sync_protocol.md)
[group('database')]
keygen *args:
    {{ python }} tools/keygen.py {{ args }}

# ----------------------------------------------------------------- website

# run the website dev server on the local network (rebuilds data payloads first)
[group('website')]
website: data
    cd website && npm run dev -- --host

# production build of the website into website/dist
[group('website')]
website-build: data
    cd website && npm run build

# The shipped corpora are empty, which leaves play, browse and deck with
# nothing to render. website/placeholder-corpus/ is layout filler, never
# shipped content. Both recipes below overwrite website/src/data/*.json, and
# plain `just website` / `just website-build` put the real payloads back.

# dev server against the placeholder corpus
[group('website')]
website-placeholder: data-placeholder
    cd website && npm run dev -- --host

# production build against the placeholder corpus, for UI work on an empty database
[group('website')]
website-build-placeholder: data-placeholder
    cd website && npm run build

# ---------------------------------------------------------------- firmware

# one-time: clone zephyr + modules into deps/ and fetch the espressif blobs
[group('firmware')]
fw-init:
    {{ python }} -m pip install -q west
    @if [ ! -d .west ]; then {{ west }} init -l firmware; else echo ".west/ exists — skipping init"; fi
    {{ west }} update
    # Install Zephyr's build and SoC-specific Python tools.
    {{ west }} packages pip --install
    {{ west }} blobs fetch hal_espressif
    just fw-patch
    just _west-build-dir build/esp32s3
    @echo
    @echo 'workspace ready. next: install the Zephyr SDK, then `just fw-doctor`'

# reapply the local Zephyr patches (run after any `west update`)
[group('firmware')]
fw-patch:
    @patch="{{ justfile_directory() }}/firmware/patches/zephyr/ssd16xx-preserve-image-on-init.patch"; \
     if git -C deps/zephyr apply --reverse --check "$patch" 2>/dev/null; then \
         echo "firmware patches already applied"; \
     else \
         {{ west }} patch -b patches -l patches.yml apply; \
     fi

# drop patches and local edits from the gitignored Zephyr tree
[group('firmware')]
fw-unpatch:
    {{ west }} patch -b patches -l patches.yml clean

# Set the build directory west uses when a command runs without -d. The
# Espressif monitor reads it to find the ELF for address decoding.
[private]
_west-build-dir dir:
    @{{ west }} config build.dir-fmt {{ dir }}

# print the toolchain/workspace state — run this when a build fails oddly
[group('firmware')]
fw-doctor:
    @echo "west:      $({{ west }} --version 2>/dev/null || echo 'MISSING — run just fw-init')"
    @echo "zephyr:    $(cd deps/zephyr 2>/dev/null && git describe --tags 2>/dev/null || echo 'MISSING — run just fw-init')"
    @want=$(cat deps/zephyr/SDK_VERSION 2>/dev/null); \
     [ -d "{{ sdk }}" ] && s="{{ sdk }}" || s="MISSING at {{ sdk }}"; \
     echo "sdk:       ${s}${want:+  (zephyr wants $want)}"
    @reg=$(cat ~/.cmake/packages/Zephyr-sdk/* 2>/dev/null | head -1); \
     echo "registered: ${reg:-NO — run '{{ sdk }}/setup.sh -c'}"
    @# Support both SDK directory layouts.
    @for pair in "esp32s3:xtensa-espressif_esp32s3_zephyr-elf" "dc233c:xtensa-dc233c_zephyr-elf"; do \
        short=${pair%%:*}; tc=${pair#*:}; \
        g=$(ls "{{ sdk }}"/gnu/$tc/bin/$tc-gdb "{{ sdk }}"/$tc/bin/$tc-gdb 2>/dev/null | head -1); \
        printf "%-11s%s\n" "$short:" "${g:-MISSING — {{ sdk }}/setup.sh -t $tc}"; \
     done
    @q=$(ls "{{ sdk }}"/hosttools/usr/bin/qemu-system-xtensa 2>/dev/null | head -1); \
     q=${q:-$(command -v qemu-system-xtensa 2>/dev/null)}; \
     echo "qemu:      ${q:-MISSING — comes with the SDK hosttools}"
    @echo "cmake:     $(cmake --version 2>/dev/null | head -1 || echo MISSING)"
    @case "$(cmake --version 2>/dev/null | head -1)" in *"version 4"*) \
        echo "           note: CMake 4.x drops compat with cmake_minimum_required < 3.5." ; \
        echo "           If a build dies there: export CMAKE_POLICY_VERSION_MINIMUM=3.5" ;; esac
    @echo "ninja:     $(ninja --version 2>/dev/null || echo MISSING)"
    @echo "dtc:       $(dtc --version 2>/dev/null || echo MISSING)"
    @echo "board:     {{ board }}"
    @echo "simboard:  {{ simboard }}"
    @echo "hardware:  {{ hw }}"
    @echo "port:      {{ if port == '' { 'auto (esptool probes every port, Bluetooth included)' } else { port } }}"
    @{{ python }} -m serial.tools.list_ports
    @# The USB jack carries JTAG and the debug console.
    @echo "usbport:   {{ if usbport == '' { 'none — USB jack not connected; no JTAG, no debug console' } else { usbport } }}"

# Build profiles keep measurement images isolated from the release build.
# Profiles: release, debug, charset, soak, retain, power, portal, corpus, bench,
# and ota. Bench requires a host. OTA requires one on its first build.

# Map a profile to its build directory. Rejects unknown profiles, so callers
# can rely on the name having been checked.
[private]
_fw-dir profile:
    #!/usr/bin/env bash
    set -euo pipefail
    case "{{ hw }}" in
        breadboard) target_dir="build/esp32s3" ;;
        rev_a) target_dir="build/cicala-rev-a" ;;
        *) echo "unknown hardware: {{ hw }} (breadboard or rev_a)" >&2; exit 2 ;;
    esac
    case "{{ profile }}" in
        release)
            echo "$target_dir"
            ;;
        debug|charset|soak|retain|power|portal|corpus|bench|ota)
            echo "$target_dir-{{ profile }}"
            ;;
        *)
            echo "unknown firmware profile: {{ profile }}" >&2
            echo "profiles: release debug charset soak retain power portal corpus bench ota" >&2
            exit 2
            ;;
    esac

# build a firmware profile: `just fw-build [profile] [host] [port]`
[group('firmware')]
fw-build profile="release" host="" port="8000": _fw-fixtures
    #!/usr/bin/env bash
    set -euo pipefail

    profile="{{ profile }}"
    host="{{ host }}"
    dir=$({{ just_executable() }} _fw-dir "$profile")
    sysbuild=false
    cmake=()
    extra_conf=()
    extra_overlay=()

    if [ "{{ hw }}" = rev_a ]; then
        extra_conf+=(rev_a.conf)
        extra_overlay+=(rev_a.overlay)
    fi

    case "$profile" in
        release)
            sysbuild=true
            ;;
        debug)
            extra_conf+=(debug.conf)
            extra_overlay+=(debug.overlay)
            ;;
        charset)
            cmake+=("-DCONFIG_CICALA_DEBUG_CHARSET=y")
            cmake+=("-DCONFIG_CICALA_SLEEP=n" "-DCONFIG_CICALA_PANEL_DEEP_SLEEP=n")
            ;;
        soak)
            extra_conf+=(soak.conf)
            cmake+=("-DCONFIG_CICALA_SLEEP=n" "-DCONFIG_CICALA_PANEL_DEEP_SLEEP=n")
            ;;
        retain)
            extra_conf+=(soak.conf)
            cmake+=("-DCONFIG_CICALA_SLEEP=n" "-DCONFIG_CICALA_PANEL_DEEP_SLEEP=n")
            cmake+=("-DCONFIG_CICALA_DEBUG_SOAK_REBOOT=y")
            ;;
        power)
            sysbuild=true
            cmake+=("-DCONFIG_CICALA_DEBUG_POWER=y")
            ;;
        portal)
            cmake+=("-DCONFIG_CICALA_DEBUG_PORTAL=y")
            ;;
        corpus)
            cmake+=("-DCONFIG_CICALA_DEBUG_CORPUS_STORE=y")
            ;;
        bench)
            if [ -z "$host" ]; then
                echo "bench profile requires the server host: just fw-build bench <host>" >&2
                exit 2
            fi
            cmake+=(
                "-DCONFIG_CICALA_SYNC_HOST=\"$host\""
                "-DCONFIG_CICALA_SYNC_PORT={{ port }}"
                "-DCONFIG_CICALA_SYNC_BASE_URL=\"http://$host:{{ port }}\""
            )
            extra_conf+=(bench.conf)
            ;;
        ota)
            sysbuild=true
            if [ -z "$host" ]; then
                if [ ! -d "$dir" ]; then
                    echo "ota profile requires the server host on its first build" >&2
                    echo "run: just fw-build ota <host>" >&2
                    exit 2
                fi
                exec {{ west }} build -d "$dir"
            fi
            cmake+=(
                "-DCONFIG_CICALA_OTA=y"
                "-DCONFIG_CICALA_SYNC_HOST=\"$host\""
                "-DCONFIG_CICALA_SYNC_PORT={{ port }}"
                "-DCONFIG_CICALA_SYNC_BASE_URL=\"http://$host:{{ port }}\""
                "-DCONFIG_CICALA_OTA_BASE_URL=\"http://$host:{{ port }}\""
            )
            extra_conf+=(bench.conf)
            ;;
    esac

    if $sysbuild; then
        if [ "{{ hw }}" = rev_a ]; then
            cmake+=("-Dmcuboot_EXTRA_DTC_OVERLAY_FILE=$PWD/firmware/app/rev_a_boot.overlay")
        fi
    else
        extra_conf+=(standalone.conf)
    fi
    if ((${#extra_conf[@]})); then
        cmake+=("-DEXTRA_CONF_FILE=$(IFS=';'; echo "${extra_conf[*]}")")
    fi
    if ((${#extra_overlay[@]})); then
        cmake+=("-DEXTRA_DTC_OVERLAY_FILE=$(IFS=';'; echo "${extra_overlay[*]}")")
    fi
    command=({{ west }} build -b "{{ board }}" firmware/app -d "$dir")
    if $sysbuild; then command+=(--sysbuild); fi
    if ((${#cmake[@]})); then command+=(-- "${cmake[@]}"); fi
    "${command[@]}"

# flash a firmware profile: `just fw-flash [profile] [host] [port]`
[group('firmware')]
fw-flash profile="release" host="" port="8000": (fw-build profile host port)
    #!/usr/bin/env bash
    set -euo pipefail
    dir=$({{ just_executable() }} _fw-dir "{{ profile }}")
    {{ west }} flash --no-rebuild -d "$dir" {{ portflag }}

# open Kconfig for the release application (q to quit, s to save)
[group('firmware')]
fw-menuconfig: (fw-build "release")
    {{ west }} build -t menuconfig -d "$({{ just_executable() }} _fw-dir release)" --domain app

# ------------------------------------------------------- firmware updates

# See docs/firmware_update.md for the complete OTA bench workflow.

# sign the built OTA image and write dist/firmware/firmware.json
[group('firmware')]
fw-ota-publish version="" key="signing_key.pem" host="" port="8000":
    #!/usr/bin/env bash
    set -euo pipefail
    v="{{ version }}"
    if [ -z "$v" ]; then
        v=$(sed -n 's/^VERSION_MAJOR *= *//p' firmware/app/VERSION).$(sed -n 's/^VERSION_MINOR *= *//p' firmware/app/VERSION).$(sed -n 's/^PATCHLEVEL *= *//p' firmware/app/VERSION)
    fi
    h="{{ host }}"
    if [ -z "$h" ]; then
        h=$(ipconfig getifaddr en0 2>/dev/null || hostname -I 2>/dev/null | awk '{print $1}')
    fi
    key=""
    if [ -f "{{ key }}" ]; then key="--sign-key {{ key }}"; fi
    dir=$({{ just_executable() }} _fw-dir ota)
    {{ python }} tools/build_firmware_manifest.py \
        "$dir/app/zephyr/zephyr.signed.bin" \
        --version "$v" --base-url "http://$h:{{ port }}" $key

# serve dist/firmware so a device on the same network can fetch it
[group('firmware')]
fw-ota-serve port="8000":
    @echo "serving dist/firmware on :{{ port }} — the device wants http://<this host>:{{ port }}/firmware.json"
    cd dist/firmware && {{ python }} -m http.server {{ port }}

# serial monitor for a build profile (ctrl-] to exit)
[group('firmware')]
fw-monitor profile="release":
    #!/usr/bin/env bash
    set -euo pipefail
    dir=$({{ just_executable() }} _fw-dir "{{ profile }}")
    {{ just_executable() }} _west-build-dir "$dir"
    {{ west }} espressif monitor {{ if profile == "debug" { usbmonport } else { monport } }}

[private]
_fw-debugserver:
    {{ west }} debugserver --no-rebuild -d "$({{ just_executable() }} _fw-dir debug)" \
        --config firmware/app/support/esp32s3_builtin_jtag.cfg

# OpenOCD gdb server on :3333 using the ESP32-S3 built-in USB-JTAG
[group('firmware')]
fw-debugserver: (fw-build "debug") _fw-debugserver

# flash the debug image and start its JTAG server
[group('firmware')]
fw-debug: (fw-flash "debug") _fw-debugserver

# run the application on the host under qemu (ctrl-a x to exit)
[group('firmware')]
fw-sim: _fw-fixtures
    {{ west }} build -b {{ simboard }} firmware/app -d build/sim
    {{ west }} build -t run -d build/sim

# run the debug image under qemu and wait for gdb on :1234
[group('firmware')]
fw-sim-debug: _fw-fixtures
    {{ west }} build -b {{ simboard }} firmware/app -d build/sim-debug -- \
        -DEXTRA_CONF_FILE=debug.conf
    {{ west }} build -t debugserver -d build/sim-debug

# delete all firmware build output
[group('firmware')]
fw-clean:
    rm -rf build/

# --------------------------------------------------------------- hardware

# Check Rev B electrical connectivity, routing and nominal mechanical fit.
[group('hardware')]
hw-rev-b-check:
    {{ python }} hardware/tools/rev_b/study.py

# Regenerate printable Rev B geometry. CAD_PYTHON must provide CadQuery.
[group('hardware')]
hw-rev-b-export *args:
    {{ python }} hardware/tools/rev_b/study.py --export {{ args }}

# Generate the prototype factory package after electrical and CAD checks.
[group('hardware')]
hw-rev-b-fab-export:
    {{ python }} hardware/tools/rev_b/export_fab.py

# Regenerate straight KiCad board renders and zoomable silkscreen drawings.
[group('hardware')]
hw-rev-b-pcb-renders:
    {{ python }} hardware/tools/rev_b/render_pcb.py

# Mechanical-only iterations explicitly skip electrical acceptance.
[group('hardware')]
hw-rev-b-fit-check:
    {{ python }} hardware/tools/rev_b/study.py --mechanical-only

[group('hardware')]
hw-rev-b-fit-export *args:
    {{ python }} hardware/tools/rev_b/study.py --export --mechanical-only {{ args }}

# validate every printable Rev A enclosure selector
[group('hardware')]
hw-case-check:
    hardware/tools/rev_a/check_enclosure.sh

# Regenerate printable files and sheet-material cutting outlines.
[group('hardware')]
hw-case-export:
    bash hardware/tools/rev_a/export_parts.sh

# validate the KiCad hierarchy, board skeleton and STEP export
[group('hardware')]
hw-pcb-check:
    hardware/tools/rev_a/check_rev_a.sh

# validate both Rev A hardware sources
[group('hardware')]
hw-check: hw-case-check hw-pcb-check

# Filled KiCad checks, independent placement/copper/USB audits and enclosure fit.
[group('hardware')]
hw-review report="build/hardware-review" *args:
    bash hardware/tools/rev_a/review_rev_a.sh "{{ report }}" {{ args }}

# Regenerate Gerber, drill and assembly outputs after the source review.
[group('hardware')]
hw-fab-export:
    bash hardware/tools/rev_a/export_fab.sh

# ------------------------------------------------------------------- tests

# everything that runs without hardware
[group('tests')]
test: validate test-tools test-website

# tools test suite (python unittest)
[group('tests')]
test-tools:
    {{ python }} -m unittest discover tools/tests

# website test suite (vitest)
[group('tests')]
test-website:
    cd website && npm test

# website end-to-end suite: builds the site and drives it in a real browser
[group('tests')]
test-e2e *args: data
    cd website && npx playwright install --with-deps chromium
    cd website && npx playwright test {{ args }}

# build the two bundle sets the firmware embeds: the fixture corpus the
# behaviour suites draw from, and the shipped corpus the corpus guards walk.
# firmware/tests/corpus/README.md explains which suite reads which.
# dist/bundles accumulates every past build, so copy the newest per language:
# glob order is alphabetical and a stale build can sort last.
[private]
_fw-fixtures: bundle
    {{ python }} tools/build_bundle.py --corpus firmware/tests/corpus --out dist/test-bundles
    mkdir -p firmware/tests/fixtures dist/corpus
    for dir in firmware/tests/corpus/*/; do \
        lang=$(basename "$dir"); \
        cp "$(ls -t dist/test-bundles/bundle-"$lang"-*.qdb | head -1)" "firmware/tests/fixtures/$lang.qdb"; \
    done
    for dir in questions/*/; do \
        lang=$(basename "$dir"); \
        [ "$lang" = "incubator" ] && continue; \
        cp "$(ls -t dist/bundles/bundle-"$lang"-*.qdb | head -1)" "dist/corpus/$lang.qdb"; \
    done
    @ls -l firmware/tests/fixtures/ dist/corpus/

# firmware suites under qemu (one suite: just fw-test input)
[group('tests')]
fw-test suite="": _fw-fixtures
    {{ west }} twister -T firmware/tests{{ if suite == "" { "" } else { "/" + suite } }} \
        -p {{ simboard }} --inline-logs -O build/twister

# firmware suites on native_sim in docker — the same image and platform CI uses
[group('tests')]
fw-test-linux suite="": _fw-fixtures
    docker run --rm --platform linux/amd64 -v "$PWD:/work" -w /work {{ ci_image }} \
        bash -lc "west twister -T firmware/tests{{ if suite == '' { '' } else { '/' + suite } }} \
        -p native_sim --inline-logs -O build/twister-linux"

# ---------------------------------------------------------------- formatting

# format everything (firmware, python, website, docs)
[group('format')]
fmt: fmt-fw fmt-py fmt-web fmt-docs

# check formatting without writing (what CI runs)
[group('format')]
fmt-check:
    git ls-files --cached --others --exclude-standard \
        '*.c' '*.cpp' '*.h' '*.hpp' | xargs -r clang-format --dry-run --Werror
    {{ ruff }} format --check tools/
    {{ prettier }} --check "website/**/*.{ts,astro,css,json}" "docs/**/*.md" "*.md"

# firmware C/C++ — --others picks up new files, --exclude-standard skips deps/
[group('format')]
fmt-fw:
    git ls-files --cached --others --exclude-standard \
        '*.c' '*.cpp' '*.h' '*.hpp' | xargs -r clang-format -i

# python tools
[group('format')]
fmt-py:
    {{ ruff }} format tools/

# lint python tools
[group('format')]
lint-py:
    {{ ruff }} check tools/

# website typescript / astro / css
[group('format')]
fmt-web:
    {{ prettier }} --write "website/**/*.{ts,astro,css,json}"

# markdown and workflow yaml (never questions/ — validate.py --fix owns those)
[group('format')]
fmt-docs:
    {{ prettier }} --write "docs/**/*.md" "*.md" ".github/**/*.yml"

# -------------------------------------------------------------------- docs

# fetch the self-hosted mermaid bundle (skipped if already present)
[group('docs')]
docs-deps:
    @mkdir -p docs/assets/javascripts
    @if [ ! -f docs/assets/javascripts/mermaid.min.js ]; then \
        echo "fetching mermaid {{ mermaid_version }}…"; \
        curl -sSL --fail -o docs/assets/javascripts/mermaid.min.js \
            "https://unpkg.com/mermaid@{{ mermaid_version }}/dist/mermaid.min.js"; \
    fi
    @grep -q 'globalThis\["mermaid"\]' docs/assets/javascripts/mermaid.min.js \
        || { echo "mermaid bundle does not set the global Material checks for"; exit 1; }

# live-preview the docs at http://127.0.0.1:8000
[group('docs')]
docs: docs-deps
    {{ mkdocs }} serve

# build the docs site into site/
[group('docs')]
docs-build: docs-deps
    {{ mkdocs }} build
