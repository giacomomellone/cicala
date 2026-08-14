# Languages

## The model: independent corpora, not translations

**There is no canonical language and no translation-completeness goal.** Every language is its own corpus of natively written questions. Rationale:

- A master/translation model creates an N×M sync debt: every new English question becomes N "missing translation" tasks, forever.
- Translations drift when originals are edited; independent corpora mean nothing needs syncing, so nothing can drift.
- Conversation questions are register-sensitive (du/Sie, tu/vous) and culturally loaded; translation produces stilted prompts. A smaller native corpus beats a large stilted one for this product.

The optional `origin` field on a question declares "this is an adaptation of that question in another language." It provides lineage and enables coverage views ("translate-me" lists for contributors) without any coupling. It is never required, and a removed origin only produces a validator warning.

**The scaling constraint is moderation, not storage.** For reference: 2,000 questions are about 160 KB of YAML, 50 KB gzipped, so twenty languages fit in a few MB. What does not scale for free is fluent human review of every submission, which is why everything below revolves around named maintainers.

## Incubator rule

New languages start in `questions/incubator/{lang}/` with the same file structure as a shipped language. Incubator languages are validated by CI but **excluded from site payloads and device bundles**.

A language **graduates** (its directory moves to `questions/{lang}/` in a reviewed PR) when it has:

1. **≥ 150 unique questions, with ≥ 20 eligible for each deck.**
2. **A named maintainer** listed below: a fluent speaker who commits to reviewing submissions in that language.
3. Its own **`STYLE.md`** (tone and register decisions; German uses informal "du", for example) and **`denylist.txt`**.

A shipped language whose maintainer steps down gets a `maintainer-wanted` notice here, never removal.

## Machine translation: drafts only

LLM-translated batches may be submitted as PRs tagged `needs-native-review`. **Nothing merges without approval from a fluent speaker.** Machine output is a scaffold for a native rewrite, not a shortcut past one.

## Per-language moderation

Submissions (GitHub issue → `promote-question.yml` → PR) are routed to the language's maintainer via `CODEOWNERS` entries on `questions/{lang}/`. The per-language `denylist.txt` is a CI tripwire for obvious slurs and explicit content; a hit means a human looks, nothing more clever than that.

## Shipped languages

| Language | Code | Maintainer                                          | Status                                                     |
| -------- | ---- | --------------------------------------------------- | ---------------------------------------------------------- |
| English  | `en` | _TODO: replace with the repo owner's GitHub handle_ | shipped (240 seed questions)                               |
| Deutsch  | `de` | _TODO: replace with the repo owner's GitHub handle_ | shipped (99 seed questions; multilingual interaction test) |

## Incubator

_None yet. To start one, open an issue titled `lang: <code>` and you'll get the same structure scaffolded, plus a `lang:{code}` label for submissions._
