# Rev A layout review

This is an engineering checkpoint, not a production release. The working
`.kicad_pcb` contains the reviewed candidate; intermediate router files are
not an alternative source of truth. No fabrication package has been issued.

## Placement and appearance

The board has 109 footprints, including four mounting holes and 22 test pads.
Independent courtyard checks find no overlaps, no underside courtyards in
the cell reserve and no unexpected front-side components. The central
36 × 30 mm reserve occupies 28.3% of the rectangular board area. Parts cannot
be packed into it without changing the reviewed battery/enclosure contract.

`tools/refine_placement.py` records the component moves and changed anchors;
`tools/README.md` explains their electrical reasons. The charger, buck-boost,
panel supply, USB interface and module remain separate blocks. Aligned rows
do not complete decoupling-distance, switching-loop or return-current review.

The underside silkscreen uses the public lowercase Literata wordmark as
native polygons, not a raster image or a replacement logo. It includes Rev
A0, the cell reserve, regulator functions, USB input, J3 battery polarity/NTC
and the J4 recovery pinout. Text is at least 0.8 mm high with 0.15 mm strokes.
Passive designators remain on the fab layers for the assembly drawing.
The wordmark and central service legend are hidden by the installed cell.
Fab references use 0.6 mm text with 0.1 mm strokes so they fit the small
component bodies; duplicate reference placeholders were removed. A combined,
enlarged underside assembly review was generated and visually inspected.

Six assembled component bodies are absent from the installed 3D view:

| References | Reason |
| --- | --- |
| J1, J3 | Footprints reference STEP files absent from the installed KiCad model library |
| U3, L1, L2, D4 | Custom footprints have no assigned 3D models |

Their pads and footprints are present. Test points, mounting holes and the
Tag-Connect landing pattern intentionally have no soldered component body.
Do not substitute a similarly shaped connector model for mechanical approval.
JST's CAD download requires registration and prohibits redistribution; it has
not been imported into this public repository. Manufacturer/redistributable
models and their transforms remain a release gate.

## Electrical checkpoint

The working board is connected, with zero unconnected items and zero
schematic-parity errors. The exact current DRC counts are in `README.md`.
Copper islands are retained; no clearance or thermal errors are waived.

The U3 switch nets and feedback network, and the panel switch node, have
explicit B.Cu routes without vias. Charger pad escapes, local capacitors and
the USB channel are reserved before maze routing. Ordinary signal routes
still need a bend, branch, width and via-count review. The model's zero
non-45-degree segments does not prove absence of right-angle junctions or
acute branch angles.

The charger and cell connector anchors remain about 26 mm apart. Their
grouping still needs the requested electrical-layout review; a clear cell
reserve and aligned capacitors do not establish a tight charger/connector
loop. EPD_SW has 21.095 mm of copper and USB_VBUS has 140.539 mm, including
branches. These totals are not individual loop lengths; they identify routes
that still need shortening and current-path review. Some low-current power
taps use 0.2 mm traces; the requested netclass-width policy remains unfinished.

SW1's crowded ground thermal now has diagonal spokes with the original
0.2 mm gap and width. U5 uses a 1.275 mm, 0.2 mm-wide connection to a dedicated
ground via in place of an automatic pour thermal. All pads remain connected;
there are no starved-thermal or dangling-copper reports.

The USB long channels each have a nominal 58.493813 mm planar main path,
excluding package/resistor interiors and shunt-capacitor branches. Each has
two F.Cu-to-B.Cu transitions: 2.330 mm additional barrel length on the
declared stack. Connector orientation branches each total 5.138528 mm;
the D+ crossover's two B.Cu-to-In2.Cu transitions contribute 0.430 mm and
are compensated in its planar length. Inline TP12/TP13 have no branch stub.
These are geometric lengths, not a field-solver propagation-delay result.

