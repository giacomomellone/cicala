# tischkarte — type `just` to see what you can do.
# One-time machine setup: `just setup` (plus `just fw-init` for firmware work)

set shell := ["bash", "-cu"]

python := ".venv/bin/python"
mkdocs := ".venv/bin/mkdocs"
ruff := ".venv/bin/ruff"
west := ".venv/bin/west"
prettier := "website/node_modules/.bin/prettier"
board := "esp32s3_devkitc/esp32s3/procpu"

# Serial port of the devkit's UART jack (the CP2102 bridge), which is where
# the console lands because the board's chosen console is uart0. With both
# cables in, the *other* jack is native USB and shows up as cu.usbmodem*: it
# carries JTAG, not the console, and esptool cannot talk to it. Hence matching
# usbserial specifically rather than taking whatever appears first.
#
# Left to esptool, the probe walks every port on the machine, Bluetooth ones
# included, and connects to whichever answers first. Override for a second
# board, or when the glob picks the wrong one:
#   just port=/dev/cu.usbserial-0002 fw-flash
#
# `|| true` so a machine with no board attached still parses the justfile;
# the empty result then means "let esptool guess", and the flags below drop
# out entirely rather than passing a blank --port, which would fail outright.

port := env("ESPTOOL_PORT", shell("ls /dev/cu.usbserial-* /dev/cu.SLAB_USBtoUART 2>/dev/null | head -1 || true"))
portflag := if port == "" { "" } else { "--esp-device " + port }
monport := if port == "" { "" } else { "-p " + port }

# The native-USB jack, where the debug build puts its console (see
# firmware/app/debug.overlay). Same device OpenOCD debugs through: the
# USB-Serial-JTAG peripheral serves a CDC-ACM and the JTAG interface at once,
# so one cable carries both and neither disturbs the other.

usbport := env("TK_USB_PORT", shell("ls /dev/cu.usbmodem* 2>/dev/null | head -1 || true"))
usbmonport := if usbport == "" { "" } else { "-p " + usbport }

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

# Host test platform. native_sim is faster but builds on Linux only, so
# qemu_xtensa (full kernel, target architecture, runs on macOS) is the default.
# Both emulate GPIO — CONFIG_GPIO_EMUL follows a zephyr,gpio-emul devicetree
# node, not the host — so the selector and Next suites run either way.
# Override for a Linux host:
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

# generate the ed25519 bundle-signing keypair (see docs/sync_protocol.md)
[group('database')]
keygen:
    {{ python }} tools/keygen.py

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
    # Python packages for zephyr and every enabled module. This covers both
    # the build scripts (pyelftools, pykwalify…) and per-SoC tools — esptool
    # comes from here, and cmake aborts without it.
    {{ west }} packages pip --install
    {{ west }} blobs fetch hal_espressif
    just fw-patch
    just _west-build-dir build/esp32s3
    @echo
    @echo "workspace ready. next: install the Zephyr SDK, then `just fw-doctor`"

# Local changes to the Zephyr tree. deps/ is gitignored and `west update`
# checks the pinned revision back out, so anything we change there has to be
# reapplied — after an update, and after a `west patch clean`. See
# firmware/patches.yml for what is patched and why.

# reapply the local Zephyr patches (run after any `west update`)
[group('firmware')]
fw-patch:
    {{ west }} patch -b patches -l patches.yml apply

# Destructive: `west patch clean` is `git checkout .` plus `git clean -d -f -x`
# inside deps/zephyr, so it discards *any* local edit there, not only ours.
# Nothing in deps/ is committed to this repo, so nothing unique is lost.

# drop the local Zephyr patches, restoring the pinned tree
[group('firmware')]
fw-unpatch:
    {{ west }} patch -b patches -l patches.yml clean

# `west espressif monitor` locates the build through west's build.dir-fmt
# config, not through a flag — its own -d means --enable-address-decoding.
# With dir-fmt unset it looks in ./build and dies with "could not find build
# configuration". Takes the dir so the debug monitor decodes addresses against
# the -O0 elf rather than the -Os one. Idempotent; writes .west/config.
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
    @# SDK 1.0 restructured: toolchains under gnu/, host tools under hosttools/.
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
    @# The console is uart0, so the cable belongs in the UART jack: that one
    @# shows up as cu.usbserial-*, the native-USB jack as cu.usbmodem*.
    @echo "port:      {{ if port == '' { 'auto (esptool probes every port, Bluetooth included)' } else { port } }}"
    @p=$(ls /dev/cu.usbserial-* /dev/cu.SLAB_USBtoUART /dev/tty.usbserial-* 2>/dev/null | tr '\n' ' '); \
     echo "  detected: ${p:-none — is the cable in the UART jack, not the USB one?}"
    @# The USB jack: JTAG for OpenOCD, and the debug build's console.
    @echo "usbport:   {{ if usbport == '' { 'none — USB jack not connected; no JTAG, no debug console' } else { usbport } }}"

