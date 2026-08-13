# Hardware and wiring

The breadboard rig: what connects to what, and why each pin is the pin it is.
The shopping list is [prototype bom](prototype_bom.md), the object this becomes
is [device prototype](device_prototype.md), and the firmware that drives it is
[firmware architecture](firmware_architecture.md).

**Status:** the buttons and the panel are wired and drawing questions, and the
refresh policy, timings and RTC retention have been measured — see "Bench
measurements" at the end, which is also the procedure for repeating any of them.

The power path is **written in firmware and not yet built in copper.** The
charger, the cell, the two dividers and the two status LEDs describe what the
firmware expects; nothing marked *verify* has been measured. "Bringing the rig
up" builds it, in the order that keeps a mistake cheap.

## Parts

| Part | Role |
|---|---|
| ESP32-S3-DevKitC-1-N8R8 | The firmware target. 8 MB flash, 8 MB PSRAM, native USB and a CP2102 UART bridge |
| Waveshare 2.13-inch e-Paper HAT | 250 × 122, SSD1680 controller, 3.3 V logic |
| Two 6 mm tactile switches | Category and Next; stand in for the KSC321G |
| Adafruit 6092 | bq25185 USB/DC/solar charger with a TPS62569 3.3 V/1 A buck. Powers the rig and charges the cell |
| EEMB 524261, 3.7 V 1500 mAh LiPo | Bench cell, JST-PH. Three times the capacity of the 503035 the product is designed around, so it discharges slowly enough to measure |
| Red and green LED, one resistor each | Status. Two packages here, one bi-colour package on the target board |
| 2 × 470 kΩ (or 2 × 1 MΩ), 100 kΩ, 150 kΩ, 100 nF ceramic | The two dividers |
| Breadboard and jumpers | — |

Everything runs at 3.3 V, so no level shifting anywhere.

The 100 nF is **ceramic**, X7R or C0G. Not an electrolytic: leakage on the order
of a microamp across a 235 kΩ source is tenths of a volt of error in the reading
the capacitor is there to steady.

**1/8 W is ample for every resistor here.** The dividers dissipate microwatts —
single-digit µA through the battery pair, 20 µA through the VBUS pair — and an
LED run at a few milliamps puts single-digit milliwatts in its series resistor,
against a 125 mW rating. What the dividers do want is **1 % metal film** rather
than 5 % carbon: the ratio scales the reading directly, and
`TK_POWER_DIVIDER_NUM`/`_DEN` exist to absorb what is left.

## The whole rig

```
   cell ──JST──┐
               │
      ┌────────┴─────────────────────────────┐
      │        Adafruit 6092 · bq25185       │
      │   USB-C ──►  charge in               │
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

One `−` rail, and everything returns to it: the charger, both divider bottoms,
the capacitor, the DevKitC's GND, the switches, the LEDs and the panel.

## Pin assignment

Two constraints drive the whole map:

1. **Every wake input must be RTC-capable.** On the ESP32-S3 that is GPIO0–21.
   Outside that range EXT1 deep-sleep wake is impossible, and the entire power
   budget depends on it.
2. **GPIO19 and GPIO20 are the native USB pair.** They carry the debug console
   and the JTAG interface OpenOCD uses. Anything else on them breaks both.

| GPIO | Function | Direction | Notes |
|---:|---|---|---|
| 4 | Category | in, pull-up | active low |
| 17 | Next | in, pull-up | active low |
| 10 | e-paper CS | out | fixed by the board's `spim2_default` |
| 11 | e-paper MOSI (DIN) | out | as above |
| 12 | e-paper SCK (CLK) | out | as above |
| 13 | SPI MISO | in | unused by the panel; claimed by the bus |
| 18 | e-paper D/C | out | active high |
| 8 | e-paper RST | out | active low; **not 19** |
| 9 | e-paper BUSY | in | active high; **not 20** |
| 1 | battery sense | in, ADC1_CH0 | through 470k/470k |
| 21 | VBUS detect | in | through 100k/150k |
| 15 | status LED, red | out | active high |
| 16 | status LED, green | out | active high |

Avoided: GPIO0 and GPIO3 are strapping pins, GPIO26–32 are the SPI flash,
GPIO33–37 are the octal PSRAM on the N8R8, GPIO43/44 are the UART0 console, and
GPIO48 is the onboard WS2812.

Spare and RTC-capable: **GPIO2, 5, 6, 7 and 14.** Four of the five are ADC1
channels, which is the state the map is kept in — the measurement that cannot go
anywhere else has somewhere to go, and so does the next one.

Battery sense needs ADC1 (GPIO1–10) because **ADC2 shares its hardware with
Wi-Fi** and a reading taken during a sync can simply fail. GPIO1 is taken rather
than one of GPIO5–7 so that run of three stays contiguous for rev A. VBUS detect
is digital, so it takes GPIO21 and spends no channel. The LEDs are on 15 and 16
for the same reason: an LED needs neither an ADC channel nor RTC capability.

## The two dividers

A divider is two resistors in series across a voltage. The junction between them
— the **tap** — sits at a fraction of it, set by the ratio. On a breadboard the
tap is one row: the lower leg of the upper resistor, the upper leg of the lower
resistor, and the jumper to the GPIO.

### Battery, 470k/470k with 100 nF

```
   BAT pad  (the cell: 3.0 – 4.2 V)
      │
    ┌─┴─┐
    │   │  470 kΩ
    └─┬─┘
      │
      ├────────────┬──────────────►  GPIO1
      │            │                 ← tap: half the cell, 1.5 – 2.1 V
    ┌─┴─┐         ═╪═ 100 nF
    │   │  470 kΩ  │
    └─┬─┘          │
      │            │
   ───┴────────────┴───  − rail
