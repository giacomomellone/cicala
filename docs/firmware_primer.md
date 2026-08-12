# Firmware primer

An introduction to the device firmware and to Zephyr as this project uses it:
what the tree contains, how a build is assembled, and what to read or run to
understand each piece. Everything here is runnable — read it next to a terminal,
with the repository root as the working directory.

The design this implements is [firmware architecture](firmware_architecture.md).
The reasons behind each choice are in [decisions](decisions.md).

**Status: the tabletop loop works.** Turning the selector or pressing Next
draws a question and renders it. Deep sleep, the power path, Wi-Fi sync and the
setup portal are still design, so this primer covers the machinery as much as
the product.

This primer describes the current DIP-switch breadboard and its code. The
[device prototype](device_prototype.md) now targets adjacent Category and Next
buttons, but that input transition waits until a second physical button is
available.

---

## 1. What is here

```
firmware/
├── west.yml                    manifest: zephyr revision + module allowlist
├── Kconfig.policy              our own settings (CONFIG_TK_*), shared with tests
├── app/
│   ├── CMakeLists.txt
│   ├── prj.conf                shared config, all boards
│   ├── Kconfig                 pulls in ../Kconfig.policy
│   ├── boards/                 per-board config + devicetree overlay
│   ├── include/                channels.h, input.h, panel.h, app_logic.h
│   └── src/
│       ├── channels.c          zbus channel definitions
│       ├── input.c             selector one-hot + settle, Next
│       ├── app.c               the app thread and its subscriber
│       ├── app_logic.cpp       state machine + question store behind a C API
│       ├── display.c           the display thread
│       ├── panel.cpp           CFB, refresh policy, accent marks
│       └── main.c              boot only; the threads do the work
├── lib/                        hardware-free C++17
│   ├── fsm/                    table-driven state machine
│   ├── app_fsm/                the tabletop machine built on it
│   ├── qdb/                    QDB2 reader and shuffle bag
│   └── layout/                 UTF-8, accents, word wrap
└── tests/                      smoke, fsm, input, app_fsm, qdb,
                                layout, panel, integration
```

`deps/` is absent from the tree: it is Zephyr itself, cloned by `just fw-init`,
gitignored, never committed. The west workspace uses T2 topology — `west.yml`
is the manifest, the repository root is the topdir, and Zephyr plus its modules
land in `deps/`.

Quick check that your machine is set up:

```sh
just fw-doctor
```

Every line should have a path or a version. If not, it prints the command to
fix that line.

---

## 2. The three commands you will actually use

```sh
just fw-build     # compile for the ESP32-S3 devkit
just fw-test      # run both ztest suites under qemu, no hardware
just fw-flash     # build + write to the board over USB
```

`just fw-test` takes about 25 seconds and needs nothing plugged in. That is
the loop for anything that is not driver code.

---

## 3. How a Zephyr build works

A Zephyr binary is assembled from four inputs. Confusing them is the source of
most early frustration, so it is worth separating them properly.

| Input | Answers | Lives in |
|---|---|---|
| **Board** | Which chip, which peripherals exist | `deps/zephyr/boards/…` |
| **Devicetree** | What hardware is wired where | `app/boards/*.overlay` |
| **Kconfig** | Which software features are compiled in | `app/prj.conf`, `app/Kconfig` |
| **Source** | What the program does | `app/src/`, `lib/` |

The rule of thumb: **devicetree describes hardware, Kconfig selects software.**
A pin number is devicetree. "Include the SPI driver" is Kconfig.

### The board target

```
esp32s3_devkitc/esp32s3/procpu
└── board          └── SoC  └── CPU cluster
```

Under Zephyr's hardware model v2, the qualifiers are part of the name. Leaving
them off is an error rather than a default — `qemu_xtensa` is a board *name*,
and the target is `qemu_xtensa/dc233c`. `west boards` lists what exists.

---

## 4. Devicetree, hands on

Devicetree is a description of hardware that the build turns into C macros. You
never write those macros — you read the tree and ask for a node.

**Look at what the board gives you:**

```sh
sed -n '15,45p' deps/zephyr/boards/espressif/esp32s3_devkitc/esp32s3_devkitc_procpu.dts
```

