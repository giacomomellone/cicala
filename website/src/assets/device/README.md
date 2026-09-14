# Device page PCB renders

`pcb-top.webp` and `pcb-bottom.webp` were rendered on 14 September 2026 from
`hardware/pcb/cicala_rev_a/cicala_rev_a.kicad_pcb` at commit `c46fb6e` using
KiCad 10 and its installed component models, including the project's local
component envelopes. They depict the prototype design; they do not establish
assembled fit or electrical operation. Hardware-derived images use
[CERN-OHL-S-2.0](../../../../LICENSE-HARDWARE).

From the repository root, render each side with `kicad-cli` on PATH:

```sh
kicad-cli pcb render --output /tmp/cicala-device-pcb-top.png \
  --width 1600 --height 1100 --side top --rotate '25,0,-20' \
  --background transparent --quality high \
  hardware/pcb/cicala_rev_a/cicala_rev_a.kicad_pcb
kicad-cli pcb render --output /tmp/cicala-device-pcb-bottom.png \
  --width 1600 --height 1100 --side bottom --rotate '25,0,-20' \
  --background transparent --quality high \
  hardware/pcb/cicala_rev_a/cicala_rev_a.kicad_pcb
```

Encode the PNGs as WebP at quality 92 with Sharp. The device page imports the
enclosure PNGs directly from `hardware/case/renders/rev_a/`; Astro produces
responsive WebP variants for all four images during the website build.

When the hardware changes, regenerate the affected views and review the
dated status text in `website/src/i18n.ts` against `docs/hardware_rev_a.md`
and `docs/firmware_rev_a.md`.
