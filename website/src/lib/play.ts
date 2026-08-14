// Play controller: no-repeat draws, session history, favorites, and sharing.

import { DECKS, PLAYBACK_DEPTH_MAX } from "../config";
import { tr } from "./apply-i18n";
import { drawFromBag } from "./bag";
import { detectLang, loadPayload, type Payload, type Question } from "./data";
import { getBag, getDeck, isFav, setBag, setDeck, toggleFav } from "./store";

interface Shown {
  q: Question;
  deck: string;
}

const HISTORY_MAX = 50;

const sleep = (ms: number) => new Promise((r) => setTimeout(r, ms));

export async function initPlay(): Promise<void> {
  const root = document.getElementById("play-root");
  if (!root) return;

  const qText = document.getElementById("q-text")!;
  const qSwap = document.getElementById("q-swap")!;
  const favBtn = document.getElementById("q-fav") as HTMLButtonElement;
  const favLabel = document.getElementById("q-fav-label")!;
  const shareBtn = document.getElementById("q-share") as HTMLButtonElement;
  const shareLabel = document.getElementById("q-share-label")!;
  const nextBtn = document.getElementById("q-next") as HTMLButtonElement;
  const selectors = Array.from(root.querySelectorAll<HTMLButtonElement>("[data-deck]"));

  const lang = detectLang();
  const isPermalink = root.dataset.permalink === "1";
  const seedId = root.dataset.seedId ?? "";
  let deck = isPermalink ? (root.dataset.seedDeck ?? "new_people") : getDeck();
  if (!(DECKS as readonly string[]).includes(deck)) deck = "new_people";

  let payload: Payload;
  try {
    payload = await loadPayload(lang);
  } catch {
    return; // static page keeps working with its SSR question
  }

  const byId = new Map(payload.questions.map((q) => [q.id, q]));

  const idsFor = (selectedDeck: string): string[] =>
    payload.questions
      .filter((q) => q.depth <= PLAYBACK_DEPTH_MAX && q.decks.includes(selectedDeck))
      .map((q) => q.id);

  function drawNext(selectedDeck: string): Shown | null {
    const bag = getBag(lang, selectedDeck);
    const id = drawFromBag(bag, idsFor(selectedDeck));
    if (id === null) return null;
    setBag(lang, selectedDeck, bag);
    const q = byId.get(id);
    return q ? { q, deck: selectedDeck } : null;
  }

  let history: Shown[] = [];
  let cursor = -1;

  function record(entry: Shown): void {
    history = history.slice(0, cursor + 1);
    history.push(entry);
    if (history.length > HISTORY_MAX) history.shift();
    cursor = history.length - 1;
  }

  const reducedMotion = matchMedia("(prefers-reduced-motion: reduce)").matches;

  function renderMeta(entry: Shown): void {
    const saved = isFav(entry.q.id);
    favBtn.setAttribute("aria-pressed", String(saved));
    favLabel.textContent = tr(lang, saved ? "play.saved" : "play.save");
  }

  function renderQuestion(entry: Shown): void {
    qText.textContent = entry.q.text;
    deck = entry.deck;
    setActiveDeck(deck);
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

  function setActiveDeck(selectedDeck: string): void {
    for (const selector of selectors)
      selector.setAttribute("aria-checked", String(selector.dataset.deck === selectedDeck));
  }

  // Replace the permalink entry when normal play begins.
  function leavePermalink(): void {
    if (location.pathname.startsWith("/q/")) window.history.replaceState({}, "", "/");
  }

  async function next(): Promise<void> {
    leavePermalink();
    if (cursor < history.length - 1) {
      cursor++;
      await show(history[cursor]!);
      return;
    }
    const entry = drawNext(deck);
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

  async function selectDeck(selectedDeck: string): Promise<void> {
    deck = selectedDeck;
    setDeck(selectedDeck);
    setActiveDeck(selectedDeck);
    leavePermalink();
    const entry = drawNext(selectedDeck);
    if (!entry) {
      qText.textContent = tr(lang, "play.empty");
      return;
    }
    record(entry);
    await show(entry);
  }

  const seedQuestion = seedId ? (byId.get(seedId) ?? null) : null;
  const seed = seedQuestion ? { q: seedQuestion, deck } : null;
  if (seed && isPermalink) {
    record(seed);
    renderMeta(seed);
    setActiveDeck(deck);
  } else {
    setActiveDeck(deck);
    const entry = drawNext(deck);
    if (entry) {
      record(entry);
      await show(entry, false);
    }
  }

  nextBtn.addEventListener("click", () => void next());

  for (const selector of selectors)
    selector.addEventListener("click", () => {
      const selectedDeck = selector.dataset.deck;
      if (selectedDeck) void selectDeck(selectedDeck);
    });

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
      /* Leave the label unchanged when clipboard access fails. */
    }
  });

  document.addEventListener("keydown", (e) => {
    if (e.ctrlKey || e.metaKey || e.altKey) return;
    const target = e.target as HTMLElement | null;
    if (target && target.closest("input, textarea, select, button, a, [contenteditable]")) return;
    if (e.key === " " || e.key === "ArrowRight") {
      e.preventDefault();
      void next();
    } else if (e.key === "ArrowLeft") {
      void prev();
    } else if (e.key === "f" || e.key === "F") {
      favBtn.click();
    } else if (e.key >= "1" && e.key <= "6") {
      const selector = selectors[Number(e.key) - 1];
      if (selector?.dataset.deck) void selectDeck(selector.dataset.deck);
    }
  });
}