You will see an `aliases` block with `watchdog0` and not much else. In
particular there is no `led0`, which is why the two status LEDs are declared in
our own overlay and reached through `DT_ALIAS(tk_led_red)` and
`DT_ALIAS(tk_led_green)` rather than through the usual `led0`. The DevKitC-1's
only onboard LED is an addressable WS2812, which this firmware never drives —
its controller idles at about 0.6 mA with the emitters dark, against a
whole-device budget of 30 µA.

**Look at what we add:**

```sh
cat firmware/app/boards/esp32s3_devkitc_esp32s3_procpu.overlay
```

The filename is not arbitrary: it is the board target with `/` replaced by `_`.
Zephyr picks it up automatically because it sits in `app/boards/`.

**Look at the merged result.** This is the file worth knowing about — board
plus overlay, fully resolved:

```sh
less build/esp32s3/app/zephyr/zephyr.dts
```

The `app/` in that path is `--sysbuild`, which `just fw-build` passes so the
bootloader is built alongside the application. It moves the application's output
one level down; a build without it writes to `build/esp32s3/zephyr/` instead.

**Now change something and watch it move.** In the overlay, find:

```dts
zephyr,user {
    tk-vbus-gpios = <&gpio0 21 GPIO_ACTIVE_HIGH>;
};
```

Change `21` to `14` — one of the spare RTC-capable pins, and not one Category,
Next, the panel, the battery divider or the LEDs already claim — then rebuild
and grep the generated header:

```sh
just fw-build
grep -m1 -A3 "zephyr_user.*tk_vbus_gpios" build/esp32s3/app/zephyr/include/generated/zephyr/devicetree_generated.h
```

That 1 MB header is what `GPIO_DT_SPEC_GET(DT_PATH(zephyr_user), tk_vbus_gpios)`
in `app/src/power.c` expands into. The C source never mentions a pin number, so
moving VBUS detect is an overlay edit and the same source still builds for the
host — and for the test suites, whose overlays put the same property on an
emulated controller instead. Put the `21` back afterwards: the pin is where the
divider is soldered.

`zephyr,user` is a conventional catch-all node for application-specific
properties that do not deserve a binding of their own — here, the battery ADC
channel and the VBUS pin. It is not where things with a real binding go: the
buttons, the LEDs and the panel have proper `gpio-keys`, `gpio-leds` and
`solomon,ssd1680` nodes in the same overlay.

The pins in that overlay are chosen, not arbitrary. Category and Next both sit
on GPIO0..21, the ESP32-S3's RTC-capable range, because EXT1 deep-sleep wake
works on no other pins; the panel avoids GPIO19/20, which are the native USB
pair the debug console and JTAG both use; and battery sense takes an ADC1
channel, because ADC2 stops answering while the radio is up. They stay in the
overlay rather than in C so the same application logic moves to the target PCB
by changing one file. [Hardware wiring](hardware_wiring.md) has the whole map.

---

## 5. Kconfig, hands on

Kconfig decides what is compiled in. Symbols come from Zephyr, from modules,
and from us.

**Ours are in `firmware/Kconfig.policy`**, sourced by `firmware/app/Kconfig`.
One level up from the application so the test suites build against the same
values rather than a copy that drifts:

```sh
cat firmware/Kconfig.policy
```

These are the values the architecture deliberately leaves open — the selector
settle window, the refresh interval, the depth cap. They are Kconfig rather
than `#define` so they can be changed per board and per test without editing
source.

**See the resolved values:**

```sh
grep "^CONFIG_TK_" build/esp32s3/app/zephyr/.config
```

```
CONFIG_TK_BUTTON_DEBOUNCE_MS=30
CONFIG_TK_FULL_REFRESH_INTERVAL=16
CONFIG_TK_REFRESH_TIMEOUT_MS=15000
CONFIG_TK_PLAYBACK_DEPTH_MAX=2
CONFIG_TK_RECENT_RING=20
CONFIG_TK_REFRESH_MIN_MV=3200
```

**Explore interactively:**

```sh
.venv/bin/west build -t menuconfig -d build/esp32s3
```

Press `/` to search for a symbol. `CONFIG_TK_` finds ours. This is the fastest
way to discover what a subsystem offers and what it depends on — Kconfig shows
you why a symbol is unavailable, which is usually a missing dependency.

