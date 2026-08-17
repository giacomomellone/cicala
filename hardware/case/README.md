# Case — Rev A

`kveld_enclosure.scad` is the canonical Rev A enclosure source. Hardware files use [CERN-OHL-S-2.0](../../LICENSE-HARDWARE).

The model includes the top shell, base, display retainer, two cap geometries, optional steel skin, light pipe, PCB/cell/antenna reference volumes, gasket and drain intent, USB opening, feet, and four fit coupons. The coordinate and validation contract is in [docs/hardware_rev_a.md](../../docs/hardware_rev_a.md).

![Rev A enclosure CAD render](renders/rev_a/assembly.png)

This is a parametric CAD render, not built hardware. The shell and controls use the public warm-paper (`#FAF8F2`) and charcoal (`#1F1F1D`) palette. The dark scene is a neutral contrast background, not an enclosure colour. These colours communicate finish intent; physical colour, texture and gloss remain sample-gated.

## Open the model

The checked tool version is OpenSCAD 2026.06.12. Older releases may open the source, but they are not part of the validation result. Set `part` in the Customizer or on the command line:

```sh
openscad --hardwarnings -D 'part="top_shell"' \
  -o top_shell.3mf hardware/case/kveld_enclosure.scad
```

Selectors are `assembly`, `exploded`, `top_shell`, `base`, `retainer`, `category_cap`, `next_cap`, `lens`, `steel_skin`, `light_pipe`, `pcb_reference`, `coupon_buttons`, `coupon_usb`, `coupon_lens`, and `coupon_boss`.

Regenerate the documentation renders at 3200 × 2200 px with:

```sh
openscad --hardwarnings --render --imgsize=3200,2200 --projection=perspective \
  --camera=42,28,7.92,55,0,25,195 --colorscheme='Tomorrow Night' \
  -D 'part="assembly"' -o assembly.png hardware/case/kveld_enclosure.scad

openscad --hardwarnings --render --imgsize=3200,2200 --projection=perspective \
  --camera=42,28,15,55,0,25,255 --colorscheme='Tomorrow Night' \
  -D 'part="exploded"' -o exploded.png hardware/case/kveld_enclosure.scad
```

The checked STL and 3MF files under `exports/rev_a/` are convenience exports. Regenerate them after changing the source. Do not scale them in a slicer.

## Prototype materials

- Print shell, base, retainer and coupons in PETG or ASA. Avoid PLA for heat, creep and cap-stop decisions.
- Print caps in a tough SLA resin for the first fit study. FDM caps remain a fallback after the bore coupon establishes achievable clearance.
- Cut the lens from 0.8 mm matte hard-coated PMMA only after the lens coupon passes. Keep adhesive outside the visible window and display glass.
- Treat the 1.2 mm steel skin as optional ballast. Its antenna cut-out is mandatory when installed.
- Use a removable cell adhesive and perimeter display support. Neither part may carry button loads.

Default cap heights are 0.2 mm sub-flush for Category and flush for Next. The OpenSCAD values are coupon starting points, not toleranced production fits.

## Checks before a complete print

Run:

```sh
just hw-case-check
```

Then print `coupon_buttons`, `coupon_usb`, `coupon_lens`, and `coupon_boss`. Confirm cap clearance and hard stops, plug shell clearance, light-pipe sealing, lens support, insert or pilot-hole behavior, and print orientation. The full shell remains a fit prototype until it passes those checks with real parts.
