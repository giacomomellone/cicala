# tischkarte — type `just` to see what you can do.
# One-time machine setup: `just setup` (plus `just fw-init` for firmware work)

set shell := ["bash", "-cu"]

python   := ".venv/bin/python"
mkdocs   := ".venv/bin/mkdocs"
ruff     := ".venv/bin/ruff"
west     := ".venv/bin/west"
prettier := "website/node_modules/.bin/prettier"

board    := "esp32s3_devkitc/esp32s3/procpu"

# Zephyr SDK. It lives outside the default search paths (~, /opt, /usr/local),
# so exporting this is what lets a build find it; `setup.sh -c` additionally
# registers it in the CMake package registry. Override for another location:
#   just sdk=~/somewhere/zephyr-sdk-1.0.1 fw-build
sdk := env("ZEPHYR_SDK_INSTALL_DIR", home_dir() / "Projects/zephyr-sdk-1.0.1")
export ZEPHYR_SDK_INSTALL_DIR := sdk

# Zephyr's espressif SoC CMake looks for esptool on PATH, not in the venv, so
# a plain `west build` fails with "esptool>=5.0.2 not found in PATH" even with
# it installed. Putting .venv/bin first also covers west, ruff and mkdocs.
export PATH := justfile_directory() / ".venv/bin:" + env("PATH")

# Host test platform. native_sim is faster and is the only one that emulates
# GPIO, but it builds on Linux only — so qemu_xtensa (full kernel, target
# architecture, runs on macOS) is the default. Override for a Linux host:
#   just simboard=native_sim fw-test
simboard := "qemu_xtensa/dc233c"

# Zephyr's CI image, for the native_sim suites on a non-Linux host.
# Note: amd64 only, so it runs emulated on Apple Silicon.
ci_image := "ghcr.io/zephyrproject-rtos/ci:latest"

# Self-hosted so the docs site loads nothing from a CDN, matching `font: false`
# in mkdocs.yml. Pinned: Material's loader expects the global this build sets.
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
    {{python}} tools/validate.py

# assign ids/dates to new questions and normalize file formatting
[group('database')]
fix:
    {{python}} tools/validate.py --fix

# regenerate website/src/data/*.json from the database
[group('database')]
data:
    {{python}} tools/build_site_data.py

# build per-language device bundles + manifest (unsigned dev build)
[group('database')]
bundle:
    {{python}} tools/build_bundle.py

# generate the ed25519 bundle-signing keypair (see docs/sync_protocol.md)
[group('database')]
keygen:
    {{python}} tools/keygen.py

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
    {{python}} -m pip install -q west
    @if [ ! -d .west ]; then {{west}} init -l firmware; else echo ".west/ exists — skipping init"; fi
    {{west}} update
    # Python packages for zephyr and every enabled module. This covers both
    # the build scripts (pyelftools, pykwalify…) and per-SoC tools — esptool
    # comes from here, and cmake aborts without it.
    {{west}} packages pip --install
    {{west}} blobs fetch hal_espressif
    @echo
    @echo "workspace ready. next: install the Zephyr SDK, then `just fw-doctor`"