Changes made in menuconfig apply to that build directory only. To keep one,
write it into `app/prj.conf` (all boards) or `app/boards/<target>.conf` (one
board).

**Three layers, in merge order:**

1. the board's `_defconfig`
2. `app/prj.conf`
3. `app/boards/<board target>.conf`

`build/<dir>/zephyr/.config` is the outcome. When a setting seems ignored,
check there first — it is the ground truth, and a typo'd symbol name silently
does nothing.

**A note on the `LATER` blocks in `prj.conf`.** Sections for zbus, display,
storage, power management and networking are written out and commented. They
are the architecture's targets, staged so the tree always builds. Uncomment as
each component lands.

The module allowlist in `firmware/west.yml` works the same way, one level up:
it is narrow on purpose, because importing all ~60 Zephyr modules costs several
GB. A build that fails on a missing upstream header usually wants a name added
there, followed by `west update`.

---

## 6. What to look at in a build directory

```sh
just fw-build
```

The link map at the end is the most informative output:

```
FLASH:      140564 B    8388352 B     1.68%
iram0_0_seg:  49832 B     415492 B    11.99%
dram0_0_seg:  58152 B     399108 B    14.57%
rtc_slow_seg:    36 B        8 KB      0.44%
```

That last line matters for this project. `rtc_slow_seg` is memory that survives
deep sleep, and the architecture puts the shuffle bag, the recent ring and the
current question there — about 512 bytes against the 8 KB shown. Watch it as
the application grows.

Worth knowing about:

| Path | What it is |
|---|---|
| `zephyr/.config` | resolved Kconfig, the ground truth |
| `zephyr/zephyr.dts` | merged devicetree, board + overlay |
| `zephyr/include/generated/zephyr/devicetree_generated.h` | the macros `DT_*` expands to |
| `zephyr/zephyr.elf` | what the debugger loads |
| `compile_commands.json` | what clangd reads for IntelliSense |

`just fw-clean` removes all of it. Rebuild after changing anything structural —
Zephyr does not always notice a new source file, and a stale build directory
produces confusing errors like `ninja: error: loading 'build.ninja'`.

---

## 7. The state machine

`firmware/lib/fsm/` is a table-driven FSM. The idea: a transition table is the
specification, and the per-tick function only answers *what happened*, never
*where do I go*.

```cpp
enum class State { IDLE = 0, WORKING, DONE, FAILED };
enum class Transition { CONTINUE, FAIL, REPEAT };

// clang-format off
const Fsm::StateTransition MyFsm::_transitions[] = {
//   Current State     Transition             Next State       Timeout
    {STATE(IDLE),     TRANSITION(CONTINUE),  STATE(WORKING),  0       },
    {STATE(IDLE),     TRANSITION(REPEAT),    STATE(IDLE),     0       },
    {STATE(WORKING),  TRANSITION(CONTINUE),  STATE(DONE),     0       },
    {STATE(WORKING),  TRANSITION(REPEAT),    STATE(WORKING),  30_s    },
};
// clang-format on
```

The `// clang-format off` is required. Without it the formatter collapses the
column alignment, and the alignment is what makes the table readable as a
specification.

Four behaviours worth knowing, each covered by a test:

- **A timeout only fires on a self-loop.** A transition that leaves the state
  wins even on the tick its timeout expires, so a state that has just succeeded
  is never sent to the fail state instead.
- **A self-loop does not restart the clock.** Otherwise a state that repeats
  every tick would never reach its timeout.
- **A transition with no table row goes to the fail state.** That is a table
  bug, not a runtime condition, and failing loudly beats silently staying put.
- **`current_state_has_timeout()`** tells an event loop whether it may block
  forever. In this design that is what lets the system reach deep sleep: no
  timeout to honour means `K_FOREVER`, which means nothing is runnable, which
  means the idle thread can power down.

Time is injected via a virtual `now_ms()`. Tests override it, so a 30-second
timeout is verified in microseconds and no suite ever sleeps:

```cpp
fsm.advance(29_s);
fsm.run();
zassert_equal(fsm.get_current_state(), STATE(WORKING), "timed out early");

fsm.advance(1_s + 1_ms);
fsm.run();
zassert_equal(fsm.get_current_state(), STATE(FAILED));
```