# build for the ESP32-S3 devkit
[group('firmware')]
fw-build: fw-fixtures
    {{ west }} build -b {{ board }} firmware/app -d build/esp32s3

# Changes land in build/esp32s3/zephyr/.config, which fw-clean and pristine
# rebuilds discard — copy anything worth keeping into firmware/app/prj.conf.

# open the kconfig menu for the devkit build (q to quit, s to save)
[group('firmware')]
fw-menuconfig: fw-build
    {{ west }} build -t menuconfig -d build/esp32s3

# flash the devkit over its USB connection
[group('firmware')]
fw-flash: fw-build
    {{ west }} flash --no-rebuild -d build/esp32s3 {{ portflag }}

# Separate build dir rather than one dir reconfigured back and forth:
# EXTRA_CONF_FILE is cached by cmake, so dropping the flag later would
# silently keep -O0.

# build with debug.conf + debug.overlay merged in (-O0, console on USB jack)
[group('firmware')]
fw-build-debug: fw-fixtures
    {{ west }} build -b {{ board }} firmware/app -d build/esp32s3-debug -- \
        -DEXTRA_CONF_FILE=debug.conf -DEXTRA_DTC_OVERLAY_FILE=debug.overlay

# flash the -O0 image — what the VSCode debug configs expect on the chip
[group('firmware')]
fw-flash-debug: fw-build-debug
    {{ west }} flash --no-rebuild -d build/esp32s3-debug {{ portflag }}

# Character-set test pages instead of questions: ASCII, then one page per
# language, then every diacritic the renderer composes. Next steps through
# them, and each page is logged so the monitor says what should be on the
# glass. Its own build dir, so switching back to the real image is not a
# rebuild — same reasoning as the debug pair above.

# build the charset test image (see CONFIG_TK_DEBUG_CHARSET)
[group('firmware')]
fw-build-charset: fw-fixtures
    {{ west }} build -b {{ board }} firmware/app -d build/esp32s3-charset -- \
        -DCONFIG_TK_DEBUG_CHARSET=y

# flash the charset test image, then `just fw-monitor`
[group('firmware')]
fw-charset: fw-build-charset
    {{ west }} flash --no-rebuild -d build/esp32s3-charset {{ portflag }}

# The device presses its own Next button every few seconds, with full refreshes
# suppressed, so a chain of partial refreshes builds up while nobody is at the
# board. That chain is the only way to see where ghosting starts, which is the
# number CONFIG_TK_FULL_REFRESH_INTERVAL is supposed to hold. The console
# reports the count and the duration of every refresh; the procedure for
# reading them is in docs/hardware_wiring.md.

# build the soak image (see firmware/app/soak.conf)
[group('firmware')]
fw-build-soak: fw-fixtures
    {{ west }} build -b {{ board }} firmware/app -d build/esp32s3-soak -- \
        -DEXTRA_CONF_FILE=soak.conf

# flash the soak image, then `just fw-monitor` and leave it running
[group('firmware')]
fw-soak: fw-build-soak
    {{ west }} flash --no-rebuild -d build/esp32s3-soak {{ portflag }}

# The same image, rebooting itself every few presses. Deep sleep is a reboot,
# so this is the closest thing to a wake that exists before CONFIG_PM does: the
# shuffle bag, the refresh counter and the question on the glass either come
# back across it or they do not, and the boot log says which.
#
# Note that a reset from the board's own EN pin reports as POWERON and takes
# the RTC domain with it, so pressing the button proves nothing here. Only a
# warm reset from software does.

# build the self-rebooting soak image (see CONFIG_TK_DEBUG_SOAK_REBOOT)
[group('firmware')]
fw-build-retain: fw-fixtures
    {{ west }} build -b {{ board }} firmware/app -d build/esp32s3-retain -- \
        -DEXTRA_CONF_FILE=soak.conf -DCONFIG_TK_DEBUG_SOAK_REBOOT=y

# flash it, then `just fw-monitor` and watch what survives each reboot
[group('firmware')]
fw-retain: fw-build-retain
    {{ west }} flash --no-rebuild -d build/esp32s3-retain {{ portflag }}

# Deep sleep. The device powers down when the table goes quiet and every wake
# runs main() from the top, so the console restarts with each press. The
# default image never sleeps; this is the one that does.

# build the setup portal without the two-button gesture (see CONFIG_TK_DEBUG_PORTAL)
[group('firmware')]
fw-build-portal: fw-fixtures
    {{ west }} build -b {{ board }} firmware/app -d build/esp32s3-portal -- \
        -DCONFIG_TK_DEBUG_PORTAL=y

# flash it: the portal comes up at every boot, no hands needed
[group('firmware')]
fw-portal: fw-build-portal
    {{ west }} flash --no-rebuild -d build/esp32s3-portal {{ portflag }}

