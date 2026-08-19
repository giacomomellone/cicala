# Firmware tests

Twister runs the ztest suites:

```sh
just fw-test             # all suites under qemu
just fw-test input       # one suite
just fw-test-linux       # native_sim in the Zephyr CI container
```

`qemu_xtensa/dc233c` is the default because it runs on macOS and matches the
target architecture. `native_sim` is faster but requires Linux. Both platforms
can emulate GPIO through a `zephyr,gpio-emul` devicetree node.

| Suite | Covers |
|---|---|
| `smoke` | harness startup |
| `fsm` | generic transition and timeout rules |
| `input` | Category, Next, and debounce |
| `app_fsm` | tabletop states and refresh handling |
| `qdb` | bundle parsing, deck filters, and no-repeat draws |
| `layout` | UTF-8, accents, wrapping, and the shipped corpus |
| `panel` | refresh policy and framebuffer output |
| `power` | voltage thresholds, hysteresis, and charge window |
| `status` | LED priority and timing |
| `portal` | setup states, DNS, forms, and pages |
| `sync` | manifest parsing and version comparison |
| `retained` | RTC block validation |
| `integration` | emulated GPIO through real application channels and panel code |
| `soak` | repeated automatic draws and refresh policy |

A suite that needs the application's `CONFIG_KVELD_*` values adds a `Kconfig`
containing `rsource "../../Kconfig.policy"`, the same file `app/Kconfig` pulls
in. Bundle and layout suites use QDB3 fixtures generated from the real question
database by `just fw-test`; the fixture files are gitignored.

Timing, electrical behavior, and the e-paper glass require the breadboard. See
the [hardware checks](../README.md#hardware-checks).
