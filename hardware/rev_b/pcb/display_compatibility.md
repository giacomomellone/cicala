# Display compatibility work after the fit study

The current firmware supports the Rev A panel. The new display is **not compatible by changing only the enclosure or bundle contents**. This milestone changes no firmware.

## New target contract

The candidate active matrix is 360 × 240 pixels in landscape. A packed monochrome framebuffer has 10,800 bytes before driver-specific alignment or secondary buffers. Confirm the controller, SPI protocol, waveform tables and power sequence against the exact supplied panel revision.

The V1.1 pin table calls BUSY_N active low while busy. The driver and GPIO flags must match the actual device. Do not copy the old panel's busy interpretation. The visible active area is centered mechanically, but the flex exits right; establish the software rotation with a labelled corner/checkerboard pattern on hardware.

The retained host pins are EPD_PWR_EN GPIO5, RESET GPIO8, BUSY GPIO9, CS GPIO10, MOSI GPIO11, CLK GPIO12 and DC GPIO18. Treat the copied `pin_contract.csv` as the host wiring proposal, not proof that the new driver can run. Park host outputs safely when panel power is off and verify the power switch, bleed path, boost voltages and leakage on the new panel.

## Before a firmware target

Resolve the vendor pin-table/application-circuit discrepancy, select the boost components, fit the FPC without bending its stiffener and confirm connector contact side. Then measure full refresh, usable partial-refresh behavior (if any), temperature effects, ghosting, wake latency and the complete power-off state. Revisit text wrapping, glyph size and question presentation for the larger matrix without changing the question corpus.

Signed question bundles remain an application-data format. A panel change does not make a competitor's firmware accept those bundles, nor does it make a Cicala firmware image compatible with a different bootloader. A future donor-device port needs its own storage/install adapter, signature verification, driver mapping, partition budget and recovery/rollback validation.

Sources: [Waveshare candidate](https://www.waveshare.com/product/displays/e-paper/3.52inch-e-paper.htm) and [V1.1 documentation](https://files.waveshare.com/wiki/3.52inch%20e-Paper%20HAT/3.52inch%20e-Paper%20V1.1.pdf). See [the engineering contract](../../../docs/hardware_rev_b.md) for the current unresolved gates.
