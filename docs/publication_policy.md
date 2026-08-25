# Publication policy

This repository is public. Everything committed here is world-readable the
moment it is pushed, and stays readable in forks, clones, and caches after it
is deleted. This page says what belongs here, what does not, and what to do
when the answer is unclear.

## What belongs here

- The question database and its language style guides.
- The website: source, styles, i18n strings, and public assets.
- Firmware sources, tests, and firmware documentation.
- Hardware design sources: schematic, PCB, enclosure structure and contracts.
- Reproducible bills of material, including component price indications that
  let somebody build the device.
- Protocol documentation, the sync and bundle formats, and the OTA path.
- Technical documentation, build instructions, and the decision log.
- The public roadmap, contribution guidance, and the licenses.
- Public-facing project assets that the project is prepared to publish.

## What does not belong here

- Go-to-market plans, launch strategy, and commercial launch thresholds.
- Retail pricing, price positioning, target margins, and internal COGS
  analysis.
- Supplier quotations, supplier contacts, and manufacturing operations.
- Cash planning and unit economics.
- Private brand and naming research.
- Unpublished product concepts, including concept renders and the prompts or
  source material used to generate them.
- Customer and prospect identities.
- Raw user-research data, interview recordings and notes, and participant
  consent records.
- Private compliance work.
- Credentials of any kind: keys, tokens, signing material, and passwords.

Material in those categories belongs in the private sibling repository. That
repository is referenced by name in `AGENTS.md`; do not link to it from public
documentation, and do not describe its contents here.

## Component prices are not commercial strategy

`hardware/pcb/BOM.md` and `docs/prototype_bom.md` carry component price
indications so a maker can reproduce the device and judge what it costs to
build. Those stay public. A price appearing in a document is not by itself a
reason to move it. What moves is negotiated pricing: quotations tied to a
supplier, volume-dependent cost, margin, and anything that reveals commercial
position rather than component cost.

## Renders and hardware claims

Concept renders are unpublished product concepts. They do not belong in this
repository, and they must not be used as evidence that hardware exists.
Hardware documentation here describes what has been measured on a real rig and
what is still a study target, and it says which is which. Photographs may be
published once they show hardware that was actually built.

## Routing a change

Decide which repository owns the material before editing, not after. The two
working trees are independent: run `git status` separately in each, and use
separate branches, commits, validation runs, and pull requests. No branch,
commit, or pull request spans both.

To move material out of this repository, copy it to the private destination
and verify it there first, then remove the public copy in a separate change.
Never copy the other direction without explicit publication approval.

A clean clone of this repository must build and test on its own. Do not add
submodules, symlinks, imports, or documentation links that make it depend on
the private repository.

If the owning repository is unavailable, stop and ask. Do not park private
material here, and do not build a private duplicate of work that already lives
here.

## `.gitignore` is not the boundary

An ignore rule only keeps a file out of `git add`. It does not classify the
file, it does not survive `git add -f`, and it does nothing about material
pasted into a document, a commit message, a test fixture, or a code comment.
Treat the ignore list as ergonomics for build output and local state.

The boundary is enforced by reading the change. Every pull request carries a
checklist item confirming that the diff contains no credentials, supplier
quotations, private commercial strategy, or participant personal data; see
[`.github/pull_request_template.md`](https://github.com/giacomomellone/cicala/blob/main/.github/pull_request_template.md).
Confirm it against the actual diff.

## If something private is already public

Assume it is disclosed. Removing a file in a later commit leaves it in history
and in every existing clone. Rotate any exposed credential first, then remove
the file, then decide with the maintainers whether history rewriting is worth
its cost. Record the outcome so the next person knows whether the history was
rewritten or only the working tree was cleaned.
