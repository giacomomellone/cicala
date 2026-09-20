// External URLs are defined here so a domain or organization move is atomic.
export const REPO_URL = "https://github.com/giacomomellone/cicala";
export const CROSSPOINT_URL = "https://github.com/giacomomellone/cicala-crosspoint";
export const SITE_URL = "https://cicala.dev";

// Shipped languages from questions/schema.json. Generated payloads add counts,
// while the submission endpoint needs this stable name/code pair.
export const LANGUAGES = [
  { code: "en", name: "English" },
  { code: "de", name: "Deutsch" },
  { code: "it", name: "Italiano" },
] as const;

export const RECENT_WINDOW = 20;
export const DEFAULT_PERMISSIONS = { dark: false, sexual: false, heavy: false };
export type Permissions = typeof DEFAULT_PERMISSIONS;
export const FORMS = [
  "icebreaker",
  "reflective",
  "hypothetical",
  "memory",
  "wouldyourather",
] as const;

export const TAGS = [
  "icebreaker",
  "reflective",
  "sexual",
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