# print the toolchain/workspace state — run this when a build fails oddly
[group('firmware')]
fw-doctor:
    @echo "west:      $({{west}} --version 2>/dev/null || echo 'MISSING — run just fw-init')"
    @echo "zephyr:    $(cd deps/zephyr 2>/dev/null && git describe --tags 2>/dev/null || echo 'MISSING — run just fw-init')"
    @want=$(cat deps/zephyr/SDK_VERSION 2>/dev/null); \
     [ -d "{{sdk}}" ] && s="{{sdk}}" || s="MISSING at {{sdk}}"; \
     echo "sdk:       ${s}${want:+  (zephyr wants $want)}"
    @reg=$(cat ~/.cmake/packages/Zephyr-sdk/* 2>/dev/null | head -1); \
     echo "registered: ${reg:-NO — run '{{sdk}}/setup.sh -c'}"
    @# SDK 1.0 restructured: toolchains under gnu/, host tools under hosttools/.
    @for pair in "esp32s3:xtensa-espressif_esp32s3_zephyr-elf" "dc233c:xtensa-dc233c_zephyr-elf"; do \
        short=${pair%%:*}; tc=${pair#*:}; \
        g=$(ls "{{sdk}}"/gnu/$tc/bin/$tc-gdb "{{sdk}}"/$tc/bin/$tc-gdb 2>/dev/null | head -1); \
        printf "%-11s%s\n" "$short:" "${g:-MISSING — {{sdk}}/setup.sh -t $tc}"; \
     done
    @q=$(ls "{{sdk}}"/hosttools/usr/bin/qemu-system-xtensa 2>/dev/null | head -1); \
     q=${q:-$(command -v qemu-system-xtensa 2>/dev/null)}; \
     echo "qemu:      ${q:-MISSING — comes with the SDK hosttools}"
    @echo "cmake:     $(cmake --version 2>/dev/null | head -1 || echo MISSING)"
    @case "$(cmake --version 2>/dev/null | head -1)" in *"version 4"*) \
        echo "           note: CMake 4.x drops compat with cmake_minimum_required < 3.5." ; \
        echo "           If a build dies there: export CMAKE_POLICY_VERSION_MINIMUM=3.5" ;; esac
    @echo "ninja:     $(ninja --version 2>/dev/null || echo MISSING)"
    @echo "dtc:       $(dtc --version 2>/dev/null || echo MISSING)"
    @echo "board:     {{board}}"
    @echo "simboard:  {{simboard}}"

# build for the ESP32-S3 devkit
[group('firmware')]
fw-build:
    {{west}} build -b {{board}} firmware/app -d build/esp32s3

# flash the devkit over its USB connection
[group('firmware')]
fw-flash: fw-build
    {{west}} flash -d build/esp32s3

# serial monitor (ctrl-] to exit)
[group('firmware')]
fw-monitor:
    {{west}} espressif monitor -d build/esp32s3

# run the application on the host under qemu (ctrl-a x to exit)
[group('firmware')]
fw-sim:
    {{west}} build -b {{simboard}} firmware/app -d build/sim
    {{west}} build -t run -d build/sim

# same, halted waiting for a debugger on :1234 (see .vscode/launch.json)
[group('firmware')]
fw-sim-debug:
    {{west}} build -b {{simboard}} firmware/app -d build/sim
    {{west}} build -t debugserver -d build/sim

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
    {{python}} -m unittest discover tools/tests

# website test suite (vitest)
[group('tests')]
test-website:
    cd website && npm test

# decompress real question bundles into firmware/tests/fixtures/
[group('tests')]
fw-fixtures: bundle
    mkdir -p firmware/tests/fixtures
    for f in dist/bundles/bundle-*.tkb; do \
        lang=$(basename "$f" | cut -d- -f2); \
        gzip -dc "$f" > "firmware/tests/fixtures/$lang.tkb2"; \
    done
    @ls -l firmware/tests/fixtures/

# firmware suites under qemu (one suite: just fw-test bag)
[group('tests')]
fw-test suite="": fw-fixtures
    {{west}} twister -T firmware/tests{{ if suite == "" { "" } else { "/" + suite } }} \
        -p {{simboard}} --inline-logs -O build/twister

# firmware suites on native_sim in docker — the GPIO-driven ones qemu can't run
[group('tests')]
fw-test-linux suite="": fw-fixtures
    docker run --rm --platform linux/amd64 -v "$PWD:/work" -w /work {{ci_image}} \
        bash -lc "west twister -T firmware/tests{{ if suite == '' { '' } else { '/' + suite } }} \
        -p native_sim --inline-logs -O build/twister-linux"

# ---------------------------------------------------------------- formatting

# format everything (firmware, python, website, docs)
[group('format')]
fmt: fmt-fw fmt-py fmt-web fmt-docs

# check formatting without writing (what CI runs)
[group('format')]
fmt-check:
    git ls-files '*.c' '*.cpp' '*.h' '*.hpp' | xargs -r clang-format --dry-run --Werror
    {{ruff}} format --check tools/
    {{prettier}} --check "website/**/*.{ts,astro,css,json}" "docs/**/*.md" "*.md"

# firmware C/C++ — git ls-files keeps this away from deps/ (zephyr)
[group('format')]
fmt-fw:
    git ls-files '*.c' '*.cpp' '*.h' '*.hpp' | xargs -r clang-format -i

# python tools
[group('format')]
fmt-py:
    {{ruff}} format tools/

# lint python tools
[group('format')]
lint-py:
    {{ruff}} check tools/

# website typescript / astro / css
[group('format')]
fmt-web:
    {{prettier}} --write "website/**/*.{ts,astro,css,json}"

# markdown and workflow yaml (never questions/ — validate.py --fix owns those)
[group('format')]
fmt-docs:
    {{prettier}} --write "docs/**/*.md" "*.md" ".github/**/*.yml"

# -------------------------------------------------------------------- docs

# fetch the self-hosted mermaid bundle (skipped if already present)
[group('docs')]
docs-deps:
    @mkdir -p docs/assets/javascripts
    @if [ ! -f docs/assets/javascripts/mermaid.min.js ]; then \
        echo "fetching mermaid {{mermaid_version}}…"; \
        curl -sSL --fail -o docs/assets/javascripts/mermaid.min.js \
            "https://unpkg.com/mermaid@{{mermaid_version}}/dist/mermaid.min.js"; \
    fi
    @grep -q 'globalThis\["mermaid"\]' docs/assets/javascripts/mermaid.min.js \
        || { echo "mermaid bundle does not set the global Material checks for"; exit 1; }

# live-preview the docs at http://127.0.0.1:8000
[group('docs')]
docs: docs-deps
    {{mkdocs}} serve

# build the docs site into site/
[group('docs')]
docs-build: docs-deps
    {{mkdocs}} build
