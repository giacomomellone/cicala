# Firmware guide

A hands-on tour of the Zephyr setup in this repo: what exists, how a build
actually works, and what to poke at to understand it. Everything here is
runnable — read it next to a terminal.

The design this implements is [docs/firmware_architecture.md](docs/firmware_architecture.md).
The reasons behind each choice are in [docs/decisions.md](docs/decisions.md).

---

## 1. What is here

```
firmware/
├── west.yml                    manifest: zephyr revision + module allowlist
├── app/
│   ├── CMakeLists.txt
│   ├── prj.conf                shared config, all boards
│   ├── Kconfig                 our own settings (CONFIG_TK_*)
│   ├── boards/                 per-board config + devicetree overlay
│   └── src/main.c              bring-up blinky
├── lib/fsm/                    table-driven state machine, C++17
└── tests/
    ├── smoke/                  proves the harness works
    └── fsm/                    13 cases, injected clock
```

Two things are deliberately absent: the application state machine, and
`deps/`. The first is next. The second is Zephyr itself — cloned by
`just fw-init`, gitignored, never committed.

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
particular there is no `led0`, which is why blinky here does not use the usual
`DT_ALIAS(led0)` — the DevKitC-1's only onboard LED is an addressable WS2812.

**Look at what we add:**

```sh
cat firmware/app/boards/esp32s3_devkitc_esp32s3_procpu.overlay
```

The filename is not arbitrary: it is the board target with `/` replaced by `_`.
Zephyr picks it up automatically because it sits in `app/boards/`.

**Look at the merged result.** This is the file worth knowing about — board
plus overlay, fully resolved:

```sh
less build/esp32s3/zephyr/zephyr.dts
```

**Now change something and watch it move.** In the overlay, find:

```dts
zephyr,user {
    blink-gpios = <&gpio0 2 GPIO_ACTIVE_HIGH>;
};
```

Change `2` to `4`, rebuild, and grep the generated header:

```sh
just fw-build
grep -m1 -A3 "zephyr_user.*blink_gpios" build/esp32s3/zephyr/include/generated/zephyr/devicetree_generated.h
```

That 1 MB header is what `GPIO_DT_SPEC_GET(DT_PATH(zephyr_user), blink_gpios)`
in `app/src/main.c` expands into. The C source never mentions a pin number, so
moving the LED is an overlay edit and the same source still builds for the
host.

`zephyr,user` is a conventional catch-all node for application-specific
properties that do not deserve a binding of their own. Good for bring-up,
not where the real selector and panel go — those get proper nodes.

---

## 5. Kconfig, hands on

Kconfig decides what is compiled in. Symbols come from Zephyr, from modules,
and from us.

**Ours are in `firmware/app/Kconfig`:**

```sh
cat firmware/app/Kconfig
```

These are the values the architecture deliberately leaves open — the selector
settle window, the refresh interval, the depth cap. They are Kconfig rather
than `#define` so they can be changed per board and per test without editing
source.

**See the resolved values:**

```sh
grep "^CONFIG_TK_" build/esp32s3/zephyr/.config
```

```
CONFIG_TK_SELECTOR_SETTLE_MS=600
CONFIG_TK_NEXT_DEBOUNCE_MS=30
CONFIG_TK_FULL_REFRESH_INTERVAL=8
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
  wins even on the tick its timeout expires. This is a fix to the original
  pattern, which checked the timeout first and could send a state that had just
  succeeded to the fail state instead.
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

**Platforms.** `qemu_xtensa/dc233c` is the default: a full Zephyr kernel on
macOS, same architecture as the target. `native_sim` is faster and is the only
host platform that emulates GPIO, but it builds on Linux only — so suites that
drive the selector and Next need it, and that is what `just fw-test-linux`
(Docker) and CI are for.

**Fixtures.** `qdb` tests will run against real question bundles rather than
hand-written bytes. `just fw-fixtures` builds them from the question database
and decompresses them, since the device stores raw TKB2 and the gzip is only a
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

## 10. Errors we already hit

These all happened while setting this up. If you see one, this is the fix.

**`esptool>=5.0.2 not found in PATH`**
Zephyr's espressif CMake looks on PATH, not in the venv. The justfile prepends
`.venv/bin` for every recipe, so use `just fw-build` rather than calling `west`
directly. If you must call west by hand:
`PATH="$PWD/.venv/bin:$PATH" .venv/bin/west build …`

**`unrecognized platform - qemu_xtensa`**
Missing SoC qualifier. Use `qemu_xtensa/dc233c`. See section 3.

**`xtensa/config/core.h: No such file or directory`**
A module is missing from the allowlist in `firmware/west.yml`. This one was
`hal_xtensa`. The allowlist is narrow on purpose — importing all ~60 modules
costs several GB — so widening it when a build fails is the expected workflow.
Add the name, run `west update` (not `west update <name>`, which refuses for
imported projects).

**`zephyr/heap_constants.h: No such file or directory`**
A library added with `add_subdirectory` after `find_package(Zephyr)` misses the
generated-header dependency, so it can compile before those headers exist.
`firmware/lib/fsm/CMakeLists.txt` shows the fix:
`add_dependencies(<lib> zephyr_generated_headers)`.

**`undefined reference to` something in a lib you added**
Same root cause — Zephyr collects its own libraries before `find_package`
returns. Link it explicitly: `target_link_libraries(app PRIVATE <lib>)`.

**`invalid application of 'sizeof' to incomplete type`**
`ARRAY_SIZE(_transitions)` in a constructor defined inline in the class, above
the table's definition. Define the constructor after the table.

**`ninja: error: loading 'build.ninja'`**
A build directory left half-configured by an earlier failure. Delete it, or
build with `-p`.

**CMake 4.x complaining about a minimum required version**
CMake 4 dropped compatibility with `cmake_minimum_required(VERSION < 3.5)`.
Zephyr itself is fine; a third-party module might not be. If it happens:
`export CMAKE_POLICY_VERSION_MINIMUM=3.5`. It has not been needed here.

---

## 11. Conventions

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

---

## 12. Where to go next

The application state machine is the next piece. The design is settled — see
the diagrams in [docs/firmware_architecture.md](docs/firmware_architecture.md) —
and the order that keeps every step testable:

1. **`input`** — six selector GPIOs plus Next, publishing on zbus. Needs
   `CONFIG_ZBUS=y` and the 600 ms settle. Testable on `native_sim`.
2. **`qdb`** — TKB2 reader and the drawn-bitmap bag, against real fixtures.
   No hardware at all.
3. **`app_fsm`** — the tabletop machine, built on `lib/fsm`. Pure logic given
   fake input, so it tests under qemu.
4. **`panel`** — SSD1680 and text layout. The first part that genuinely needs
   the breadboard.

One loose end: `tools/*.py` predates the formatter and `just fmt-check` fails
on it. `just fmt-py && just lint-py` cleans it up in a commit of its own.
