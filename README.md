# cicala

Cicala is the Italian word for cicada, the loud, mildly annoying voice of every Italian summer.
What do cicadas talk about all that time? They must be asking each other quite interesting questions.
`cicala` is an open source questions and conversation project built to keep the words going, and to stimulate introspection and discovery.

Its three parts share one database:

1. **[Question database](questions/)**: community-maintained YAML files in this repo.
2. **[Website](website/)**: a static front-end to the database, no account needed. Anyone can play, save favorites, and suggest questions.
3. **[Device](hardware/)**: a pocket-sized offline object with one e-paper display and adjacent Filters and Next buttons. The first prototype is still in development.

## AI policy

Cicala's questions are [**Human Reserved**](https://www.gatesnotes.com/home/home-page-topic/reader/a-turbulent-ai-era-and-critical-choices-to-make).
Do not use generative AI to write or edit questions.
The questions in this project are meant for humans, not for machines (unless machines start having consciousness and enjoy asking each other questions).
For everything else in this repository, like code, documentation, tests, tooling, firmware, and hardware, use AI as much as you like.

**Just make sure the questions are meant for people.**

AI may translate an accepted human-written question into another supported language and may suggest metadata such as tags. After a new original question or a human edit in any language is merged, the Google Cloud Translation workflow opens or updates review pull requests for the other linked translations. Editing a translation never rewrites its human-written original. Every machine-translated entry retains that original's `origin` and keeps `translated_by: google`, including after review. `translation_sync` records the version used for each update and prevents generated updates from triggering another round.

Machine translations do not merge automatically. A maintainer compares each draft with the accepted source edit and its original, and may edit or reject it to preserve the intended meaning and make it read naturally in the target language. Published machine translations remain labeled `translated_by: google` in the database, and the website marks them as automatically translated and human-reviewed wherever they appear, with a link to the original.

### Some tips to write a good question

More on the reasoning behind these points: [docs/theory.md](docs/theory.md). Every example below is quoted from that page.

- **Make people build an answer instead of repeating one.** If the answer is already finished in their head, the question is boring. Try adding a constraint.
  - Weak: "What food do you dislike?"
  - Better: "Which food do you wish you liked, and what keeps stopping you?"
- **Avoid rankings and superlatives: best, favourite, top, most.** They ask a person to scan a whole life, pick a winner, and defend that taste in public. Someone who cannot think of an answer feels boring, instead of blaming the question. Ask only what you would answer yourself, at that table.
  - Weak: "What's the best book you've ever read?" A memory test and a public verdict on taste.
  - Better: "Which book do you still think about at odd moments?" It asks what you noticed, not what is best.
- **Ask about one concrete thing, not a whole subject or a principle.** A subject gives attention nowhere to land, a principle has a socially correct answer, and a scene gives the table people, places, and choices to ask about next.
  - Weak: "Do you value honesty?"
  - Better: "When did you last tell a lie you're still unsure about?"
  - Weak: "What do you think about music?"
  - Better: "Which song do you love for a reason that has nothing to do with the music?"
- **Do not invent someone's life.** Keep the assumption that helps and remove the details you cannot know about their family, work, health, or history.
  - Weak: "Which parent understands you better?" It assumes two parents, a relationship with both, and a ranking.
  - Better: "Who understood you better than you expected?"
- **Give the question one job, and keep it short.** Leave the follow-up to the people at the table.

## Contributing a question

You don't need to be a developer or have an account. Use [suggest a question](https://cicala.dev/suggest), or open a [new-question issue](../../issues/new?template=new-question.yml) directly on GitHub. A maintainer who speaks your language reviews it, and once merged it ships to the site and every device. All questions are dedicated to the public domain (CC0); see [CONTRIBUTING.md](CONTRIBUTING.md).

To correct an existing question, use **suggest an edit** on its permalink. That
form needs no account either; the
[question-edit form](../../issues/new?template=edit-question.yml) on GitHub
stays available. Question rewrites remain Human Reserved; accepted edits keep
the existing ID.

Want to add a whole language? Read [docs/languages.md](docs/languages.md).

## Development

[`just`](https://github.com/casey/just) is the entry point. Type `just` to see every command:

```sh
just setup     # one-time: python venv + npm install
just validate  # check the question database
just website   # run the site locally
just test      # database validation + tools tests + website tests
just docs      # live-preview the docs (mkdocs)
```

Requires `just`, Python 3.11+ and Node 20+. `just setup` creates the venv (`.venv/`) from [requirements.txt](requirements.txt).

On a machine with [Nix](https://nixos.org), `nix develop` (or `direnv allow`, using the committed `.envrc`) provides those tools at the versions CI uses, without installing anything system-wide. It is optional and no recipe depends on it; `flake.nix` says what it deliberately leaves to upstream installers, the Zephyr SDK included.

## Documentation

- [docs/design.md](docs/design.md): product and UX rationale
- [docs/brand.md](docs/brand.md): public wordmark, typography, colour, and asset rules
- [docs/device_prototype.md](docs/device_prototype.md): device feasibility review, enclosure study targets, and prototype plan
- [docs/prototype_bom.md](docs/prototype_bom.md): breadboard and rev A parts list
- [docs/decisions.md](docs/decisions.md): decision log
- [docs/languages.md](docs/languages.md): multilingual policy and maintainers
- [docs/sync_protocol.md](docs/sync_protocol.md): device sync and bundle format
