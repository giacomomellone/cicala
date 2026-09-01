// Play controller: the device's Category/Next model, plus the web extras a
// browser can support without adding state to the object — history,
// favorites, and sharing.

import { BAG_TEXTURE, DEVICE_DECKS, PLAYBACK_DEPTH_MAX } from "../config";
import { tr } from "./apply-i18n";
import { drawFromBag } from "./bag";
import { detectLang, loadPayload, type Payload, type Question } from "./data";
import {
  getBag,
  getDeck,
  getLastShown,
  isFav,
  setBag,
  setDeck,
  setLastShown,
  toggleFav,
} from "./store";

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
  const qMeta = document.getElementById("q-meta")!;
  const deckName = document.getElementById("q-deck-name")!;
  const originLink = document.getElementById("q-origin") as HTMLAnchorElement;
  const editLink = document.getElementById("q-edit");
  const favBtn = document.getElementById("q-fav") as HTMLButtonElement;
  const favLabel = document.getElementById("q-fav-label")!;
  const shareBtn = document.getElementById("q-share") as HTMLButtonElement;
  const shareLabel = document.getElementById("q-share-label")!;
  const nextBtn = document.getElementById("q-next") as HTMLButtonElement;
  const categoryBtn = document.getElementById("q-category") as HTMLButtonElement;

  const lang = detectLang();
  const isPermalink = root.dataset.permalink === "1";
  const seedId = root.dataset.seedId ?? "";

  // The player offers the device cycle; anything else (e.g. a stored or
  // permalink deck from before parity) lands on the cycle's first deck.
  const playDeck = (name: string): string =>
    (DEVICE_DECKS as readonly string[]).includes(name) ? name : DEVICE_DECKS[0];

  let deck = playDeck(isPermalink ? (root.dataset.seedDeck ?? "new_people") : getDeck());

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

  const deckLabel = (name: string): string => tr(lang, `deck.${name}` as never);

  function drawNext(selectedDeck: string): Shown | null {
    const bag = getBag(lang, selectedDeck);
    const texture = BAG_TEXTURE
      ? { seed: byId.get(getLastShown(lang)), meta: (id: string) => byId.get(id) }
      : undefined;
    const id = drawFromBag(bag, idsFor(selectedDeck), Math.random, texture);
    if (id === null) return null;
    setBag(lang, selectedDeck, bag);
    setLastShown(lang, id);
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

  /* Provenance belongs to the question on screen, not to the page: it appears
     only on a machine translation, and links to the human-written original. */
  function renderOrigin(entry: Shown): void {
    const origin = entry.q.translated_by === "google" ? entry.q.origin : undefined;
    originLink.hidden = !origin;
    if (origin) originLink.href = `/q/${origin}`;
  }

  function renderQuestion(entry: Shown): void {
    qText.classList.remove("is-name");
    qMeta.style.visibility = "";
    qText.textContent = entry.q.text;
    deck = playDeck(entry.deck);
    deckName.textContent = deckLabel(deck);
    renderOrigin(entry);
    renderMeta(entry);
    document.documentElement.lang = lang;
  }

  /* The deck-name card, as on the device: the name replaces the question
     until Next draws. */
  function renderName(): void {
    qText.classList.add("is-name");
    qText.textContent = deckLabel(deck);
    qMeta.style.visibility = "hidden";
  }

  async function swap(render: () => void, fade = true): Promise<void> {
    if (fade && !reducedMotion) {
      qSwap.classList.add("is-fading");
      await sleep(120);
      render();
      qSwap.classList.remove("is-fading");
    } else {
      render();
    }
  }

  async function show(entry: Shown, fade = true): Promise<void> {
    await swap(() => renderQuestion(entry), fade);
  }

  // Replace the permalink entry when normal play begins.
  function leavePermalink(): void {
    if (!location.pathname.startsWith("/q/")) return;
    window.history.replaceState({}, "", "/");
    editLink?.remove();
  }

  async function next(): Promise<void> {
    leavePermalink();
    // Forward history only belongs to the deck it was drawn from; a Category
    // change since makes Next a fresh draw, and record() drops the tail.
    if (cursor < history.length - 1 && history[cursor + 1]!.deck === deck) {
      cursor++;
      await show(history[cursor]!);
      return;
    }
    const entry = drawNext(deck);
    if (!entry) {
      qText.classList.remove("is-name");
      qMeta.style.visibility = "";
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

  async function cycleCategory(): Promise<void> {
    leavePermalink();
    const i = (DEVICE_DECKS as readonly string[]).indexOf(deck);
    deck = DEVICE_DECKS[(i + 1) % DEVICE_DECKS.length] ?? "new_people";
    setDeck(deck);
    await swap(renderName);
  }

  const seedQuestion = seedId ? (byId.get(seedId) ?? null) : null;
  const seed = seedQuestion ? { q: seedQuestion, deck } : null;
  if (seed && isPermalink) {
    record(seed);
    renderMeta(seed);
    setLastShown(lang, seed.q.id); // the permalink is what is on screen
  } else {
    const entry = drawNext(deck);
    if (entry) {
      record(entry);
      await show(entry, false);
    }
  }
  deckName.textContent = deckLabel(deck);

  nextBtn.addEventListener("click", () => void next());
  categoryBtn.addEventListener("click", () => void cycleCategory());

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
    } else if (e.key === "c" || e.key === "C") {
      void cycleCategory();
    } else if (e.key === "f" || e.key === "F") {
      favBtn.click();
    }
  });
}
