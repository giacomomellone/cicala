// Single place for external URLs — the org/domain are placeholders until the
// real ones exist (docs/decisions.md "Placeholder org and domain").
export const REPO_URL = "https://github.com/tischkarte/tischkarte";
export const SITE_URL = "https://tischkarte.pages.dev";

// Knob order — mirrors questions/schema.json x-tischkarte.categories.
// `all` is a mode, not a category; it never appears in data files.
export const CATEGORIES = ["party", "family", "love", "work", "deep"] as const;
export type Category = (typeof CATEGORIES)[number];

export const TAGS = [
  "icebreaker",
  "reflective",
  "spicy",
  "hypothetical",
  "memory",
  "wouldyourather",
] as const;
export type Tag = (typeof TAGS)[number];

// The OLED-style display number, shared with the device (docs/decisions.md):
// decimal of the first 2 id-hash bytes mod 10000.
export function displayId(id: string): string {
  return "#" + (parseInt(id.slice(2, 6), 16) % 10000);
}

export const MAX_SHARED_DECK = 150;
