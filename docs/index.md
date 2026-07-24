# tischkarte docs

An open-source system for conversation questions: a community-maintained [question database](https://github.com/tischkarte/tischkarte/tree/main/questions), a free static [website](https://tischkarte.pages.dev), and a pocket-sized e-paper [device](design.md#why-a-device-at-all) — all sharing one database, which lives in the Git repository.

> **Design principle:** minimize time-to-question, maximize time-in-conversation. The product succeeds when people stop looking at it.

## Pages

- **[design](design.md)** — product and UX rationale: why a device, the interaction surface, website jobs, non-goals.
- **[device prototype](device_prototype.md)** — honest feasibility review, proposed enclosure, concept renders, and a gated hardware-development plan.
- **[decisions](decisions.md)** — append-only decision log (ADR-lite), including every dependency justification.
- **[languages](languages.md)** — multilingual policy: independent corpora, the incubator, maintainers.
- **[sync protocol](sync_protocol.md)** — device sync flow and the `.tkb` bundle format, byte by byte.
- **[votes](votes.md)** — the agreed v2 voting design (not built; seams are in place).

## Elsewhere in the repo

- [README](https://github.com/tischkarte/tischkarte#readme) — project overview and part status.
- [CONTRIBUTING](https://github.com/tischkarte/tischkarte/blob/main/CONTRIBUTING.md) — how to add a question (30 seconds, no dev setup) and the CC0 dedication.
- Per-language style guides live next to the data: `questions/{lang}/STYLE.md`.

## Working on the docs

```sh
just docs      # live-preview at http://127.0.0.1:8000
just docs-build
```
