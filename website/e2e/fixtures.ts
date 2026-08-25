// Shared helpers for the end-to-end suites.

import { readFileSync } from "node:fs";
import { fileURLToPath } from "node:url";
import type { Page } from "@playwright/test";
import { DEVICE_DECKS } from "../src/config";

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
  "x-cicala": {
    decks: string[];
    tags: string[];
    depthLabels: string[];
    playbackDepthMax: number;
    languages: Record<string, { name: string }>;
  };
}

export const schema = readJson<Schema>("../../questions/schema.json")["x-cicala"];

export const payload = (lang: string): Question[] =>
  readJson<{ questions: Question[] }>(`../src/data/questions.${lang}.json`).questions;

/** Questions the player can actually serve for a deck (spec §7.4). */
export const playable = (lang: string, deck: string): Question[] =>
  payload(lang).filter((q) => q.depth <= schema.playbackDepthMax && q.decks.includes(deck));

/** The category that requires the fewest draws to exhaust, among the decks
    the player offers. */
export const smallestDeck = (lang: string): string =>
  DEVICE_DECKS.map((deck) => ({ deck, size: playable(lang, deck).length })).sort(
    (a, b) => a.size - b.size,
  )[0]!.deck;

/** Wait for the play island to replace the build-time seed question. */
export async function playReady(page: Page, lang = "en"): Promise<void> {
  await page.waitForFunction(
    (prefix) => Object.keys(localStorage).some((key) => key.startsWith(prefix)),
    `cicala.bag.${lang}.`,
  );
}

/** Wait for a list island to take over from the statically rendered rows. */
export async function islandReady(page: Page): Promise<void> {
  await page.waitForLoadState("networkidle");
}

/** Capture `window.open` calls instead of opening tabs. */
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
