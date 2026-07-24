// Play controller (spec §7.4): shuffle-bag randomness with no repeats,
// session history (in-memory, max 50), keyboard map, favorites, share.

import { CATEGORIES, displayId } from "../config";
import { tr } from "./apply-i18n";
import { drawFromBag } from "./bag";
import {
  detectLang,
  loadPayload,
  type Payload,
  type Question,
} from "./data";
import { getBag, getCat, isFav, setBag, setCat, toggleFav } from "./store";

interface Shown {
  q: Question;
  category: string;
}

const HISTORY_MAX = 50;

const sleep = (ms: number) => new Promise((r) => setTimeout(r, ms));

export async function initPlay(): Promise<void> {
  const root = document.getElementById("play-root");
  if (!root) return;

  const qText = document.getElementById("q-text")!;
  const qSwap = document.getElementById("q-swap")!;
  const qNum = document.getElementById("q-num")!;
  const qCat = document.getElementById("q-cat")!;
  const favBtn = document.getElementById("q-fav") as HTMLButtonElement;
  const favLabel = document.getElementById("q-fav-label")!;
  const shareBtn = document.getElementById("q-share") as HTMLButtonElement;
  const shareLabel = document.getElementById("q-share-label")!;
  const nextBtn = document.getElementById("q-next") as HTMLButtonElement;
  const chips = Array.from(
    root.querySelectorAll<HTMLButtonElement>(".chip[data-cat]"),
  );

  const lang = detectLang();
  const isPermalink = root.dataset.permalink === "1";
  const seedId = root.dataset.seedId ?? "";
  let cat = isPermalink ? (root.dataset.seedCat ?? "all") : getCat();
  if (cat !== "all" && !(CATEGORIES as readonly string[]).includes(cat)) cat = "all";

  let payload: Payload;
  try {
    payload = await loadPayload(lang);
  } catch {
    return; // static page keeps working with its SSR question
  }

  const byId = new Map<string, Shown>();
  for (const [category, questions] of Object.entries(payload.categories)) {
    for (const q of questions) byId.set(q.id, { q, category });
  }

  const idsFor = (c: string): string[] =>
    c === "all"
      ? CATEGORIES.flatMap((k) => (payload.categories[k] ?? []).map((q) => q.id))
      : (payload.categories[c] ?? []).map((q) => q.id);

  // --------------------------------------------------------- shuffle bag

  function drawNext(c: string): Shown | null {
    const bag = getBag(lang, c);
    const id = drawFromBag(bag, idsFor(c));
    if (id === null) return null;
    setBag(lang, c, bag);
    return byId.get(id) ?? null;
  }

  // ------------------------------------------------------------- history

  let history: Shown[] = [];
  let cursor = -1;

  function record(entry: Shown): void {
    history = history.slice(0, cursor + 1);
    history.push(entry);
    if (history.length > HISTORY_MAX) history.shift();
    cursor = history.length - 1;
  }

  // ------------------------------------------------------------ rendering

  const reducedMotion = matchMedia("(prefers-reduced-motion: reduce)").matches;

  function renderMeta(entry: Shown): void {
    qNum.textContent = displayId(entry.q.id);
    qCat.textContent = tr(lang, `cat.${entry.category}` as never);
    qCat.dataset.cat = entry.category;
    const saved = isFav(entry.q.id);
    favBtn.setAttribute("aria-pressed", String(saved));
    favLabel.textContent = tr(lang, saved ? "play.saved" : "play.save");
  }

  function renderQuestion(entry: Shown): void {
    qText.textContent = entry.q.text;
    renderMeta(entry);
    document.documentElement.lang = lang;
  }

  async function show(entry: Shown, fade = true): Promise<void> {
    if (fade && !reducedMotion) {
      qSwap.classList.add("is-fading");
      await sleep(120);
      renderQuestion(entry);
      qSwap.classList.remove("is-fading");
    } else {
      renderQuestion(entry);
    }
  }

  function setActiveChip(c: string): void {
    for (const chip of chips)
      chip.setAttribute("aria-checked", String(chip.dataset.cat === c));
  }

  // --------------------------------------------------------------- moves

  // "next" from a permalink transitions into normal play without history spam
  function leavePermalink(): void {
    if (location.pathname.startsWith("/q/"))
      window.history.replaceState({}, "", "/");
  }

  async function next(): Promise<void> {
    leavePermalink();
    if (cursor < history.length - 1) {
      cursor++;
      await show(history[cursor]!);
      return;
    }
    const entry = drawNext(cat);
    if (!entry) {
      qText.textContent = tr(lang, "play.empty");
      return;
    }
    record(entry);
    await show(entry);
  }

  async function prev(): Promise<void> {
    if (cursor > 0) {
      cursor--;
      await show(history[cursor]!);
    }
  }

  async function selectCat(c: string): Promise<void> {
    cat = c;
    setCat(c);
    setActiveChip(c);
    leavePermalink();
    const entry = drawNext(c);
    if (!entry) {
      qText.textContent = tr(lang, "play.empty");
      return;
    }
    record(entry);
    await show(entry);
  }

  // ---------------------------------------------------------------- init

  const seed = seedId ? (byId.get(seedId) ?? null) : null;
  if (seed && isPermalink) {
    // permalink: the SSR question is already on screen — adopt it
    record(seed);
    renderMeta(seed);
    setActiveChip(cat);
  } else {
    // normal play: draw from the bag (replaces the build-time SSR question)
    setActiveChip(cat);
    const entry = drawNext(cat);
    if (entry) {
      record(entry);
      await show(entry, false);
    }
  }

  // --------------------------------------------------------------- events

  nextBtn.addEventListener("click", () => void next());

  for (const chip of chips)
    chip.addEventListener("click", () => void selectCat(chip.dataset.cat ?? "all"));

  favBtn.addEventListener("click", () => {
    const entry = history[cursor];
    if (!entry) return;
    const saved = toggleFav(entry.q.id);
    favBtn.setAttribute("aria-pressed", String(saved));
    favLabel.textContent = tr(lang, saved ? "play.saved" : "play.save");
  });

  let shareTimer: ReturnType<typeof setTimeout> | undefined;
  shareBtn.addEventListener("click", async () => {
    const entry = history[cursor];
    if (!entry) return;
    const url = `${location.origin}/q/${entry.q.id}`;
    try {
      await navigator.clipboard.writeText(url);
      shareLabel.textContent = tr(lang, "play.copied");
      clearTimeout(shareTimer);
      shareTimer = setTimeout(() => {
        shareLabel.textContent = tr(lang, "play.share");
      }, 1500);
    } catch {
      /* clipboard unavailable (http, permissions) — leave the label alone */
    }
  });

  // Keyboard map (spec §7.4). Never hijack keys when a form element or any
  // interactive element is focused.
  document.addEventListener("keydown", (e) => {
    if (e.ctrlKey || e.metaKey || e.altKey) return;
    const target = e.target as HTMLElement | null;
    if (
      target &&
      target.closest("input, textarea, select, button, a, [contenteditable]")
    )
      return;
    if (e.key === " " || e.key === "ArrowRight") {
      e.preventDefault();
      void next();
    } else if (e.key === "ArrowLeft") {
      void prev();
    } else if (e.key === "f" || e.key === "F") {
      favBtn.click();
    } else if (e.key >= "1" && e.key <= "6") {
      const chip = chips[Number(e.key) - 1];
      if (chip) void selectCat(chip.dataset.cat ?? "all");
    }
  });
}
