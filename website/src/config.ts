// External URLs are defined here so a domain or organization move is atomic.
export const REPO_URL = "https://github.com/giacomomellone/cicala";
export const SITE_URL = "https://cicala.dev";

// Category cycle order from questions/schema.json.
export const DECKS = ["new_people", "close", "family", "work", "here", "wild"] as const;
export type Deck = (typeof DECKS)[number];

// The decks the player offers, in the same order as the device's Category
// cycle (firmware/app/src/channels.c). Work stays in the corpus and browse.
export const DEVICE_DECKS = ["new_people", "close", "family", "here", "wild"] as const;
export const PLAYBACK_DEPTH_MAX = 2;

// GitHub issue-form depth values, in schema order.
export const DEPTH_OPTIONS = [
  "1 — little public exposure",
  "2 — a personal construction",
  "3 — vulnerability, conflict, fear, loss, or consequential disclosure",
] as const;

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

// The bag prefers a depth band and form different from the question just
// shown, relaxing to a uniform draw when the pool cannot offer one. Study
// builds flip this off to compare against uniform draws.
export const BAG_TEXTURE = true;
