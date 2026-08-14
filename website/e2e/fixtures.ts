// Shared helpers for the end-to-end suites. Everything here reads the real
// build inputs — the generated payloads and the question schema — so a test
// never restates a value the site could have got wrong.

import { readFileSync } from "node:fs";
import { fileURLToPath } from "node:url";
import type { Page } from "@playwright/test";

const at = (relative: string) => fileURLToPath(new URL(relative, import.meta.url));

const readJson = <T>(relative: string): T => JSON.parse(readFileSync(at(relative), "utf8")) as T;

export interface Question {
  id: string;
  text: string;
  decks: string[];
  depth: number;
  tags: string[];
}

export interface Schema {
  "x-tischkarte": {
    decks: string[];
    tags: string[];
    depthLabels: string[];
    playbackDepthMax: number;
    languages: Record<string, { name: string }>;
  };
}

export const schema = readJson<Schema>("../../questions/schema.json")["x-tischkarte"];

export const payload = (lang: string): Question[] =>
  readJson<{ questions: Question[] }>(`../src/data/questions.${lang}.json`).questions;

/** Questions the player can actually serve for a deck (spec §7.4). */
export const playable = (lang: string, deck: string): Question[] =>
  payload(lang).filter((q) => q.depth <= schema.playbackDepthMax && q.decks.includes(deck));

/** The deck with the fewest playable questions — the cheapest one to exhaust. */
export const smallestDeck = (lang: string): string =>
  schema.decks
    .map((deck) => ({ deck, size: playable(lang, deck).length }))
    .sort((a, b) => a.size - b.size)[0]!.deck;

/**
 * Wait for the play island to replace the build-time seed question.
 *
 * `/` ships a statically rendered question so the page works without
 * JavaScript; src/lib/play.ts draws its own the moment the payload lands.
 * Reading the question before that swap reads the seed. The first draw is
 * also the first bag write, so the bag key appearing marks the handover.
 */
export async function playReady(page: Page, lang = "en"): Promise<void> {
  await page.waitForFunction(
    (prefix) => Object.keys(localStorage).some((key) => key.startsWith(prefix)),
    `tk.bag.${lang}.`,
  );
}

/**
 * Wait for a list island to take over from the statically rendered rows.
 *
 * Browse and deck ship a server-rendered list so the page works without
 * JavaScript, and re-render it once the payload arrives. Until then the
 * toolbar is inert markup: clicking "show more" before the island has bound
 * its listener does nothing. The site fetches once and never polls, so an
 * idle network means the handover is done.
 */
export async function islandReady(page: Page): Promise<void> {
  await page.waitForLoadState("networkidle");
}

/**
 * Capture `window.open` calls instead of opening tabs. The contribute form
 * hands off to GitHub that way, and the URL it builds is the contract under
 * test.
 */
export async function captureWindowOpen(page: Page): Promise<void> {
  await page.addInitScript(() => {
    (window as unknown as { __opened: string[] }).__opened = [];
    window.open = ((url?: string | URL) => {
      (window as unknown as { __opened: string[] }).__opened.push(String(url));
      return null;
    }) as typeof window.open;
  });
}

export const openedUrls = (page: Page): Promise<string[]> =>
  page.evaluate(() => (window as unknown as { __opened: string[] }).__opened);

/** Seed localStorage before the first script runs. */
export async function seedStorage(page: Page, entries: Record<string, string>): Promise<void> {
  await page.addInitScript((values) => {
    for (const [key, value] of Object.entries(values)) localStorage.setItem(key, value);
  }, entries);
}
