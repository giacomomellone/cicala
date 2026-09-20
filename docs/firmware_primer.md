# Firmware primer

This page covers the ESP32-S3 device. The [Xteink X4 integration](crosspoint.md)
uses the same portable Cicala core through a CrossPoint adapter.

The firmware targets an ESP32-S3 DevKitC with Zephyr. The breadboard rig runs
the full tabletop loop, deep sleep, setup portal, bundle sync, signed OTA, and
power-status path. See [firmware architecture](firmware_architecture.md) for
the design and [hardware wiring](hardware_wiring.md) for the rig.

## Source tree

```text
firmware/
├── west.yml                    Zephyr revision and module allowlist
├── Kconfig.policy              shared CONFIG_CICALA_* settings
├── app/
│   ├── prj.conf                common Zephyr configuration
│   ├── boards/                 board configuration and overlays
│   ├── include/                application interfaces
│   ├── src/                    Zephyr-facing modules
│   └── sysbuild/               MCUboot configuration
├── lib/                        hardware-free C++17 logic
│   ├── app_fsm/                tabletop state machine
│   ├── ed25519/                signature verification
│   ├── fsm/                    generic table-driven state machine
│   ├── layout/                 UTF-8 layout and accent composition
│   ├── portal/                 setup protocol and state machine
│   ├── power/                  battery-state rules
│   ├── qdb/                    QDB4 reader and shuffle bag
│   ├── retained/               RTC state block
│   ├── status/                 LED policy
│   └── sync/                   manifest parsing
└── tests/                      ztest suites
```

`deps/` is the gitignored west workspace containing Zephyr and its modules. Do
not edit or commit it. Local changes required in Zephyr are listed in
`firmware/patches.yml` and managed through `just fw-patch`.

## Setup and common commands

Run the one-time setup and inspect the result:

```sh
just fw-init
just fw-doctor
```

The normal development loop is:

```sh
just fw-build             # build MCUboot and the release application
just fw-flash             # build and flash the release profile
just fw-monitor           # open the UART console
just fw-test              # run all firmware suites under qemu
just fw-test fsm          # run one suite
```

Special images use a profile argument instead of separate recipes:

```sh
just fw-build debug
just fw-flash power
just fw-monitor power
just fw-build bench 192.168.1.23
```

Available profiles are `release`, `debug`, `charset`, `soak`, `retain`,
`power`, `portal`, `corpus`, `bench`, and `ota`. Each profile has its own build
directory. `bench` requires a server host. `ota` requires a host when its build
directory is first configured.

## Build inputs

A Zephyr application combines four inputs:

| Input      | Defines                               | Location                                              |
| ---------- | ------------------------------------- | ----------------------------------------------------- |
| Board      | chip and built-in peripherals         | `deps/zephyr/boards/`                                 |
| Devicetree | attached hardware and pin assignments | `app/boards/*.overlay`                                |
| Kconfig    | compiled features and settings        | `app/prj.conf`, `app/boards/*.conf`, `Kconfig.policy` |
| Source     | runtime behavior                      | `app/src/`, `lib/`                                    |

The target is `esp32s3_devkitc/esp32s3/procpu`. Zephyr hardware-model
qualifiers are part of the target name. The default simulator target is
`qemu_xtensa/dc233c`.

### Devicetree

The ESP32-S3 overlay is
`firmware/app/boards/esp32s3_devkitc_esp32s3_procpu.overlay`. Zephyr derives the
filename from the board target by replacing `/` with `_`.

The overlay declares:

- Filters and Next as `gpio-keys`;
- red and green indicators as `gpio-leds`;
- the SSD1680 panel on SPI2;
- battery ADC and VBUS GPIO properties under `zephyr,user`.

Application code reads nodes and aliases rather than fixed pin numbers. Tests
provide matching properties on emulated devices.

After a release build, inspect the merged tree and generated macros here:

```text
build/esp32s3/app/zephyr/zephyr.dts
build/esp32s3/app/zephyr/include/generated/zephyr/devicetree_generated.h
```

### Kconfig

Project settings live in `firmware/Kconfig.policy`; the application and tests
include the same file. Common values are visible in the resolved configuration:

```sh
grep '^CONFIG_CICALA_' build/esp32s3/app/zephyr/.config
```

Use `just fw-menuconfig` to inspect symbols and dependencies. Persist a setting
in `app/prj.conf` for every board or in `app/boards/<target>.conf` for one board.
Generated `.config` files belong to their build directories.

## Build output

The release profile uses sysbuild, so application output lives under
`build/esp32s3/app/` and MCUboot output under `build/esp32s3/mcuboot/`.

| Path                           | Contents                    |
| ------------------------------ | --------------------------- |
| `app/zephyr/.config`           | resolved Kconfig            |
| `app/zephyr/zephyr.dts`        | merged devicetree           |
| `app/zephyr/zephyr.elf`        | debugger image              |
| `app/zephyr/zephyr.signed.bin` | MCUboot-signed application  |
| `app/compile_commands.json`    | clangd compilation database |
| `mcuboot/zephyr/zephyr.bin`    | bootloader image            |

The linker summary includes `rtc_slow_seg`, which stores retained state across
deep sleep. `just fw-clean` removes every firmware build directory.

## Runtime boundaries

Files in `firmware/lib/` contain portable logic. Files in `firmware/app/src/`
connect that logic to Zephyr devices, threads, zbus, storage, and networking.
C interfaces isolate the C++ libraries from Zephyr macros that require C
designated initializers.

Dedicated threads run the app, display, network, and portal DNS work. Input
callbacks run on the system workqueue; power, status, and sleep use delayed
work. They communicate through typed zbus channels. `main.c` initializes the
shared services. The full execution and data flow is in
[firmware architecture](firmware_architecture.md).

The generic FSM uses a transition table and an injected clock. A self-loop does
not restart its timeout; a transition that leaves the current state wins on the
timeout tick; a missing transition enters the fail state. These rules are
covered by `firmware/tests/fsm/`.

## Tests

Each suite under `firmware/tests/` contains its CMake file, configuration,
`testcase.yaml`, and sources. Suites that use project policy include
`firmware/Kconfig.policy` through a local `Kconfig` file.

`just fw-test` runs on `qemu_xtensa/dc233c`. `just fw-test-linux` runs the same
suites on `native_sim` in the Zephyr CI container. Both platforms support
emulated GPIO through a `zephyr,gpio-emul` devicetree node.

The private fixture recipe builds two sets of QDB4 files before application
builds and tests: the shipped database into `dist/corpus/`, which the image, the
layout and panel walks and the qdb corpus guards read, and the fixture corpus in
`firmware/tests/corpus/` into `firmware/tests/fixtures/`, which the bag,
integration and soak cases draw from. Behaviour suites stay green whatever the
editorial database currently holds; the guards skip while it is empty. See
`firmware/tests/corpus/README.md`.

## Debugging

`just fw-sim-debug` runs the simulator and waits for GDB on port 1234.
`just fw-debug` flashes the debug profile and starts OpenOCD on port 3333 through
the DevKitC native USB-JTAG interface. VS Code launch configurations attach to
those ports.

The debug profile moves its console to native USB. The release profile uses the
UART jack. `just fw-doctor` reports both detected ports and the configured SDK
debuggers.

## Conventions

Use `just` for firmware commands so west, esptool, the SDK, fixture generation,
and build directories stay consistent. Run `just fw-patch` after `west update`.

Firmware logic uses C++17 without exceptions, RTTI, or dynamic allocation.
Construct objects after Zephyr devices are ready. Zephyr-facing macro
definitions remain in C files.

Run `just fmt` for formatting and `just fmt-check` for the CI check. Never edit
or format `deps/`.
