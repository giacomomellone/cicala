# Rev B PCB 3D renders

`top.png` and `bottom.png` are straight orthographic exports from native KiCad
10.0.5, with zero rotation and matching scale on both sides. They show the
routed, labelled board with its installed
KiCad models and local component envelopes. Some local models describe only
the component's outside dimensions. Colors, pin markings and surfaces are
illustrative; these are not photographs of an assembled board.

`silkscreen_top.svg` and `silkscreen_bottom.svg` are native KiCad plots of the
silkscreen, solder openings and board edge. The bottom plot is mirrored for
reading from below. Use these scalable drawings to inspect small legends.

The device page imports the PNGs directly; Astro generates responsive WebP
variants and links to each full-size PNG and SVG. Hardware-derived images use
[CERN-OHL-S-2.0](../../../../LICENSE-HARDWARE).

From the repository root, with KiCad and its model libraries installed:

```sh
just hw-rev-b-pcb-renders
```

The exporter requests 2400 × 1560 pixels, `--rotate 0,0,0` and `--zoom 1.4`.
The basic raytracing preset avoids offset shadows on the transparent background.
It records the actual pixel dimensions, projection, source and output hashes
in `provenance.json`. Regenerate all four files whenever the board or its models
change. No perspective transform or CSS rotation is applied by the website.

The exporter uses temporary KiCad viewer settings that show both SMT and
through-hole models. Local viewer preferences cannot hide the two switches.
The PCB views show switches without the separately fitted purchased caps;
the enclosure renders show ivory Filters and larger orange Next caps.
