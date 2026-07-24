# Voting: v2 stub (agreed design, not built)

One anonymous, account-free signal per question: **"sparked a good conversation."** It asks about the conversation, not the card, which is what separates it from a like button.

Agreed design, frozen here so v1 can leave the right seams:

- **Action:** a single tap on a hidden-for-now element. One vote per question per browser, deduplicated via `localStorage` (`tk.voted` array). No undo UI; re-tapping is a no-op.
- **Backend:** a Cloudflare Worker + KV counter keyed by question id. No cookies, no IP storage, no fingerprinting; abuse ceiling accepted as the cost of zero accounts.
- **Aggregation:** a weekly bot commit writes aggregates into `questions/votes.json` (id → count). The Git repo remains the single database; the KV store is only a buffer.
- **Display rules:** counts are **never shown on the play page** (§1 principle: nothing may compete with the question). Counts power only the `loved` sort on browse (v2) and maintainer curation.
- **Seams already present in v1:** every play meta line and browse row contains a hidden `data-vote-slot` element; `build_site_data.py` will merge `votes.json` into payloads when it exists.

Out of scope permanently: downvotes, comments, per-user history, trending feeds.
