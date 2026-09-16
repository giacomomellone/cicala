# Rev B PCB

Open `cicala_rev_b.kicad_pro` in KiCad 10. The engineering design uses the ESP32-S3 module, an L-shaped 1.2 mm four-layer PCB and one fitted assembly face. Bare diagnostic contacts may be on the underside. Component selection and assembly assumptions are recorded in [the engineering guide](../../../docs/hardware_rev_b.md) and [selection record](../../../docs/hardware_rev_b_selection.md).

The display is Waveshare 22609, 3.52-inch V1.1 UC8253. The Molex 503480-2400 connector uses the panel table: pins 1 and 4 unused, pin 5 decoupled. The protected Renata pack connects through JST SH: BAT+/NTC/GND on pins 1/2/3. R8 sets nominal 100 mA charging.

The board uses 0.2 mm general clearance, 0.5 mm copper-to-edge spacing, 0.15 mm fine-pin escapes and ordinary 0.3 mm drilled / 0.6 mm diameter through-vias. 87 SMT components and two B3F-4050 through-hole switches are installed from the top; switch solder joints are underneath; there are no filled via-in-pad requirements. Ground is poured on both outer faces and In1, with local In1 signal crossings outside reserved reference areas. In2 carries power/signals and a dedicated ground reference beneath the bottom USB pair.

Run `just hw-rev-b-check` for native electrical and mechanical checks. `just hw-rev-b-fab-export` exports only after those checks pass, including assembly BOM/CPL, Gerbers, drills, IPC-2581 and STEP. The factory must confirm the stack, controlled impedance, panel support, component orientations and paste treatment before an engineering order. This is an unbuilt prototype; see [validation](../validation.md) for actual check results and remaining measurements.

[`renders/`](renders/README.md) contains top and bottom KiCad 3D exports used by
the public device page. The mechanical board model is
`exports/cicala_rev_b_board.step`.

[Reading the board](silkscreen.md) explains its native silkscreen: Cicala
branding, circuit brackets, connector polarity, every test point, and the
underside power/recovery guide. Main text is 1 mm, compact labels are 0.8 mm,
with 0.15 mm minimum strokes and silk clearance. The render directory also
contains both silkscreen drawings as SVGs.

The switch hole pattern uses 1.2 mm finished PTH and 1.8 mm NPTH holes. The JLC BOM/CPL covers 87 SMT references; `manual_assembly.csv` specifies the separate switch soldering and purchased-cap installation. Caps are fitted only after soldering.

The [fabrication notes](fabrication_notes.md) specify stack, impedance, panel, tighter switch-hole tolerances, assembly stages and inspection. They are included in the exported prototype package. SW1/Filters is the upper control; SW2/Next is the lower control.
