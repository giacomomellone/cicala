# cicala docs

An open-source system for conversation questions: a community-maintained [question database](https://github.com/giacomomellone/cicala/tree/main/questions), a free static-first [website](https://cicala.dev) with one submission endpoint, and a pocket-sized e-paper [device](design.md#why-a-device). All three share one database, which lives in the Git repository.

> **Design principle:** minimize time-to-question, maximize time-in-conversation. The product succeeds when people stop looking at it.

## Pages

- **[design](design.md)**: product and UX rationale. Why a device, the interaction surface, website jobs, non-goals.
- **[device prototype](device_prototype.md)**: honest feasibility review, proposed enclosure, and a gated hardware-development plan.
- **[prototype bom](prototype_bom.md)**: bench-prototype shopping list with checked order links for DE/EU.
- **[hardware wiring](hardware_wiring.md)**: the breadboard rig — pin assignment and why each pin, how the buttons, the charger, the two dividers, the status LEDs and the e-paper HAT connect.
- **[firmware primer](firmware_primer.md)**: introduction to the device firmware and to Zephyr as this project uses it — build inputs, devicetree, Kconfig, tests, debugging.
- **[firmware architecture](firmware_architecture.md)**: threads, the messages between them, and which modules depend on Zephyr.
- **[decisions](decisions.md)**: append-only decision log (ADR-lite), including every dependency justification.
- **[contribution pipeline](contribution_pipeline.md)**: how a question gets from a form into the database, the site and a device — the labels, the automatic checks, and which rule is enforced where.
- **[native submission setup](native_submission_setup.md)**: GitHub App, Turnstile, Cloudflare Pages Function, runtime secrets, editorial labels, and verification.
- **[publication policy](publication_policy.md)**: what belongs in this public repository, what belongs in the private sibling, and how to route a change.
- **[languages](languages.md)**: multilingual policy. Independent corpora, the incubator, maintainers.
- **[sync protocol](sync_protocol.md)**: device sync flow and the `.qdb.gz` bundle format, byte by byte.
- **[votes](votes.md)**: the agreed v2 voting design (not built; seams are in place).

## Elsewhere in the repo

- [README](https://github.com/giacomomellone/cicala#readme): project overview and part status.
- [CONTRIBUTING](https://github.com/giacomomellone/cicala/blob/main/CONTRIBUTING.md): how to add a question (30 seconds, no dev setup) and the CC0 dedication.
- Per-language style guides live next to the data: `questions/{lang}/STYLE.md`.

## Working on the docs

```sh
just docs      # live-preview at http://127.0.0.1:8000
just docs-build
```