# build with the compiled-in corpus written to the filesystem at boot
[group('firmware')]
fw-build-corpus: fw-fixtures
    {{ west }} build -b {{ board }} firmware/app -d build/esp32s3-corpus -- \
        -DCONFIG_TK_DEBUG_CORPUS_STORE=y

# flash it, reboot once, and the corpus then comes off the filesystem
[group('firmware')]
fw-corpus: fw-build-corpus
    {{ west }} flash --no-rebuild -d build/esp32s3-corpus {{ portflag }}

# build with sync pointed at a server on the bench: just fw-build-bench 192.168.1.2
[group('firmware')]
fw-build-bench host port="8000": fw-fixtures
    {{ west }} build -b {{ board }} firmware/app -d build/esp32s3-bench -- \
        -DEXTRA_CONF_FILE=bench.conf \
        -DCONFIG_TK_SYNC_HOST=\"{{ host }}\" \
        -DCONFIG_TK_SYNC_PORT={{ port }} \
        -DCONFIG_TK_SYNC_BASE_URL=\"http://{{ host }}:{{ port }}\"

# flash it, then `just fw-monitor` to watch a real bundle arrive
[group('firmware')]
fw-bench host port="8000": (fw-build-bench host port)
    {{ west }} flash --no-rebuild -d build/esp32s3-bench {{ portflag }}

# serial monitor (ctrl-] to exit)
[group('firmware')]
fw-monitor: (_west-build-dir "build/esp32s3")
    {{ west }} espressif monitor {{ monport }}

# Reads the CDC-ACM on the USB jack, which is where debug.overlay puts the
# console. Safe to open mid-session: that jack has no auto-reset circuit, so
# unlike fw-monitor it cannot reset the chip out from under gdb.
#
# For the -Os build this prints nothing — its console is on the UART jack.

# serial monitor for the debug build, usable while debugging (ctrl-] to exit)
[group('firmware')]
fw-monitor-debug: (_west-build-dir "build/esp32s3-debug")
    {{ west }} espressif monitor {{ usbmonport }}

# OpenOCD on :3333 over the DevKitC's built-in USB-JTAG. That peripheral is on
# the *USB* jack, while the console and flashing are on the *UART* jack, so
# debugging wants both cables connected at once. Started for you by the
# "devkit: debug (OpenOCD)" launch config; run it here to keep one server up
# across repeated reflashes and use the "attach" config instead.
#
# --config replaces Zephyr's board openocd.cfg, which only works with
# Espressif's OpenOCD fork and not the upstream build the SDK ships. The
# replacement explains itself.
#
# Serves the -O0 build, matching the elf the launch configs hand to gdb.

# OpenOCD gdb server on :3333 for the devkit (built-in USB-JTAG)
[group('firmware')]
fw-debugserver: fw-build-debug
    {{ west }} debugserver --no-rebuild -d build/esp32s3-debug \
        --config firmware/app/support/esp32s3_builtin_jtag.cfg

# What the "devkit: debug (OpenOCD)" launch config runs, as one just process
# so the -O0 build happens once: just runs a dependency at most once per
# invocation, but VSCode chaining two tasks means two invocations and two
# builds. The --no-rebuild flags above stop west re-running cmake on top of
# that, which was another two.

# flash the -O0 image and serve it over JTAG — one build, not four
[group('firmware')]
fw-debug: fw-flash-debug fw-debugserver

# run the application on the host under qemu (ctrl-a x to exit)
[group('firmware')]
fw-sim: fw-fixtures
    {{ west }} build -b {{ simboard }} firmware/app -d build/sim
    {{ west }} build -t run -d build/sim

# Same debug.conf and same separate-build-dir reasoning as the devkit pair
# above: -Os reads every local as <optimized out>, and EXTRA_CONF_FILE is
# cached by cmake, so sharing build/sim would leave fw-sim silently at -O0.
#
# qemu always halts at the reset vector, so the first thing the debugger shows
# is reset_vector.S rather than main. That is the CPU at PC 0, not a fault —
# the launch config arms a breakpoint on main, so one continue lands there.

# same at -O0, halted waiting for a debugger on :1234 (see .vscode/launch.json)
[group('firmware')]
fw-sim-debug: fw-fixtures
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

# copy the real question bundles into firmware/tests/fixtures/
[group('tests')]
fw-fixtures: bundle
    mkdir -p firmware/tests/fixtures
    for f in dist/bundles/bundle-*.qdb; do \
        lang=$(basename "$f" | cut -d- -f2); \
        cp "$f" "firmware/tests/fixtures/$lang.qdb"; \
    done
    @ls -l firmware/tests/fixtures/

# firmware suites under qemu (one suite: just fw-test input)
[group('tests')]
fw-test suite="": fw-fixtures
    {{ west }} twister -T firmware/tests{{ if suite == "" { "" } else { "/" + suite } }} \
        -p {{ simboard }} --inline-logs -O build/twister

# firmware suites on native_sim in docker — the same image and platform CI uses
[group('tests')]
fw-test-linux suite="": fw-fixtures
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
