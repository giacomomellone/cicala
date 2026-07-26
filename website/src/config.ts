// Single place for external URLs — the org/domain are placeholders until the
// real ones exist (docs/decisions.md "Placeholder org and domain").
export const REPO_URL = "https://github.com/tischkarte/tischkarte";
export const SITE_URL = "https://tischkarte.pages.dev";

// Absolute selector order — mirrors questions/schema.json x-tischkarte.decks.
// `all` exists only as a website browse filter.
export const DECKS = [
  "new_people",
  "close",
  "family",
  "work",
  "here",
  "wild",
] as const;
export type Deck = (typeof DECKS)[number];
export const PLAYBACK_DEPTH_MAX = 2;

export const TAGS = [
  "icebreaker",
  "reflective",
  "spicy",
  "dark",
  "hypothetical",
  "memory",
  "wouldyourather",
] as const;
export type Tag = (typeof TAGS)[number];

export const MAX_SHARED_DECK = 150;
