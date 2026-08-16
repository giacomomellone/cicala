// External URLs are defined here so a domain or organization move is atomic.
export const REPO_URL = "https://github.com/giacomomellone/kveld";
export const SITE_URL = "https://kveld.pages.dev";

// Category cycle order from questions/schema.json.
export const DECKS = ["new_people", "close", "family", "work", "here", "wild"] as const;
export type Deck = (typeof DECKS)[number];
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
