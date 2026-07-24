# Language incubator

New languages start here, in `questions/incubator/{lang}/`, using the exact same file structure as a shipped language (`STYLE.md`, `denylist.txt`, one YAML file per category). Incubator languages are validated by CI but **excluded** from website payloads and device bundles.

A language graduates — its directory moves to `questions/{lang}/` — when it has:

1. **≥ 150 questions with ≥ 20 per category**, written natively (not machine-translated; see [docs/languages.md](../../docs/languages.md)).
2. **A named maintainer** listed in `docs/languages.md` who is a fluent speaker and commits to reviewing submissions.
3. Its own **`STYLE.md`** and **`denylist.txt`**.

Never open a PR that adds a top-level `questions/{lang}/` directory directly — graduation is a deliberate, reviewed move. To start a new language, open an issue titled `lang: <code>` and read [docs/languages.md](../../docs/languages.md) first.
