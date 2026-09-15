# Rev B PCB 3D renders

`top.png` and `bottom.png` are 1768 × 1176 exports from native KiCad 10.0.5,
generated on 15 September 2026. They show the routed board with its installed
KiCad models and local component envelopes. Some local models describe only
the component's outside dimensions. Colors, pin markings and surfaces are
illustrative; these are not photographs of an assembled board.

The device page imports these PNGs directly; Astro generates responsive WebP
variants and links to each full-size PNG. Hardware-derived images use
[CERN-OHL-S-2.0](../../../../LICENSE-HARDWARE).

From the repository root, with KiCad and its model libraries installed (the
requested viewport is 1800 × 1200; this renderer produced 1768 × 1176 PNGs):

```sh
kicad-cli pcb render --output hardware/rev_b/pcb/renders/top.png \
  --width 1800 --height 1200 --side top --rotate '25,0,-20' \
  --background transparent --quality high \
  hardware/rev_b/pcb/cicala_rev_b.kicad_pcb
kicad-cli pcb render --output hardware/rev_b/pcb/renders/bottom.png \
  --width 1800 --height 1200 --side bottom --rotate '25,0,-20' \
  --background transparent --quality high \
  hardware/rev_b/pcb/cicala_rev_b.kicad_pcb
```

`provenance.json` records source and output hashes for this export. Regenerate
both images and update the provenance whenever the board or its models change.
