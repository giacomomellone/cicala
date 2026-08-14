# Hardware and wiring

This page describes the breadboard rig and its bench checks. See
[prototype BOM](prototype_bom.md) for parts, [device prototype](device_prototype.md)
for the product design, and [firmware architecture](firmware_architecture.md)
for firmware behavior.

The rig includes the two buttons, e-paper panel, charger, cell, voltage dividers,
and separate red and green status LEDs. The DevKitC indicators make sleep-current
measurements invalid for the 30 µA target; use rev A or a separate power mule for
that measurement.

## Wiring overview

```text
   cell ──JST──┐
               │
      ┌────────┴─────────────────────────────┐
      │        Adafruit 6092 · bq25185       │
      │   USB-C ──► charge input             │
      │                                      │
      │   ● VU      ● BAT      ● 3V     ● −  │
      └─────┬──────────┬─────────┬────────┬──┘
            │          │         │        │
       100k/150k   470k/470k     │        │
            │      + 100 nF      │        │
            ▼          ▼         ▼        ▼
         GPIO21      GPIO1      3V3      GND ─────┐
      ┌──────────────────────────────────────┐    │
      │        ESP32-S3-DevKitC-1            │    │
      │                                      │    │
      │  GPIO4  ──── Category switch ────────┼────┤
      │  GPIO17 ──── Next switch ────────────┼────┤
      │  GPIO15 ──── R ──►|── red ───────────┼────┤
      │  GPIO16 ──── R ──►|── green ─────────┼────┤
      │                                      │    │
      │  GPIO10 11 12 13 · 18 8 9 ──── HAT   │    │
      │  3V3, GND ───────────────────── HAT ─┼────┤
      └──────────────────────────────────────┘    │
                                              − rail
```

Connect every ground to one rail: charger, dividers, capacitor, DevKitC,
switches, LEDs, and panel.

## Pin assignment

| GPIO | Function         | Direction      | Notes                            |
| ---: | ---------------- | -------------- | -------------------------------- |
|    4 | Category         | input, pull-up | active low; RTC-capable          |
|   17 | Next             | input, pull-up | active low; RTC-capable          |
|   10 | e-paper CS       | output         | SPI2                             |
|   11 | e-paper MOSI     | output         | SPI2                             |
|   12 | e-paper SCK      | output         | SPI2                             |
|   13 | SPI MISO         | input          | claimed by SPI2; unused by panel |
|   18 | e-paper D/C      | output         | active high                      |
|    8 | e-paper reset    | output         | active low                       |
|    9 | e-paper busy     | input          | active high                      |
|    1 | battery sense    | ADC1_CH0       | 470 kΩ / 470 kΩ divider          |
|   21 | VBUS detect      | input          | 100 kΩ / 150 kΩ divider          |
|   15 | red status LED   | output         | active high                      |
|   16 | green status LED | output         | active high                      |

Wake inputs must use RTC-capable GPIO0–21. GPIO19 and GPIO20 remain free for
native USB. GPIO0 and GPIO3 are strapping pins; GPIO26–32 are used by flash;
GPIO33–37 are used by octal PSRAM; GPIO43/44 carry UART0; GPIO48 drives the
onboard WS2812.

GPIO0–31 belong to Zephyr controller `gpio0`; GPIO32–53 belong to `gpio1`. For
example, GPIO48 is `<&gpio1 16 ...>`.

Battery sensing uses ADC1 because ADC2 shares hardware with Wi-Fi. GPIO2, 5, 6,
7, and 14 remain spare and RTC-capable.

## Voltage dividers

Use 1% metal-film resistors. The 100 nF battery-sense capacitor must be ceramic
(X7R or C0G); electrolytic leakage causes a large error at this impedance. An
1/8 W rating is sufficient.

### Battery sense

```text
   BAT (3.0–4.2 V)
      │
    470 kΩ
      │
      ├────────────┬────────────► GPIO1
      │            │
    470 kΩ        100 nF
      │            │
      └────────────┴────────────► GND
```

The tap is half the cell voltage. Place the capacitor across the lower resistor,
close to GPIO1. `CONFIG_TK_POWER_DIVIDER_NUM=2` and
`CONFIG_TK_POWER_DIVIDER_DEN=1` convert the ADC reading back to cell voltage.

| Resistor pair   | Current at 4.2 V | Source impedance |
| --------------- | ---------------: | ---------------: |
| 100 kΩ / 100 kΩ |            21 µA |            50 kΩ |
| 470 kΩ / 470 kΩ |           4.5 µA |           235 kΩ |
| 1 MΩ / 1 MΩ     |           2.1 µA |           500 kΩ |

