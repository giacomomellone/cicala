# Design

Product and interaction rationale. [Theory](theory.md) governs question writing.

## The principle

**Minimize time-to-question, maximize time-in-conversation.** Start with a question.
The originality belongs in the writing and its variety, rather than a taxonomy
that people must learn before talking.

## One mixed stream

Questions have no relationship categories. Each question is stored once per
language with editorial depth and optional tags. There is no topic taxonomy,
new emotional-weight scale, intensity ladder, or inferred readiness.

Depth means the existing disclosure cost:

1. Little public exposure.
2. A personal construction.
3. Vulnerability, conflict, fear, loss, or consequential disclosure.

Forms describe how a question invites an answer: `icebreaker`, `reflective`,
`hypothetical`, `memory`, and `wouldyourather`. Form tags guide variety and remain
available in Browse. They are not modes of play.

The ordinary pool includes depths 1 and 2, with Dark and Sexual excluded.
Thoughtful, gentle questions can belong here. Editorial review must ensure that
a standalone question does not depend on everyone sharing a particular
relationship. Existing wording and IDs remain human-owned.

## Optional permissions

| Filters row | What enabling it permits                                                        |
| ----------- | ------------------------------------------------------------------------------- |
| Dark        | Questions tagged `dark`: macabre material, morbid scenarios, disturbing imagery |
| Sexual      | Questions tagged `sexual`: sexual experiences, desire, explicit material        |
| Heavy       | Depth 3: vulnerable or consequential disclosure                                 |
| Done        | Apply the draft and resume play                                                 |

These are independent permissions. A dark, sexual depth-3 question needs all
three. Enabling a permission adds eligible questions to the mixed pool. It does
not select a separate channel. Absurdity, playfulness, ordinary disagreement,
and emotional depth alone do not imply sexual content.

The old `spicy` tag is retired. It was broader than Sexual, so pending submissions
with that tag need explicit human reclassification. It must never be silently
converted. Production question texts and IDs are unchanged by this migration.

## Two-button interaction

The small physical button is **Filters**; the large one is **Next**. The phone
and web player use the same controls and state transitions. On web and phone,
each filter row is also a clickable toggle, and Done can be tapped directly.
Tapping a row selects it for subsequent Filters / Next input. These edits use
the same draft and apply only at Done.

| Input                          | During play                      | In Filters                                    |
| ------------------------------ | -------------------------------- | --------------------------------------------- |
| Filters                        | Open the menu with Dark selected | Move to the next row; Done wraps to Dark      |
| Next                           | Draw a permitted question        | Toggle the selected permission, or apply Done |
| Hold a button                  | One action on release            | One action on release                         |
| Both buttons held through boot | Open service setup               | Existing service gesture                      |

All four menu rows are visible. The e-paper explains “Filters: move” and
“Next: change / done”; the cap labels stay fixed. Changes remain a draft until
Done. Done resumes the same question if it is still permitted, otherwise it
selects an eligible replacement. An empty pool shows an empty state with Filters
still reachable. Question-specific save, share, edit, and provenance controls
are hidden while the menu is open.

During play, a compact summary shows each permission as included or excluded.
A discarded press changes no state. Firmware stages the question, bag, draft,
and permissions, committing only after a successful matching display result.
A failed queue, denied refresh, error, or timeout does not silently consume a
question or apply a permission.

## Fresh starts and sleep

All permissions start excluded. During use, they persist through device deep
sleep and same-tab browser reloads and navigation. The browser uses
`sessionStorage`; a fresh tab/session starts excluded. Favorites and language
preferences still use `localStorage`.

The device retains the menu cursor, draft, permissions, question text snapshot,
and per-language selection state in validated RTC memory. Full power loss or an
invalid RTC block restores defaults. A corpus fingerprint change clears its
index-based selection state. A copied question snapshot remains safe to display
even when the underlying corpus buffer is replaced.

## Selection and variance

Both platforms select at draw time, using one bag per language and a recent
window of 20. Filter changes preserve seen history. The priority order is:

1. Require every relevant permission.
2. Prefer unseen eligible questions. Reopen the eligible cycle only when exhausted;
   excluded questions retain their seen state.
3. Avoid recent questions when possible, then avoid the current question when possible.
4. After depth 3, prefer a non-3 question within those candidates.
5. Prefer a different depth band (1 versus 2/3) and a different form; each repetition
   costs one point.
6. Choose randomly within the best remaining group.

A breather never forces an early repeat. Small pools can relax recency and
texture, but permissions never relax. A singleton can repeat; an empty pool
stays empty. There is no escalation schedule, turn count, or judgment of the table.

## Website jobs

- Play mirrors the device and adds visible save/share actions. F opens or advances
  Filters; Space and Right perform Next. There is no previous/forward replay history.
- Browse exposes all questions with search, depth, tags, and sort. Play permissions
  do not filter Browse.
- Saved and shared collections remain at `/deck`, without relationship chips.
- Direct question links display their named question regardless of play permissions.
  They do not enable permissions; Next returns to the allowed automatic pool.
- Contribution collects original human wording. Editors assign depth and precise tags.

The visual language remains two tones, square corners, 1 px rules, ordered
dither, Zilla Slab questions, Literata wordmark, and IBM Plex Mono controls.
The short fade respects reduced motion. See the [brand guide](brand.md).

## Why a device

The device stays a shared object: e-paper keeps the conversation visible at
zero display power, without accounts, notifications, Bluetooth, or an app.
Wi-Fi and language setup remain on a phone. Sync is optional and does not
interrupt the displayed question. There are no physical favorites or automatic
session progression.

QDB4 removes category masks. Question manifests use schema 4 and require
firmware 0.2.0; RTC layout version 3 replaces version 2. A stored QDB3 bundle is
rejected and the embedded QDB4 corpus is the fallback. The electrical aliases
and enclosure geometry retain their existing identifiers; visible labels become
Filters. See [the protocol](sync_protocol.md) and
[firmware architecture](firmware_architecture.md).

Physical validation still needs to check menu legibility, accepted versus
dropped presses during real e-paper refreshes, wake replay, and total power loss.