The filled In1.Cu ground has one polygon; neither coupled USB centreline has
an uncovered section. This does not yet prove every regulator's high-frequency
return loop. Two off-grid stitches close the largest gaps near J4 and the
right edge. A 0.5 mm sampling audit now finds a maximum 5.802 mm distance from
ground copper to the nearest ground via/PTH near (29.3,20.3). A 4 mm coverage
target is not met there; no legal additional site was found within 2 mm
without moving signal copper or putting a via in a pad. The nominal stitching
grid has 4 mm pitch where legal.

The TC2030 contact field now has a B.Cu track/via/pour keepout and a custom
0.508 mm contact-to-foreign-copper rule. Four service signals were escaped
outward and reconnected. The contact pads have no solder-paste apertures;
they must be marked DNL in assembly procurement data.

## Open checks, assumptions and risks

- The 58 retained copper islands still block DRC; ground-stitch coverage also
  needs further work. Thermals and dangling copper are now clear.
  IPC-2152 current/temperature sizing and full switching-loop, feedback,
  decoupling and via-count review are not complete. Narrow power escapes are
  not qualified merely because the global power tracks are 0.4 mm wide.
- The GDEY0213B74 flex exit and fold direction have no verified mechanical
  drawing. J1's assumed 3.3 mm body height remains unconfirmed. D4's emitting
  face is assumed to be its 2.0 × 1.0 mm face. Missing component models prevent
  a complete assembled collision check.
- The USB geometry assumes 0.18 mm prepreg at er=4.5. The fabricator's real
  stackup and impedance process must confirm or revise the 90 ohm geometry.
- WROOM-1 sits over FR-4 instead of overhanging the board edge. Assembled radio
  performance is unmeasured despite the copper antenna keepout.
- Whole-device sleep current against 30 µA, e-paper refresh brownout margin,
  charging temperature and LED leakage require hardware measurements.
- U5 has no dedicated input capacitor. Its switched 1.47 MΩ divider draws
  about 3 µA; this remains a deviation from the typical application circuit.
- The exact protected pack, PCM, NTC, keyed wire order, wire exit, adhesive
  and swelling allowance must be checked against the 36 × 30 × 5.4 mm reserve.
- Assembly requires tilting the board into the shell because the USB nose
  enters the wall aperture. Connector retention, service-tool access and
  mechanical tolerances remain physical validation gates.
- Released assembly drawings, complete pad/polarity review, paste/drill/outline inspection
  and Gerber round-trip review have not been completed. No fab export has run.
- Firmware Rev A board configuration, its ztests, decision-log additions and
  the stale ESP-IDF sentence in CONTRIBUTING remain outside this PCB change.

## Verification at this checkpoint

`just test` passed 84 Python tests and 101 website tests. The PCB-tool suite
passed 21 tests. `just hw-case-check` passed all 17 selectors and geometry
assertions. Schematic and enclosure sources are unchanged. `just hw-pcb-check`
still fails on retained copper islands; ERC and schematic parity are zero.
The independent pad/track/via audit finds 0.200 mm minimum foreign-copper
clearance, no edge conflicts and no vias in pads. STEP/fabrication export and
Gerber-viewer inspection have not passed the release gate.

## Sources

Layout requirements: [TI BQ25185](https://www.ti.com/lit/ds/symlink/bq25185.pdf),
[TI TPS63802](https://www.ti.com/lit/ds/symlink/tps63802.pdf),
[Espressif ESP32-S3 PCB guidelines](https://docs.espressif.com/projects/esp-hardware-design-guidelines/en/latest/esp32s3/pcb-layout-design.html),
and [Tag-Connect TC2030 revision B](https://www.tag-connect.com/wp-content/uploads/bsk-pdf-manager/2019/12/TC2030-IDC-NL-Datasheet-Rev-B.pdf).
JST CAD restrictions are on its
[S3B-PH-SM4-TB download page](https://www.jst-mfg.com/product/index.php?doc=2&filename=S3B-PH-SM4-TB.zip&series=153&type=10).