The BOM uses 470 kΩ. Rev A switches this divider off during sleep. Connect the
divider to `BAT`, which follows the cell, rather than the regulated 3.3 V rail.

### VBUS detect

```text
   VU (about 5 V with charger USB-C connected)
      │
    100 kΩ
      │
      ├─────────────────────────► GPIO21
      │
    150 kΩ
      │
      └─────────────────────────► GND
```

The tap is about 3.0 V. A 100 kΩ / 100 kΩ divider leaves too little margin over
the ESP32-S3 input-high threshold. Connect the divider to `VU`; `VIN` can reach
18 V and would overvoltage GPIO21. DC or solar power applied at `VIN` is not
detected as VBUS.

Before fitting either divider, connect GPIO1 and GPIO21 to ground. Both pins are
read by the normal firmware and must not float. Remove each jumper only after
checking the corresponding divider tap with a meter.

The buttons are the deep-sleep wake sources. USB insertion is sampled on the
next boot; `CONFIG_TK_POWER_WAKE_ON_USB` is disabled because the ESP32-S3 cannot
mix EXT1 trigger polarities per pin.

## Buttons

Both buttons are active low and use internal pull-ups. Connect one leg from each
internally joined pair of a tactile switch:

```text
   switch viewed from above

     1 ●━━━━━━━━━● 2
       ╎  ▄▄▄▄▄  ╎
     3 ●━━━━━━━━━● 4

   GPIO ───● 1        4 ●─── GND
```

Connect Category to GPIO4 and Next to GPIO17. Continuity should be open when
released and closed when pressed.

## Status LEDs

```text
GPIO15 ── resistor ──►|── GND    red
GPIO16 ── resistor ──►|── GND    green
```

The long leg is the anode and faces the resistor. Choose resistor values for a
clear but subdued indication across a table.

| State               | Red          | Green      |
| ------------------- | ------------ | ---------- |
| Healthy on battery  | off          | off        |
| Charging            | on           | off        |
| Charged             | off          | on         |
| Setup portal        | on           | on         |
| Sync or update      | off          | slow pulse |
| Low-battery warning | one blink    | one blink  |
| Refresh refused     | three blinks | off        |

The breadboard uses separate LEDs. Rev A uses one bi-color package, where both
dies appear amber when lit together.

## E-paper panel

Connect the Waveshare HAT through its 40-pin header:

```text
   pin 1  VCC  → 3V3       pin 6  GND  → GND
   pin 11 RST  → GPIO8     pin 18 BUSY → GPIO9
   pin 19 DIN  → GPIO11    pin 22 DC   → GPIO18
   pin 23 CLK  → GPIO12    pin 24 CS   → GPIO10
```

The overlay targets the 250 × 122 V3/V4 panel:

| Revision | Devicetree compatible     | Controller | Size      |
| -------- | ------------------------- | ---------- | --------- |
| V3 / V4  | `gooddisplay,gdey0213b74` | SSD1680    | 250 × 122 |
| V2       | `gooddisplay,gdeh0213b72` | SSD1675A   | 250 × 120 |
| V1       | `gooddisplay,gdeh0213b1`  | SSD1673    | 250 × 120 |

