# Firmware tests

ztest suites run by twister:

```sh
just fw-test                        # all suites
just fw-test input                  # one suite
just simboard=native_sim fw-test    # on a Linux host
```

Most of this application is testable off-target. `qdb`, the bag, the text
layout and `Fsm` include no Zephyr headers, so they compile for a host platform
and for the ESP32-S3 from identical source. The state machine, the 600 ms
settle window and the drop-during-refresh rule are testable too, because the
inputs arrive as zbus messages and the display is a stub whose framebuffer the
test can read.

A suite that needs the application's `CONFIG_TK_*` policy values adds a
`Kconfig` with `rsource "../../Kconfig.policy"` — the same file `app/Kconfig`
pulls in, so a suite cannot keep passing against a window someone has changed.

Timing, power and the panel itself need hardware and are covered by the
breadboard acceptance list in [firmware/README.md](../README.md).

## Platforms

`qemu_xtensa/dc233c` runs a full Zephyr kernel on macOS and matches the target
architecture. It is the default so the daily loop works without Docker.

`native_sim` is faster and builds on Linux hosts only, so it is the CI platform
rather than the daily one.

Both emulate GPIO. `CONFIG_GPIO_EMUL` is selected by a `zephyr,gpio-emul` node
in the devicetree and is not tied to the host, so a suite that drives the
selector and Next runs on either — `tests/input` carries one overlay per
platform and is otherwise unremarkable.

The practical split: run `qemu_xtensa/dc233c` locally, let CI run `native_sim`.

## Suites

| Suite | Covers | Platform | Status |
|---|---|---|---|
| `smoke` | The harness itself builds and runs | any | present |
| `fsm` | Transition table, timeouts, fail-state entry | any | present |
| `input` | Settle window, detent crossing, invalid selector, Next debounce | any | present |
| `app_fsm` | Every edge of the tabletop machine, drop during refresh, refresh timeout | any | present |
| `qdb` | QDB2 parse, deck mask, depth filter, no repeat within a cycle, recent ring, fingerprint invalidation | any | present |
| `layout` | UTF-8, accent decomposition, word wrap, every shipped question fits | any | present |
| `panel` | Refresh policy, marks, every shipped question renders | any | present |
| `integration` | GPIO to panel through the real threads and channels | any | present |

`integration` is the one that checks seams rather than pieces: it builds every
application source except `main.c`, drives the emulated selector and Next, and
watches what reaches `chan_question` and `chan_render`. When something works in
isolation but not on the device, that is the suite to extend.

## Fixtures

`qdb` tests run against real bundles rather than hand-written bytes, which is
what the component notes require. `just fw-fixtures` builds them from the
question database and decompresses them, since the device stores raw QDB2 and
the gzip is only a transport encoding:

```
dist/bundles/bundle-en-<ver>.qdb.gz     gzip, as published
firmware/tests/fixtures/en.qdb      decompressed, what qdb actually reads
```

Both paths are gitignored. The question YAML stays the single source of truth,
and a corpus change is picked up by the next test run rather than needing a
committed blob to be refreshed.
