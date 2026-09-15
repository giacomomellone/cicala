# Rev A firmware: build, flash and bring up

Use `hw=rev_a` for the integrated PCB. It selects the ESP32-S3-WROOM-1-N16,
16 MiB flash without PSRAM, native USB console, charger status inputs,
switched battery sensing and panel power, and the case's 180° display rotation.
The default `hw=breadboard` has different power wiring and remains a separate
target. Run commands from the public repository containing `justfile`.

The firmware has been built and tested in simulation. It has not been flashed
to a physical Rev A board. The acceptance procedure in
`hardware/rev_a/REVIEW.md` covers first power, charging, display, radio and sleep
measurements. Perform initial USB work with the battery disconnected.

## Install the toolchain

Install Git, Python 3.12 or newer, Node/npm, `just`, CMake, Ninja and the device
tree compiler, plus the [Zephyr host prerequisites](https://docs.zephyrproject.org/latest/develop/getting_started/index.html).
On Apple Silicon, the OpenSCAD binary may additionally
need Rosetta; firmware compilation uses the host's Zephyr SDK.

```sh
just setup
just fw-init
.venv/bin/west sdk install --version 1.0.1 \
  --install-dir "$HOME/Projects/zephyr-sdk-1.0.1" \
  --gnu-toolchains xtensa-espressif_esp32s3_zephyr-elf xtensa-dc233c_zephyr-elf
just hw=rev_a fw-doctor
```

`firmware/west.yml` pins Zephyr v4.4.1. `fw-init` installs its modules, Python
packages and Espressif blobs and applies the repository's display-driver patch.
Keep SDK host tools enabled: they provide OpenOCD and QEMU. If the SDK is already
installed elsewhere, set `ZEPHYR_SDK_INSTALL_DIR` to that directory, or pass
`sdk=/absolute/path` to `just`. CMake 4 hosts may need
`export CMAKE_POLICY_VERSION_MINIMUM=3.5` for older third-party CMake files.

## Build the board images

```sh
just hw=rev_a fw-build debug
just hw=rev_a fw-build charset
just hw=rev_a fw-build release
just fw-test
```

| Profile   | Build directory              | Use                                                    |
| --------- | ---------------------------- | ------------------------------------------------------ |
| `debug`   | `build/cicala-rev-a-debug`   | USB console and JTAG, sleep disabled                   |
| `charset` | `build/cicala-rev-a-charset` | Existing glyph/display diagnostic; Next advances pages |
| `power`   | `build/cicala-rev-a-power`   | Raw ADC and battery-voltage logging, with MCUboot      |
| `portal`  | `build/cicala-rev-a-portal`  | Wi-Fi setup screen at every boot                       |
| `release` | `build/cicala-rev-a`         | Normal application and matching MCUboot                |

Debug, charset and portal are standalone diagnostic images. Release, power
and OTA profiles build MCUboot and a signed application together. The local
build uses the committed **development signing key**; its private half is
public. Follow [firmware updates](firmware_update.md) before making release-key
devices. This prototype workflow does not program secure-boot or USB/JTAG eFuses.

The reviewed `questions/` database currently contains zero accepted questions.
A normal release therefore embeds an empty corpus; it can show empty/setup
screens but cannot display a conversation question. Use the charset profile
for display bring-up. Human-authored, accepted content must enter the normal
database workflow before a content-bearing release is built. Firmware tests
use their separate existing fixture corpus.

## First USB flash

J1 accepts a USB-C **data cable** and provides power, ROM download, serial and
built-in JTAG. No separate programmer is required for the normal path.

1. Inspect the bare assembly and complete the current-limited first-power
   checks. Keep the battery and display disconnected until the rails pass.
2. Connect J1 to the computer. Identify the serial port:

   ```sh
   .venv/bin/python -m serial.tools.list_ports -v
   ```

   Typical ports are `/dev/cu.usbmodem…` on macOS and `/dev/ttyACM0` on Linux.
   Use the reported path; the examples below use Linux's usual name. Linux
   needs permission to open that serial device and the USB JTAG endpoint.

3. If the blank board does not enter download mode, connect BOOT to ground
   at TP1 or J4 pin 4. Momentarily ground EN at TP2 or J4 pin 3, then release
   EN while BOOT remains grounded. Release BOOT after the port enumerates.
   TP3 is ground. Keep the PCB supported and avoid slipping probes.
4. Close any serial monitor, then flash the debug profile:

   ```sh
   just hw=rev_a port=/dev/ttyACM0 fw-flash debug
   just hw=rev_a usbport=/dev/ttyACM0 fw-monitor debug
   ```

   Exit the monitor with Ctrl-]. The port can change after a reset; enumerate
   it again if necessary. The explicit `usbport` selects the debug monitor;
   `port` selects the flash and normal monitor. Both can also be set through
   `CICALA_USB_PORT` and `ESPTOOL_PORT`.

5. Verify console output and the charger status log. A charger fault is expected
   while the battery/NTC harness is disconnected. Power down, connect the
   display with J2 contacts facing the PCB, then flash the charset profile:

   ```sh
   just hw=rev_a port=/dev/ttyACM0 fw-flash charset
   just hw=rev_a port=/dev/ttyACM0 fw-monitor charset
   ```

   Check the installed display's orientation, full refresh and Next input.
   Fit and tune both printed caps before closing the case.