Set the compatible and height to match the board marking. The panel uses
MIPI-DBI; [firmware architecture](firmware_architecture.md#display) describes
the refresh policy.

## Power and cables

Connect the charger's `3V` output to DevKitC `3V3` and `−` to GND. This bypasses
the DevKitC regulator. Disconnect the cell, or open a switch in its lead, before
powering the DevKitC from USB: otherwise both regulators drive the 3.3 V rail.

The charger `VU` signal detects only the charger USB-C input. A DevKitC cable
powers the board without asserting VBUS detect, so remove all DevKitC cables for
battery and current measurements.

| Job                      | Cell                    | Charger USB-C | DevKitC UART        | DevKitC USB  |
| ------------------------ | ----------------------- | ------------- | ------------------- | ------------ |
| Flash and normal console | disconnected            | disconnected  | connected           | optional     |
| Debug console and JTAG   | disconnected            | disconnected  | connected for flash | connected    |
| Charge-state check       | connected               | connected     | disconnected        | disconnected |
| Battery behavior         | connected               | disconnected  | disconnected        | disconnected |
| Sleep current            | connected through meter | disconnected  | disconnected        | disconnected |

The UART jack appears as `/dev/cu.usbserial-*` and carries flashing and the
normal console. The native USB jack appears as `/dev/cu.usbmodem*` and carries
JTAG plus the debug profile's console. Opening the UART port resets the chip via
the CP2102 auto-reset circuit.

Rev A routes charger USB D+/D− to GPIO19/20. Native-USB flashing through that
route remains a hardware verification item.

## Bring-up checks

Perform these checks in order.

1. With nothing connected, confirm that the 6092 charge-rate jumper is open.
   Check that cell polarity matches the charger marking and that the cell reads
   3.6–4.0 V. A reversed cell can damage the charger and cell.
2. Connect the charger and cell. Confirm `3V` is 3.30 V ±0.05 and `4.5V` is near
   cell voltage. Connect charger USB-C briefly; `VU` should be about 5 V and the
   charge LED should light.
3. Measure each divider before connecting it to the MCU. Battery sense should be
   half the cell voltage. VBUS sense should be 2.9–3.1 V with charger USB-C
   connected and about 0 V without it.
4. Touch each LED resistor to 3.3 V and confirm polarity.
5. With DevKitC USB disconnected, connect charger `3V` to DevKitC `3V3` and
   charger `−` to GND. Confirm 3.30 V ±0.05 at the DevKitC.
6. Disconnect the cell before connecting the DevKitC UART cable for flashing.

Build the power profile to inspect ADC and VBUS readings:

```sh
just fw-flash power
just fw-monitor power
```

Compare the reported cell voltage with a meter at `BAT`; the error should be
within about 2%. Connect and disconnect charger USB-C and confirm that VBUS
follows on the next sample. Check charging red, charged green, portal amber, and
the three-red-blink refresh refusal.

Restore the release profile and test on the cell with all DevKitC cables removed:

```sh
just fw-flash
```

The panel should respond to both buttons, the LEDs should follow charger state,
and the device should sleep two seconds after external power is removed.

## Functional checks

```sh
just fw-flash
just fw-monitor
```

- Boot selects New People and shows its category name.
- Category advances through all six categories and wraps.
- Next shows one question per accepted press.
- Holding and releasing a button produces one event.

Ground GPIO1 and GPIO21 when the dividers are absent. A floating GPIO21 can keep
the device awake; a floating GPIO1 can make the firmware reject refreshes.

## Bench measurements

Measured with a Waveshare 2.13-inch V4 panel:

| Measurement                   | Result                                             | Configuration                        |
| ----------------------------- | -------------------------------------------------- | ------------------------------------ |
| Consecutive partial refreshes | minor artifacts at 193; visible ghosting before 64 | `TK_FULL_REFRESH_INTERVAL=16`        |
| Partial refresh duration      | 622 ms ±2 ms across 193 refreshes                  | below the 1 s product target         |
| Refresh after deep-sleep wake | 624 ms                                             | Zephyr panel patch applied           |
| Peak thread stack             | logging 89%; other threads 18–38%                  | `LOG_PROCESS_THREAD_STACK_SIZE=2048` |
| RTC memory across warm reboot | retained across three runs                         | retained block works on ESP32-S3     |
| Battery reading               | 3890 mV reported; 3930 mV measured                 | divider ratio remains 2/1            |
| VBUS detection                | followed first sample after connect and disconnect | sync and sleep inputs work           |

Open measurements:

- full-refresh duration at the BUSY pin;
- lowest cell voltage that gives a clean partial refresh;
- discharge curve;
- idle, refresh, Wi-Fi, LED, and sleep current.

### Measurement profiles

Use `just fw-flash <profile>` and `just fw-monitor <profile>`.

| Profile   | Purpose                                                                                                        |
| --------- | -------------------------------------------------------------------------------------------------------------- |
| `soak`    | Draw a new question every five seconds with periodic full refresh disabled. Record the first visible ghosting. |
| `retain`  | Warm reboot every three presses. Confirm sequence and partial-refresh state survive.                           |
| `charset` | Draw every supported diacritic and inspect placement.                                                          |
| `power`   | Log raw and converted ADC values once per second. Use for divider calibration and discharge measurements.      |

For the refresh floor, replace the cell with a bench supply and reduce voltage
in steps while requesting partial refreshes. Set `TK_REFRESH_MIN_MV` above the
first voltage that leaves visible artifacts. Below the configured floor, the
panel should remain unchanged and the red LED should blink three times.

Measure current in series with the cell and remove every DevKitC USB cable. The
DevKitC power LED and onboard WS2812 add roughly 1–2 mA, so this rig cannot
validate the 30 µA sleep budget.
