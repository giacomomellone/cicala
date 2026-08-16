// Runtime data access.

import { getStoredLang, setStoredLang } from "./store";

export interface Question {
  id: string;
  text: string;
  decks: string[];
  depth: number;
  tags: string[];
}

export interface Payload {
  version: string;
  generated: string;
  questions: Question[];
}

export interface LangInfo {
  code: string;
  name: string;
  count: number;
}

export interface RecentItem {
  id: string;
  text: string;
  decks: string[];
  depth: number;
  tags: string[];
  added: string;
}

export interface SiteConfig {
  langs: LangInfo[];
  payloads: Record<string, string>;
  recents: Record<string, string>;
  index: string;
}

let config: SiteConfig | null = null;

export function siteConfig(): SiteConfig {
  if (!config) {
    const el = document.getElementById("kveld-config");
    config = el
      ? (JSON.parse(el.textContent || "{}") as SiteConfig)
      : {
          langs: [],
          payloads: {},
          recents: {},
          index: "",
        };
  }
  return config;
}

/* Resolve language from the permalink, stored preference, browser, then English. */
export function detectLang(): string {
  const cfg = siteConfig();
  const shipped = cfg.langs.map((l) => l.code);
  const seed = document.body?.dataset.seedLang;
  if (seed && shipped.includes(seed)) {
    setStoredLang(seed);
    return seed;
  }
  const stored = getStoredLang();
  if (stored && shipped.includes(stored)) return stored;
  const nav = (navigator.language || "en").slice(0, 2).toLowerCase();
  if (shipped.includes(nav)) return nav;
  return shipped.includes("en") ? "en" : (shipped[0] ?? "en");
}

const payloadCache = new Map<string, Promise<Payload>>();

export function loadPayload(lang: string): Promise<Payload> {
  let cached = payloadCache.get(lang);
  if (!cached) {
    const url = siteConfig().payloads[lang];
    if (!url) return Promise.reject(new Error(`no payload for language ${lang}`));
    cached = fetch(url).then((r) => {
      if (!r.ok) throw new Error(`payload fetch failed: ${r.status}`);
      return r.json() as Promise<Payload>;
    });
    payloadCache.set(lang, cached);
  }
  return cached;
}

let indexCache: Promise<Record<string, string>> | null = null;

/* id → lang map, fetched on demand (deck links across languages). */
export function loadIndex(): Promise<Record<string, string>> {
  if (!indexCache) {
    indexCache = fetch(siteConfig().index).then((r) => r.json() as Promise<Record<string, string>>);
  }
  return indexCache;
}

export function loadRecent(lang: string): Promise<RecentItem[]> {
  const url = siteConfig().recents[lang];
  if (!url) return Promise.resolve([]);
  return fetch(url).then((r) => r.json() as Promise<RecentItem[]>);
}

export function allQuestions(payload: Payload): Question[] {
  return payload.questions;
}

export function findQuestion(payload: Payload, id: string): Question | null {
  return payload.questions.find((question) => question.id === id) ?? null;
}
