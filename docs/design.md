# Design

Product and UX rationale. This is the shared mental model for everyone touching the site, the firmware, or the database. [Theory](theory.md) governs the question corpus. When any decision conflicts with another, the principle below wins.

## The principle

**Minimize time-to-question, maximize time-in-conversation.** The product succeeds when people stop looking at it. Any proposed feature must either shorten the path to a good question or deepen the conversation after it. If it does neither, reject it, even if it would "improve engagement." Engagement with the product is the failure mode. Engagement between people is the goal.

## Non-goals (permanent unless explicitly revisited)

- No user accounts, login, or profiles. Anywhere, ever.
- No mobile app, no Bluetooth.
- No analytics in v1; never any third-party scripts. (A self-hosted, cookieless option may come later.)
- No visible vote counts on question cards (voting itself is v2, see [votes.md](votes.md)).
- No dark mode in website v1: light, paper-like only. CSS is structured with custom properties so dark mode is a v2 patch rather than a rewrite.
- No CMS, no database server. The Git repo is the database.
- No master-language translation pipeline, see [languages.md](languages.md).

## Why a device at all

The device's advantage over a phone:

1. It is a shared object. It gets passed around and left mid-table. A phone is personal territory; handing yours over is lending, not sharing.
2. The question persists at zero power. E-paper keeps displaying it after everyone's attention has moved into the conversation. In that sense the device *is* the table card.
3. Placing it on the table is the social invitation, like producing a deck of cards. No one has to say "let's do conversation prompts now"; the object says it.
4. It is finite and offline. There is no feed and nothing to check. When the questions run out, they reshuffle, and the device never asks for attention back.

## Device interaction surface (complete — nothing else exists)

| Input | Action | Feedback |
|---|---|---|
| Turn knob | Change category | OLED wakes and scrolls category names; e-paper untouched |
| Press | Next question in category | One e-paper partial refresh (~0.3 s); OLED shows `#274 · deep` for 3 s |
| Long-press 1.5 s | Utility menu on OLED only | sync now / Wi-Fi setup / language / battery / about; 10 s timeout |
| Idle 30 s | Deep sleep | OLED off; question remains on e-paper; µA draw |

Hard rules:

- The e-paper shows **only questions**, never menus, logos, or status. It belongs to the table, not to the device.
- The OLED is dark whenever hands are off the device.
- The device is fully functional out of the box with the preloaded database. Wi-Fi is optional forever.
- Sync runs opportunistically while charging and never interrupts use.

## Website jobs

Flat navigation: play / browse / contribute / device. In priority order the site is:

1. The player, for people without the device. One question, huge type, next on Space. Nothing else competes with the question.
2. The contribution surface. A 30-second flow for non-developers; the "recently added" list closes the loop by showing contributions shipping within minutes.
3. The bridge to the device (v2: deck codes).

The site should feel like the e-paper device: calm, typographic, slightly warm. Sentence case everywhere, one accent color, no images outside the device page, no motion beyond a 120 ms fade.

## v2 stubs (agreed design, do not build yet)

- **Anonymous submissions:** a Cloudflare Worker that accepts submissions without a GitHub account and opens PRs via a bot account. Removes the single biggest contribution barrier; deferred because v1 must stay zero-backend.
- **Deck codes:** short codes that move a favorites deck from the site onto a device.
- **Voting:** one anonymous "sparked a good conversation" action per question. Design in [votes.md](votes.md); the seam is already in the DOM (`data-vote-slot`).
- **Dark mode:** a token-swap patch on `tokens.css`.