---

## 8. Tests

```sh
just fw-test              # everything
just fw-test fsm          # one suite
```

Under the hood this is `twister`, Zephyr's runner. It builds each suite, runs
it under qemu, and parses the output.

**To add a suite**, copy `firmware/tests/fsm/` — four files:

| File | Purpose |
|---|---|
| `CMakeLists.txt` | sources, plus `add_subdirectory` for any lib under test |
| `prj.conf` | `CONFIG_ZTEST=y` and whatever the code needs |
| `testcase.yaml` | which platforms may run it |
| `src/main.cpp` | `ZTEST_SUITE` and `ZTEST` cases |

A suite that needs the application's own `CONFIG_TK_*` values adds a `Kconfig`
with `rsource "../../Kconfig.policy"`, the same file `app/Kconfig` pulls in, so
changing the settle window changes what the test asserts.

A library added with `add_subdirectory` after `find_package(Zephyr)` needs two
lines that are easy to miss, because Zephyr collects its own libraries before
`find_package` returns: `add_dependencies(<lib> zephyr_generated_headers)` so
it does not compile before the generated headers exist, and
`target_link_libraries(app PRIVATE <lib>)` so it is actually linked.
`firmware/lib/fsm/CMakeLists.txt` is the worked example.

**Platforms.** `qemu_xtensa/dc233c` is the default: a full Zephyr kernel on
macOS, same architecture as the target. `native_sim` is faster and builds on
Linux only, which is what `just fw-test-linux` (Docker) and CI are for.

Emulated GPIO is available on both. `CONFIG_GPIO_EMUL` is selected by a
`zephyr,gpio-emul` node in the devicetree, not by the host, so a suite that
drives the selector and Next — `tests/input` — runs under qemu on macOS like
any other. Its overlay is `tests/input/boards/qemu_xtensa_dc233c.overlay`.

**Fixtures.** `qdb` tests will run against real question bundles rather than
hand-written bytes. `just fw-fixtures` builds them from the question database
and decompresses them, since the device stores raw QDB2 and the gzip is only a
transport encoding. They are gitignored, so a corpus change is picked up by the
next run.

---

## 9. Debugging

**Host, under qemu.** Open the repo in VSCode and pick *sim: debug (qemu)* —
it builds, starts a gdb server on :1234 halted, and attaches. Breakpoints in
`main()` hit.

**On hardware.** *devkit: debug (OpenOCD)* uses the DevKitC-1's built-in
USB-JTAG, so no external probe is needed. It starts OpenOCD on :3333 and
attaches with the xtensa gdb from the SDK.

If either fails to launch, the SDK paths are in `.vscode/settings.json` as
`tischkarte.gdbTarget` and `tischkarte.gdbSim`. `just fw-doctor` prints the
paths that actually exist.

**Printf debugging** works too — `just fw-monitor` opens the serial console.
`LOG_INF` output appears there.

---

## 10. Conventions

**Always go through `just`.** Zephyr's espressif CMake looks for `esptool` on
`PATH` rather than in the venv, and every recipe prepends `.venv/bin` for that
reason. Calling `west` directly works only with the same prefix:
`PATH="$PWD/.venv/bin:$PATH" .venv/bin/west build …`.

**C++ for logic, C at the Zephyr boundary.** Not stylistic:
`ZBUS_CHAN_DEFINE`, `ZBUS_MSG_INIT` and several HAL macros expand to
out-of-order designated initializers, which C accepts and C++17 rejects.
Keeping those files `.c` means upstream samples paste in unmodified.

No exceptions, no RTTI, nothing allocates. Construct objects inside `main()`
rather than as globals — a static constructor runs before device drivers are
ready.

**Formatting** is `just fmt`. Firmware is clang-format with Linux braces and
four-space indent; `deps/` is protected by its own `DisableFormat` config, so
an editor cannot reformat Zephyr on save. `just fmt-check` is what CI runs.

**Never format `questions/`.** `just fix` (`tools/validate.py --fix`) owns that
formatting exactly, and prettier is configured to ignore it.

**Commits** are conventional, enforced by `.githooks/commit-msg`. Scope `fw`
for firmware.