```

Half the pack, so a 4.2 V cell arrives as 2.1 V — inside what 12 dB attenuation
reads, with room above. `CONFIG_TK_POWER_DIVIDER_NUM`/`_DEN` = 2/1 is the
firmware undoing the ratio.

The capacitor sits across the lower resistor, physically next to the pin. It is
not optional: the SAR ADC wants a low source impedance and this is 235 kΩ, so
without a reservoir the reading wanders.

**Any equal pair works, and the value is a trade.** Unlike the VBUS divider this
one is across the cell forever, so what it costs is standing current; what it
buys is a source the ADC can drive.

| Pair | Standing draw at 4.2 V | Source impedance |
|---|---|---|
| 100k / 100k | 21 µA | 50 kΩ |
| 470k / 470k | 4.5 µA | 235 kΩ |
| 1M / 1M | 2.1 µA | 500 kΩ |

All three are 2:1, so the tap and `CONFIG_TK_POWER_DIVIDER_NUM`/`_DEN` do not
change with the choice. 470k is what the BOM lists; **1M is better and is the
one to fit if it is what is on hand**, since it halves a current that is paid
forever. Its stiffer source shows up as ADC input leakage — tens of nA, so
roughly 25 mV of offset against 12 mV at 470k — which is nothing beside a
3200 mV floor, and is an offset rather than a ratio, so it is the reason bench
measurement 5 asks for readings at both ends of the range.

100k is the one to avoid, and only for the current: 21 µA is 70 % of a 30 µA
budget. On this rig it is invisible anyway — the DevKitC's own power LED draws a
hundred times more — so fit it if it is all you have and move on.

`hardware/pcb/README.md` still requires rev A to switch this divider, whatever
value it ends up with, and it should.

Tap **BAT**, which is the cell — not the 3.3 V rail. The buck holds that flat
above roughly 3.45 V and then passes the cell through at full duty, so the rail
says nothing about state of charge over most of a discharge.

### VBUS, 100k/150k

```
   VU pad  (≈5 V, only while the 6092's USB-C is in)
      │
    ┌─┴─┐
    │   │  100 kΩ
    └─┬─┘
      │
      ├───────────────────────────►  GPIO21
      │                              ← tap: 3.0 V
    ┌─┴─┐
    │   │  150 kΩ
    └─┬─┘
      │
   ───┴───  − rail
```

VBUS is 5 V and the pin is 3.3 V tolerant only. Not 100k/100k: 2.5 V sits under
the ESP32-S3's V_IH of 0.75 × VDD ≈ 2.48 V by too little to trust across
tolerance and droop. 100k/150k puts it at 3.0 V with margin at both ends, and
draws 20 µA only while something is plugged in — when the current is somebody
else's.

Tap **VU**, not VIN. VU is the USB rail; VIN accepts 5–18 V from a DC or solar
source, and 18 V through this divider is 10.8 V on a 3.3 V pin. The consequence:
**charging from DC or solar is invisible to the firmware.** The device runs, and
believes it is on battery.

### Before either is fitted

**Ground both pins.** Two jumpers to the − rail, and the everyday image runs on
devkit USB with no charger board at all. `CONFIG_TK_POWER` is on in the board
conf, so the firmware reads these pins whether or not anything is attached to
them.

```
   no charger board yet

      ┌──────────────────────────┐
      │    ESP32-S3-DevKitC-1    │
      │                          │        USB jack supplies 3V3
      │   GPIO1  ●───────────────┼───┐    through the board's own LDO
      │   GPIO21 ●───────────────┼───┤
      │   GND    ●───────────────┼───┤
      └──────────────────────────┘   │
                                  ───┴───  − rail
```

Both jumpers come out when the dividers go in, one at a time — see stage 2 of
"Bringing the rig up", which measures each tap before it touches a pin.

**GPIO21**, because a floating input can read high, and a device that believes it
is plugged in never sleeps — awake on the cell until the cell is flat, joining a
network on every press. Software cannot tell that reading from a real plug-in: it
is a digital pin, and high is high.

**GPIO1**, because the plausibility floor covers most of a floating pin's range
but not all of it. Below `CONFIG_TK_POWER_PLAUSIBLE_MV` a reading is treated as
no cell rather than as a flat one, which is what makes an unfitted divider
harmless — but that threshold is a 1250 mV tap, and the refresh floor is a
1600 mV tap:

| Floating tap reads | The firmware concludes | Result |
|---|---|---|
| under 1.25 V | not a cell — `UNKNOWN` | draws questions |
| **1.25 – 1.6 V** | **a flat cell** | **every press refused** |
| over 1.6 V | a healthy cell | draws questions |

Roughly a sixth of the pin's range lands in the middle row, and a high-impedance
input picking up mains hum can sit anywhere. Grounded it reads 0, which is the
top row, so one jumper removes the question.

Neither pin is a wake source in the image `just fw-build` produces. EXT1 takes
one trigger polarity for the whole mask, the buttons have claimed active-low, and
the ESP32-S3 does not define `SOC_PM_SUPPORT_EXT1_WAKEUP_MODE_PER_PIN`. EXT0 does
it and costs a powered RTC domain on every sleep, which is why
`CONFIG_TK_POWER_WAKE_ON_USB` exists and is off; turn it on to measure that cost.
With it off, plugging in does not wake the device — the next press does, and that
boot finds VBUS high and opens the sync window.

## Wiring the switches

Active low with internal pull-ups from the devicetree, so a switch simply shorts
its pin to ground. **No external pull-up or pull-down** — it does nothing useful.

A 6 mm tactile switch has four legs in two internally shorted pairs. Use two
**diagonally opposite** legs and leave the other two unconnected.

```
   from above

     1 ●━━━━━━━━━● 2      ━━ joined inside the package, always
       ╎  ▄▄▄▄▄  ╎
     3 ●━━━━━━━━━● 4      pressing bridges the two rows

   so take one leg from each row, diagonally:

       GPIO4 ───● 1        4 ●─── − rail
```

Category to GPIO4, Next to GPIO17.

If both legs come from the same pair the pin sits at ground forever, and the
symptom is silence rather than a stuck reading: the driver reports only edges, so
a permanently shorted pin produces no event at all. Check on continuity — open
unpressed, closed held.

## Wiring the status LEDs

```
   GPIO15 ──────[ R ]──────►|──────  − rail
                          red LED
```

The triangle is the anode, which is the long leg, and it faces the resistor. The
bar is the cathode — the flat side of the package, the short leg — and it goes to
the − rail. GPIO16 drives green the same way.

Both are active high, so a reversed LED gives no light and no error message.
Check them by hand before flashing, or a dark LED reads as a firmware bug.

Size the resistors for something visible across a table rather than for maximum
brightness. This is an object that sits in a room with people in it.

Lighting both is amber, and that is the whole palette:

| What is happening | Red | Green | Looks like |
|---|---|---|---|
| Nothing, healthy cell | off | off | dark |
| Charging | on | off | red |
| Charged | off | on | green |
| Portal on air | on | on | amber |
| Sync or update running | off | pulsing | slow green pulse |
| Press refused, cell low | blink ×1 | blink ×1 | one amber blink |
| Press refused, cell flat | blink ×3 | off | three red blinks |

Two separate packages will not blend into amber the way the bi-colour part on the
target board does — they read as a red LED and a green LED both lit. Every
transition and priority rule is still exercised; only the appearance waits for
rev A.

The pairs that share a colour are separated by rhythm rather than hue, because
with two dice there is no third colour: a low-battery blink against a steady
portal amber, and a sync pulse against a steady charged green.

## Wiring the panel

The HAT is a Raspberry Pi form factor, so everything comes off its 40-pin header.

```
   Waveshare HAT, 40-pin header from above

     1 ● VCC → 3V3        2 ●
     3 ●                  4 ●
     5 ●                  6 ● GND  → − rail
     7 ●                  8 ●
     9 ●                 10 ●
    11 ● RST  → GPIO8    12 ●
    13 ●                 14 ●
    15 ●                 16 ●
    17 ●                 18 ● BUSY → GPIO9
    19 ● DIN  → GPIO11   20 ●
    21 ●                 22 ● DC   → GPIO18
    23 ● CLK  → GPIO12   24 ● CS   → GPIO10
```

### Which revision

Waveshare has shipped three electrically different 2.13-inch panels under the
same name, and they need different controller profiles. Check the marking on the
board and set the devicetree compatible to match:

| Revision | Compatible | Controller | Size |
|---|---|---|---|
| V3 / V4 | `gooddisplay,gdey0213b74` | SSD1680 | 250 × 122 |
| V2 | `gooddisplay,gdeh0213b72` | SSD1675A | 250 × 120 |
| V1 | `gooddisplay,gdeh0213b1` | SSD1673 | 250 × 120 |

The overlay targets V3/V4, which is what is sold now. The older two are 120 rows
rather than 122, so `height` changes with the compatible.

### Two traps in the driver

In Zephyr 4.4 the SSD16xx driver sits behind MIPI-DBI rather than on SPI
directly: the display node is a child of a `zephyr,mipi-dbi-spi` node that owns
the bus and the D/C and reset pins. Putting those properties on the display node
itself is the older binding and does not bind at all.

Its `full` and `partial` children are not decoration — the driver offers partial
refresh only when a partial profile exists, and picks between them by the
blanking state. So a full refresh is a sandwich: `display_blanking_on()`,
`display_write()`, `display_blanking_off()`, and the update happens on the third
call. A partial refresh is a plain write with blanking already off. Doing only
the first half loads the image and never shows it — the refresh returns in about
20 ms with nothing changed, and the update is deferred onto whichever later call
turns blanking off, which then runs long and shows the previous image. A real
bug, found on the bench: the dummy display accepts blanking calls in any order.

## Powering the rig from the cell

The charger's **3V** goes to the DevKitC's **3V3** pin and its **−** to GND,
bypassing the board's own regulator: the buck already makes 3.3 V at up to 1 A,
and an LDO afterwards would only add a drop. It also leaves the DevKitC's 5 V
rail dead, which is worth having — the CP2102 lives there, so on battery it is
simply unpowered.

**Two supplies, one node.** With the charger on 3V3 and a USB cable in the
DevKitC, two regulators drive the same net and the higher wins. Unplug the JST
before flashing, or fit a slide switch in the battery lead. The switch is worth
ten minutes: the flash-and-monitor loop runs many times a day.

Tapping VU rather than the DevKitC's own VBUS has two consequences, and both
matter. Plugging the devkit in to flash leaves the firmware's VBUS reading low,
so the sync window does not fire on every flash and the LED does not go red every
time you work on the board. It also means a devkit cable feeds the rig invisibly:
any "running on battery" measurement taken with one in is a lie.

### Which cables, for which job

The CP2102 sits on the DevKitC's 5 V rail, so plugging either devkit jack in
powers 3V3 through the board's own LDO. **The normal console and "running on the
cell" are therefore not the same session** — every job below picks one.

| Job | Cell | Charger USB-C | DevKitC jacks | Console |
|---|---|---|---|---|
| Flashing, and everyday logic work | out, or switch open | — | UART, plus USB for the debug build | full |
| Watching a charge run red → green | in | in | out | none; the LEDs are the readout |
| Anything about battery behaviour | in | out | out | none |
| Sleep current | in | out | out | meter in series with the cell |

That is what the two status LEDs are for. The cases with no console are exactly
the ones where the panel cannot say anything either, which is why a refused press
gets three red blinks rather than a log line.

A console *while* genuinely on the cell needs the native jack and a cable whose
VBUS wire is not connected: the USB-Serial-JTAG peripheral is inside the SoC and
runs from the chip's own 3.3 V, so only D+, D− and ground have to arrive.
*verify:* not tried here, and it is the same peripheral the rev A connector rests
on.

## Cables

Keep both USB cables connected while developing. They can share a host.

| Jack | Shows up as | Carries |
|---|---|---|
| UART | `/dev/cu.usbserial-*` | Flashing, and the normal console |
| USB | `/dev/cu.usbmodem*` | JTAG for OpenOCD, and the debug build's console |

The UART jack's CP2102 drives EN and IO0 through the auto-reset circuit, so
anything opening that port resets the chip — which is why the debug build moves
its console to the native USB side, where no such circuit exists.

A third cable matters now: the **charger's** USB-C. That is the one the firmware
sees — plugging the DevKitC in is not plugging the device in. It carries power
only. The 6092 is a charger, and the D+/D− it breaks out go nowhere until
somebody wires them.

Rev A collapses all three into one USB-C on the charger input, with D+ and D− to
GPIO19/20 — see [decisions](decisions.md) for why that needs no CP2102 and what
it obliges instead. *verify:* nothing here has flashed over `/dev/cu.usbmodem*`
yet. Point `ESPTOOL_PORT` at the usbmodem device and run `just fw-flash`; it is
one check, and rev A's connector rests on it. The 6092 breaks D+/D− out on its
bottom edge, so the whole arrangement can be tried on the bench afterwards —
keep that pair under about 10 cm and twisted, and **never plug both connectors
at once**, since they land on the same two pins.

## Bringing the rig up

Six stages, each ending in a number from the meter. Nothing moves on until the
current stage reads what it should. The two irreversible mistakes available are a
reversed cell and 5 V on a 3.3 V pin, and both are caught before they happen.

### 0. The parts, nothing connected

- Continuity across the 6092's charge-rate jumper pads: **open**. Closed means
  the cut did not take and charging runs at 1 A instead of 500 mA.
- Cell leads on DC volts: **3.6–4.0 V**, and note which lead is positive. Compare
  against the polarity printed at the 6092's JST footprint.

**If the polarity disagrees, stop.** Do not re-crimp and do not try it briefly. A
reversed cell damages the charger and can vent the cell.

### 1. Charger and cell, no MCU

- Plug the cell in. Nothing gets warm, and the fault LED marked `F` stays off.
- `3V` to `−`: **3.30 V ±0.05**. This is the rail the DevKitC will run on and the
  most important reading on the page.
- `4.5V` to `−`: **≈ the cell voltage**, not 4.5 V. On battery the power path
  passes the cell through; 4.5 V is a ceiling, not an output.
- Plug the charger's USB-C: `C` lights, `VU` reads **≈5 V**, the cell climbs over
  a few minutes. Unplug again.

### 2. The dividers, still no MCU

Both taps get measured before either touches a pin. This is the stage where a
slip puts 5 V on a 3.3 V input.

- Battery tap to `−`: **half the cell**, 1.85–2.10 V. Above 2.2 V means the
  divider is wrong or the tap is on the wrong node.
- VBUS tap to `−`, USB-C **in**: **2.9–3.1 V**. USB-C **out**: **≈0 V**. Near
  5 V means the divider is not in circuit, and GPIO21 would not survive it.
- Only now run the two taps to GPIO1 and GPIO21.

### 3. The LEDs, still no firmware

Touch each LED's resistor to 3V3 by hand and watch it light: red on GPIO15's leg,
green on GPIO16's. Fix polarity here rather than after flashing.

### 4. The power feed

- DevKitC USB unplugged. `3V` to the 3V3 pin, `−` to GND.
- 3V3 pin to GND: **3.30 V ±0.05**. A sag means the buck is loaded by something
  it should not be.
- Pull the JST for a moment and confirm the rail collapses. That proves the cell
  is the source and not a USB cable somebody forgot about, which is the
  commonest way a "running on battery" test lies.

### 5. Contention, once

Understand this reading rather than avoiding it. With the charger on 3V3, plug a
USB cable into the DevKitC and meter the 3V3 pin — two regulators, one node, the
higher one wins. Note the number, then pick a remedy from "Powering the rig".

### 6. With firmware

- `just fw-power` reports millivolts within about 2 % of a meter on the BAT pad.
  Calibrating the divider constants is bench measurement 5 below.
- Plug and unplug the charger's USB-C and watch the logged VBUS flag follow.
- The LEDs, in one pass: unplugged and healthy is dark, charging is red, past
  `CONFIG_TK_POWER_FULL_MV` is green, the portal is amber, and a press below
  `CONFIG_TK_REFRESH_MIN_MV` is three red blinks with the panel unchanged.

What good looks like: the device runs from the cell with no USB attached, reports
a voltage that tracks a meter, goes red when the charger's USB-C is plugged and
green when the cell fills, refuses to refresh below the floor and says so, and
goes dark two seconds after being unplugged.

## Checking it

```sh
just fw-flash
just fw-monitor
```

- Boot logs `active deck: 0 new_people` and the panel shows that name.
- One Category press names the next deck; six return to where they started.
- One Next press produces exactly one question.
- Holding a button and releasing it counts once, not twice.

All four hold with neither divider fitted, which is the state the image is first
flashed onto — **provided GPIO1 and GPIO21 are jumpered to the − rail**, for the
reasons in "Before either is fitted" above. Left floating, either one can put the
device somewhere it will not draw a card. With both loose and reading high, the
boot log says so: `VBUS is high but the pack reads N mV; check both dividers are
fitted`.

## Bench measurements

Measured on a Waveshare 2.13-inch V4:

| Measurement | Result | Where it went |
|---|---|---|
| Partial refreshes before ghosting | visible at 64 in ordinary use | `TK_FULL_REFRESH_INTERVAL` = 16 |
| Partial refresh duration | 622 ms, ±2 ms across 193 of them | clears the 1 s target in [device prototype](device_prototype.md) |
| Full refresh duration | not measured — see below | `TK_REFRESH_TIMEOUT_MS` stays at 15 s |
| Refresh after a deep-sleep wake | 624 ms, against 2919 ms before the panel patch | [patching zephyr](firmware_patches.md) |
| Peak thread stack | logging 89%, everything else 18–38% | `LOG_PROCESS_THREAD_STACK_SIZE` = 2048 |
| RTC memory across a warm reboot | kept, three times running | the retained block works on the chip |
| Battery reading against a meter | not measured | `TK_POWER_DIVIDER_NUM` / `_DEN` |
| Where a refresh actually corrupts | not measured | `TK_REFRESH_MIN_MV`, still at its guessed 3200 |
| Discharge curve | not measured | nothing yet; the first runtime figure |
| Current: idle, refresh, Wi-Fi, per LED, asleep | not measured | rev A's resistor values, and the sleep budget |

The soak run reached 193 consecutive partials with only minor artefacts, and 64
was three times under that. Ghosting turned out to be obvious well before 64 —
"minor artefacts" is a more forgiving standard than a question somebody is
reading at a table. 193 is the ceiling to stay under, not the target.

Full-refresh duration is unmeasured on purpose. `ssd16xx` waits on BUSY before
each command rather than after, so a full refresh is started and never waited on:
the application times it at 19 ms for something the panel spends roughly two
seconds doing. Measuring it means watching BUSY, not the log.

Each procedure below wants both USB cables in and `just fw-monitor` open.

### 1. Ghosting, for `CONFIG_TK_FULL_REFRESH_INTERVAL`

E-paper leaves a faint trace of what it was showing and a partial refresh does
not clear it. The question is how many partials the panel tolerates before that
residue is visible from across a table.

```sh
just fw-soak
```

The soak image presses Next every five seconds with full refreshes suppressed, so
the chain builds while nobody is at the board, and logs where each one sits:

```
[tk_soak] partial #1 of seq 2
```

Leave it alone. Write down the number on the console at the point the glass stops
looking clean — from a normal seat at the table, not from six inches away. That
number, less a margin, goes in `firmware/Kconfig.policy`. `just fw-flash` returns
the board to the real firmware.

### 2. Refresh duration, for `CONFIG_TK_REFRESH_TIMEOUT_MS`

The same console gives it away, from the line `display.c` logs after every render:

```
[tk_display] partial refresh of question seq 2 took 412 ms (0)
```

The timeout exists only so a dead panel cannot wedge the device, so it should sit
well clear of the slowest honest refresh — a *full* one, which the soak run
produces once at boot. Set the timeout several times that.

The same output carries stack high-water marks once a minute, which is what the
board conf's promise to tighten stacks is waiting on.

### 3. What survives a reboot, with `just fw-retain`

Deep sleep is a reboot, so everything the device remembers between presses lives
in RTC memory. This image is the soak build, warm-rebooting itself every three
presses. Each reboot should print, in order:

```
rst:0xc (RTC_SW_CPU_RST)
<inf> tk_main: reset: software
<inf> tk_app: retained state: kept across the reboot
```

and then continue — same `seq` sequence, same partial count, no full refresh,
nothing drawn until the next scheduled press. A question redrawn at boot, a count
restarting at zero, or `cold boot, starting a fresh cycle` all mean the block did
not survive.

Use the image rather than the reset button: an EN reset reports `rst:0x1
(POWERON)` and clears the RTC domain, which is what a power-on should do.

### 4. Accents, with `just fw-charset`

Not a number, but the same trip to the bench. The charset image draws every
diacritic the renderer composes; photograph the pages and judge whether the marks
sit where they should. Details in `firmware/README.md`.

### 5. The battery reading, for the divider constants

`just fw-power` is the everyday image plus `CONFIG_TK_DEBUG_POWER=y`, and that
one symbol is the whole difference: it samples once a second instead of every
`CONFIG_TK_POWER_SAMPLE_MS`, and logs the raw conversion beside the converted
millivolts. The ordinary image reports the cell too, but only when the state
changes or the reading has moved more than 20 mV, so a resting cell goes quiet —
which is why calibration and the discharge curve want this build and nothing else
does. Put a meter on the BAT pad and compare.

```
[tk_power_adc] raw 2412 -> 1943 mV at the pin
[tk_power] 3886 mV, on the cell
```

A reading off by a constant ratio is the divider: real 470 kΩ resistors are ±1 %
each, which is why `CONFIG_TK_POWER_DIVIDER_NUM` and `_DEN` are two integers
rather than one number. Take readings at both ends of the range — a fixed offset
and a wrong ratio look identical at a single point. A reading that jumps around
is the 100 nF missing, or the divider too stiff for the ADC.

Left running, the same image records a discharge curve unattended, which is the
first runtime figure the project will have. Note wall-clock time against voltage;
the interesting part is below 3.6 V, where the curve turns down.

### 6. Where a refresh corrupts, for `CONFIG_TK_REFRESH_MIN_MV`

3200 mV is a guess and has never been checked. Replace the cell with a bench
supply on the BAT pad and wind it down, pressing Next at each step, watching the
glass rather than the console.

What is wanted is the voltage at which a partial refresh leaves artefacts — not
where the regulator gives up. The buck passes the cell straight through below
about 3.45 V, so the rail sags with the cell and the panel keeps working for a
while after that. Take the first voltage where the glass goes wrong, plus margin.

Below the configured floor the device should log the millivolts, leave the panel
alone and blink red three times — which is also the check that the gate works:

```
[tk_app] 3140 mV is under the 3200 mV floor; the panel keeps what it has
```

### 7. Current, for the sleep budget

A meter in series with the cell, at each of: idle and awake, mid-refresh, with
the radio up, with each LED lit, and asleep.

Only some of those are honest here. **The DevKitC's power LED draws a milliamp or
two continuously from the 3V3 rail, and its onboard WS2812 idles at roughly
another 0.6 mA whether anything drives it or not** — together one to two orders
of magnitude above the 30 µA target, under which the bq25185 and TPS62569
quiescent currents disappear entirely. The sleep figure measured here bounds
nothing about rev A and no runtime can be extrapolated from it.

What the bench does give: the awake numbers, the per-LED current that sets rev
A's series resistors, and a delta per firmware change, which is the useful form
anyway.

Worth writing down once, because it settles where the budget goes: a press is
under a second at tens of milliamps, roughly 0.015 mAh. A hundred presses a day
is about 1.5 mAh against a 1500 mAh cell. Sleep current is the whole design
problem and everything else is rounding.
