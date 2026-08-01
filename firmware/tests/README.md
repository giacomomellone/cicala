# Firmware tests

ztest suites run by twister:

```sh
just fw-test                        # all suites
just fw-test bag                    # one suite
just simboard=native_sim fw-test    # on a Linux host
```

Most of this application is testable off-target. `qdb`, the bag, the text
layout and `Fsm` include no Zephyr headers, so they compile for a host platform
and for the ESP32-S3 from identical source. The state machine, the 600 ms
settle window and the drop-during-refresh rule are testable too, because the
inputs arrive as zbus messages and the display is a stub whose framebuffer the
test can read.

Timing, power and the panel itself need hardware and are covered by the
breadboard acceptance list in [firmware/README.md](../README.md).

## Platforms

`native_sim` is the fastest option and the only one that emulates GPIO, so the
suites that drive the selector and Next need it. It builds on Linux hosts only.

`qemu_xtensa` runs a full Zephyr kernel on macOS and matches the target
architecture, which covers everything except the GPIO-driven suites. It is the
default so the daily loop works without Docker.

The practical split: run `qemu_xtensa` locally, let CI run `native_sim`, and
keep the GPIO-driven suites in their own directory so a macOS run skipping them
is obvious rather than silent.

## Suites

| Suite | Covers | Platform | Status |
|---|---|---|---|
| `smoke` | The harness itself builds and runs | any | present |
| `fsm` | Transition table, timeouts, fail-state entry | any | planned |
| `bag` | No repeat within a cycle, recent ring, fingerprint invalidation | any | planned |
| `qdb` | TKB2 parse, deck mask, depth filter, round-trip of real bundles | any | planned |
| `layout` | UTF-8 word wrap, German coverage, longest question fits | any | planned |
| `app` | Settle window, drop during refresh, invalid selector handling | `native_sim` | planned |

## Fixtures

`qdb` tests run against real bundles rather than hand-written bytes, which is
what the component notes require. `just fw-fixtures` builds them from the
question database and decompresses them, since the device stores raw TKB2 and
the gzip is only a transport encoding:

```
dist/bundles/bundle-en-<ver>.tkb     gzip, as published
firmware/tests/fixtures/en.tkb2      decompressed, what qdb actually reads
```

Both paths are gitignored. The question YAML stays the single source of truth,
and a corpus change is picked up by the next test run rather than needing a
committed blob to be refreshed.
