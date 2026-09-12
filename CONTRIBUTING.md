# Contributing

## Adding a question (no developer setup needed)

This is the contribution that matters most. Two ways, both take about 30 seconds:

1. **Website:** [suggest a question](https://cicala.dev/suggest) without an account. Enter the question and language, optionally add a credit, and confirm the public-domain dedication. A maintainer assigns depth and tags.
2. **GitHub:** open a [new-question issue](../../issues/new?template=new-question.yml) directly.

The website submits through the repository's narrowly scoped GitHub App, so the resulting issue is opened by `cicala-bot` without exposing a contributor account. Direct GitHub submissions require a free GitHub account and ask the contributor to choose editorial metadata. Within a minute of opening either kind of issue, an automated check covers the applicable text, tone, denylist, and duplicate rules. A maintainer who speaks the language reviews every submission; once approved, another workflow turns the issue into a pull request. The question ships to the website within minutes of merge and to devices with the next database release. [docs/contribution_pipeline.md](docs/contribution_pipeline.md) describes the path.

### The public domain dedication (please read)

By submitting a question you **dedicate it to the public domain under [CC0-1.0](LICENSE-QUESTIONS)**. There is no attribution requirement and no take-backs: your question may be reused anywhere, by anyone, for any purpose, including commercial products and training data. The optional `author` field is a courtesy credit and carries no legal claim. The submission form makes you confirm this explicitly; submissions without that confirmation are not merged.

### What makes a good question

- Standalone and answerable without a prescribed relationship.
- Give the asker and every participant a way to answer.
- Open-ended, never yes/no.
- Specific enough to start a story ("When did you last…" beats "What do you think about…").
- 10–140 characters, ends with `?`, one question per entry.
- Assign depth 1–3 and precise form tags. Mark dark or sexual content explicitly;
  depth 3 is controlled by Heavy. Retired `spicy` metadata needs human review.
- Read your language's style guide: [`questions/en/STYLE.md`](questions/en/STYLE.md), [`questions/de/STYLE.md`](questions/de/STYLE.md).

### Adding a new language

Languages are independent editorial corpora; reviewed machine translations are
optional and never create a completeness requirement (see
[docs/languages.md](docs/languages.md)). New languages start in
`questions/incubator/{lang}/` and graduate once they have ≥ 150 questions with a varied default pool, a named fluent maintainer, a `STYLE.md`, and a
`denylist.txt`. Never open a PR adding a top-level `questions/{lang}/` directory
directly.

## Editing an existing question

Open the question's permalink and select **suggest an edit**. Like the
suggestion form, it needs no GitHub account: the site opens the
`question-edit` issue for you. The
[question-edit form](../../issues/new?template=edit-question.yml) on GitHub
stays available, and is the better route when you want to follow the
discussion yourself — an issue opened by the bot cannot be edited by the
person who asked for it.

Propose wording in your own words, or leave the wording empty and tick the
metadata you want reconsidered: depth, tags, or translation
provenance. Either way an explanation is required. Proposed wording must be
written by a person and dedicated to CC0. A fluent maintainer applies accepted
wording or metadata changes in a normal pull request while preserving the
question's `id` and `added` values.

## Editing question files directly (developers)

One corpus file per language: `questions/{lang}/questions.yaml`. Store a
question once, with its depth and precise form/content tags.

### Writing a batch

A draft file states the editorial metadata once per group and lists the
questions under it, which is the cheaper way to draft several questions in one sitting:

```text
# depth: 1 · tags: icebreaker
What's the most spontaneous thing you've ever done?
Which song gets you on the dance floor every single time?

# depth: 2
What did you believe about your parents that turned out to be wrong?
```

```sh
just draft en drafts/en.txt              # append them to questions/en/
just draft en drafts/en.txt --dry-run    # show what it would append, write nothing
```

Fields are separated by `·` or `|`. A header replaces the previous one outright,
so a group with no `tags` field has no tags. A `#` line that does not open with
`depth:`, `tags:` or `author:` is a comment. A question may wrap over
several lines and closes on the line that ends with `?`.

The import applies the validator's own rules before it writes anything: a draft
that breaks one leaves the corpus untouched and names the line to fix. Questions
already in the corpus are reported and skipped, so re-running a draft you have
edited in part is safe. It finishes by running the fix pass, so ids and dates
are already assigned when it returns.

`drafts/` is gitignored. The draft is your working copy; the corpus is the
database.

### Writing one

Append the entry **without an `id` and without `added`**; CI assigns both:

```yaml
- text: "When did you last change your mind about something important?"
  depth: 2
  tags: [reflective]
  author: "your name" # optional
```

Then run the validator before pushing (`just setup` once, if you haven't):

```sh
just fix        # assigns ids/dates, normalizes formatting
just validate   # must pass clean
```

Rules the validator enforces: schema conformance, 10–140 chars ending in `?`,
no duplicates within a language, depth 1–3,
explicit dark/sexual tags, per-language denylist, controlled tags, and origin
references. Never hand-write or edit an `id`. Once assigned, IDs stay stable
through text and depth edits.

To edit an existing question, change its text or editorial metadata in a normal
pull request and keep its `id` and `added` values. Only a person may make that
edit. Once it merges, CI refreshes the other linked translations in review pull
requests, including translations previously written by people. An edit to a
translation never rewrites its human-written original. Keep `origin`,
`translated_by`, and `translation_sync` intact: the latter records generated
updates so their merges do not trigger another translation round. A fluent
maintainer reviews each update before merge. If one push edits multiple versions
of the same question, select the intended source using the workflow's manual
run inputs; see [Languages](docs/languages.md#machine-translation-reviewed-synchronization).

## Website development

```sh
just website        # dev server (regenerates website/src/data/*.json first)
just test-website   # vitest suite
just test-e2e       # playwright: builds the site and drives it in a browser
```

Ground rules (from [docs/design.md](docs/design.md)): no UI frameworks, no Tailwind, no analytics, no accounts. The play and browse surfaces load no third-party scripts; the suggestion page loads Cloudflare Turnstile for abuse prevention. Performance budget: ≤ 60 KB gzipped JS per page, Lighthouse mobile ≥ 95 on the play page. Every new dependency needs a one-line justification in [docs/decisions.md](docs/decisions.md).

## Firmware / hardware

Structure and interface docs live in [`firmware/`](firmware/) and [`hardware/`](hardware/); both are early-stage. Firmware targets ESP-IDF v5.3+ on ESP32-S3. Hardware is licensed CERN-OHL-S-2.0.

## Commit style

Conventional commits, small PR-sized changes: `feat(site): …`, `feat(questions): …`, `fix(tools): …`, `chore(ci): …`, `docs: …`.

The format is enforced locally by a zero-dependency `commit-msg` hook in [`.githooks/`](.githooks/). Enable it with `just hooks` (also part of `just setup`); it runs `git config core.hooksPath .githooks`. Allowed types: `feat fix docs test chore refactor ci build perf style revert`; header ≤ 72 chars; scope optional and lowercase.

## Security

Do not open a public issue or pull request for anything security-sensitive — in the website, tools, firmware, hardware, or the release process. Report it privately instead: [SECURITY.md](SECURITY.md).

## Code of conduct

Everyone interacting in this project is expected to follow the [Code of Conduct](CODE_OF_CONDUCT.md).
