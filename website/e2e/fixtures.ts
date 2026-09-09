// Shared helpers for the end-to-end suites.

import { readFileSync } from "node:fs";
import { fileURLToPath } from "node:url";
import { test, type Page } from "@playwright/test";
import { DEFAULT_PERMISSIONS } from "../src/config";
import { permitted } from "../src/lib/bag";

const at = (relative: string) => fileURLToPath(new URL(relative, import.meta.url));

const readJson = <T>(relative: string): T => JSON.parse(readFileSync(at(relative), "utf8")) as T;

export interface Question {
  id: string;
  text: string;
  depth: number;
  tags: string[];
}

export interface Schema {
  "x-cicala": {
    tags: string[];
    depthLabels: string[];
    languages: Record<string, { name: string }>;
  };
}

export const schema = readJson<Schema>("../../questions/schema.json")["x-cicala"];

export const payload = (lang: string): Question[] =>
  readJson<{ questions: Question[] }>(`../src/data/questions.${lang}.json`).questions;

/** Skip a test that has nothing to drive because a corpus ships no questions.
    These suites read the real database on purpose, so an empty language is a
    missing fixture rather than a failure; they run again on their own once the
    language has entries. */
export const skipWithoutQuestions = (...langs: string[]): void => {
  const empty = langs.filter((lang) => payload(lang).length === 0);
  test.skip(empty.length > 0, `no questions in the ${empty.join(" and ")} corpus yet`);
};

/** Automatic play uses explicit permissions, independent of Browse. */
export const playable = (lang: string): Question[] =>
  payload(lang).filter((q) => permitted(q, DEFAULT_PERMISSIONS));

/** Wait for the play island to replace the build-time seed question. */
export async function playReady(page: Page, lang = "en"): Promise<void> {
  await page.waitForFunction(
    (prefix) => Object.keys(sessionStorage).some((key) => key.startsWith(prefix)),
    `cicala.play.v1.${lang}`,
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
