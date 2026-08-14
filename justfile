# tischkarte — type `just` to see what you can do.
# One-time machine setup: `just setup` (plus `just fw-init` for firmware work)

set shell := ["bash", "-cu"]

python := ".venv/bin/python"
mkdocs := ".venv/bin/mkdocs"
ruff := ".venv/bin/ruff"
west := ".venv/bin/west"
prettier := "website/node_modules/.bin/prettier"
board := "esp32s3_devkitc/esp32s3/procpu"

# The UART jack carries the console and flashing traffic. Override the detected
# port when several boards are connected:
#   just port=/dev/cu.usbserial-0002 fw-flash

port := env("ESPTOOL_PORT", shell("ls /dev/cu.usbserial-* /dev/cu.SLAB_USBtoUART 2>/dev/null | head -1 || true"))
portflag := if port == "" { "" } else { "--esp-device " + port }
monport := if port == "" { "" } else { "-p " + port }

# The native USB jack carries JTAG and the debug console.

usbport := env("TK_USB_PORT", shell("ls /dev/cu.usbmodem* 2>/dev/null | head -1 || true"))
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

# regenerate website/src/data/*.json from the database
[group('database')]
data:
    {{ python }} tools/build_site_data.py

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

# run the website dev server (rebuilds data payloads first)
[group('website')]
website: data
    cd website && npm run dev

# production build of the website into website/dist
[group('website')]
website-build: data
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
    @echo "workspace ready. next: install the Zephyr SDK, then `just fw-doctor`"

# reapply the local Zephyr patches (run after any `west update`)
[group('firmware')]
fw-patch:
    {{ west }} patch -b patches -l patches.yml apply

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
    @# The UART jack appears as cu.usbserial-*.
    @echo "port:      {{ if port == '' { 'auto (esptool probes every port, Bluetooth included)' } else { port } }}"
    @p=$(ls /dev/cu.usbserial-* /dev/cu.SLAB_USBtoUART /dev/tty.usbserial-* 2>/dev/null | tr '\n' ' '); \
     echo "  detected: ${p:-none — is the cable in the UART jack, not the USB one?}"
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
    case "{{ profile }}" in
        release)
            echo "build/esp32s3"
            ;;
        debug|charset|soak|retain|power|portal|corpus|bench|ota)
            echo "build/esp32s3-{{ profile }}"
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

    case "$profile" in
        release)
            sysbuild=true
            ;;
        debug)
            cmake+=("-DEXTRA_CONF_FILE=debug.conf" "-DEXTRA_DTC_OVERLAY_FILE=debug.overlay")
            ;;
        charset)
            cmake+=("-DCONFIG_TK_DEBUG_CHARSET=y")
            ;;
        soak)
            cmake+=("-DEXTRA_CONF_FILE=soak.conf")
            ;;
        retain)
            cmake+=("-DEXTRA_CONF_FILE=soak.conf" "-DCONFIG_TK_DEBUG_SOAK_REBOOT=y")
            ;;
        power)
            sysbuild=true
            cmake+=("-DCONFIG_TK_DEBUG_POWER=y")
            ;;
        portal)
            cmake+=("-DCONFIG_TK_DEBUG_PORTAL=y")
            ;;
        corpus)
            cmake+=("-DCONFIG_TK_DEBUG_CORPUS_STORE=y")
            ;;
        bench)
            if [ -z "$host" ]; then
                echo "bench profile requires the server host: just fw-build bench <host>" >&2
                exit 2
            fi
            cmake+=(
                "-DEXTRA_CONF_FILE=bench.conf"
                "-DCONFIG_TK_SYNC_HOST=\"$host\""
                "-DCONFIG_TK_SYNC_PORT={{ port }}"
                "-DCONFIG_TK_SYNC_BASE_URL=\"http://$host:{{ port }}\""
            )
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
                "-DEXTRA_CONF_FILE=bench.conf"
                "-DCONFIG_TK_OTA=y"
                "-DCONFIG_TK_SYNC_HOST=\"$host\""
                "-DCONFIG_TK_SYNC_PORT={{ port }}"
                "-DCONFIG_TK_SYNC_BASE_URL=\"http://$host:{{ port }}\""
                "-DCONFIG_TK_OTA_BASE_URL=\"http://$host:{{ port }}\""
            )
            ;;
    esac

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
    {{ west }} build -t menuconfig -d build/esp32s3 --domain app

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
    {{ python }} tools/build_firmware_manifest.py \
        build/esp32s3-ota/app/zephyr/zephyr.signed.bin \
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
    {{ west }} debugserver --no-rebuild -d build/esp32s3-debug \
        --config firmware/app/support/esp32s3_builtin_jtag.cfg

# OpenOCD gdb server on :3333 for the devkit (built-in USB-JTAG)
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

# copy the real question bundles into firmware/tests/fixtures/
[private]
_fw-fixtures: bundle
    mkdir -p firmware/tests/fixtures
    for f in dist/bundles/bundle-*.qdb; do \
        lang=$(basename "$f" | cut -d- -f2); \
        cp "$f" "firmware/tests/fixtures/$lang.qdb"; \
    done
    @ls -l firmware/tests/fixtures/

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