6. Flash the normal bootloader/application pair:

   ```sh
   just hw=rev_a port=/dev/ttyACM0 fw-flash release
   just hw=rev_a port=/dev/ttyACM0 fw-monitor release
   ```

`west flash` uses the generated per-domain runner settings: MCUboot at
`0x0`, signed application at `0x20000`, DIO/80 MHz/16 MB for this build.
Use the recipe to keep addresses and images together. Flashing only the
signed application onto a blank board omits MCUboot. Do not use DevKit N8R8
or generic ESP-IDF partition-table instructions with this image.

## Debug and recover

Flash the debug image, then start the supplied USB JTAG configuration:

```sh
just hw=rev_a fw-debugserver
```

In another terminal, run the SDK's
`xtensa-espressif_esp32s3_zephyr-elf-gdb` with
`build/cicala-rev-a-debug/zephyr/zephyr.elf`. For SDK 1.0.1 its executable
is under `gnu/xtensa-espressif_esp32s3_zephyr-elf/bin/`.

```gdb
target remote :3333
monitor reset halt
thbreak main
continue
```

`fw-doctor` prints the executable path. Debug has sleep disabled so the USB
endpoint remains available. The application console uses native USB in all
Rev A profiles; early ROM/MCUboot diagnostics may appear on UART0 instead.
Upstream OpenOCD does not provide full Zephyr thread awareness for this target.

UART recovery uses the unpopulated TC2030-IDC-NL pattern at J4. Remove the
base and disconnect/move the battery. Wire a breakout to this **project-specific**
mapping; an ordinary ARM cable mapping is incompatible.

| J4 pin | Board signal      | Adapter connection                                              |
| ------ | ----------------- | --------------------------------------------------------------- |
| 1      | 3V3               | Target voltage reference only; leave adapter power disconnected |
| 2      | GND               | Ground                                                          |
| 3      | EN                | Momentary pull to ground for reset                              |
| 4      | GPIO0 / BOOT      | Ground while releasing reset for ROM download                   |
| 5      | GPIO43 / UART0 TX | 3.3 V adapter RX                                                |
| 6      | GPIO44 / UART0 RX | 3.3 V adapter TX                                                |

Power the target through J1, use 3.3 V logic, enter ROM mode as above and run
`fw-flash` with the UART adapter's port. J4 pin 1 must not back-power the
regulator. GPIO38's factory-reset test contact is reserved; normal Wi-Fi
setup uses the two front buttons.

## Normal use and power checks

Category selects a deck; Next advances within it when accepted content is
available. Hold both buttons for two seconds during boot to enter Wi-Fi setup,
or use the portal profile. Follow the SSID and password on the display.

The BQ25185 status pins are sampled together once per power sample. Rev A
samples at a one-second interval after the previous work completes, with
200 ms divider settling; display activity can defer sampling. The four
pin combinations follow [TI's status table](https://www.ti.com/lit/ds/symlink/bq25185.pdf).

| STAT1 / STAT2 | Interpretation                     | Idle LED output  |
| ------------- | ---------------------------------- | ---------------- |
| High / low    | Charging                           | Steady red       |
| High / high   | Idle, charge complete, or disabled | Slow amber pulse |
| Low / high    | Recoverable charger fault          | Slow red pulse   |
| Low / low     | Latched charger fault              | Slow red pulse   |

An unreadable status input also produces the ambiguous amber state and a log
message. Green pulses indicate network work; steady amber indicates setup.
Charge faults take priority over those activities. Battery-only low/critical
states produce the existing amber/red blink alerts. Rev A does not infer a
definite full battery from voltage or high/high status. The log distinguishes
recoverable and latched faults, and firmware does not automatically reset them.

GPIO14 low requests charging; the independent temperature guard still controls
whether CE can enable it. Do not bypass the bonded NTC. To inspect conversion,
build/flash the `power` profile and compare its logged pack voltage with a meter.

On battery, normal firmware sleeps after two seconds of inactivity, retaining
the displayed image. It stops ADC work, switches off the divider and LED, parks
the display bus physically low, disables the panel rail and holds those pins.
The first later refresh restores controller RAM with a full update. Either
front button wakes the device. USB insertion alone does not wake a sleeping
board by default; press a button to start the console/network window. Charging
hardware can operate while the MCU sleeps. Disconnect USB/JTAG before measuring
the whole-board sleep-current target of less than 30 µA.

## Repeat the combined checks

With KiCad 10, OpenSCAD and Python NumPy/Shapely installed:

```sh
just hw=rev_a fw-build release
just hw-review build/hardware-review \
  --app build/cicala-rev-a/app/zephyr \
  --boot build/cicala-rev-a/mcuboot/zephyr
just fw-test
```

`CICALA_HW_PYTHON` selects the analysis Python; `KICAD_PYTHON_BIN` selects one
with `pcbnew`. The combined check compares PCB datums, case dimensions, actual
USB routes and compiled GPIO/flash/display settings. Reports belong to the
source version checked. After changing placement or STEP models, regenerate
the case's component envelopes and repeat the checks and exports.

The reviewed run passed 226 firmware cases across 16 QEMU configurations;
three platform-specific cases were skipped. It includes both breadboard and
Rev A charger integration, ADC failure cleanup, fault transitions and stopping
pending ADC/LED work before sleep. Physical flash, JTAG, RF, charge thresholds,
display operation and current remain first-prototype acceptance measurements.
