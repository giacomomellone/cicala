// Deck controller (spec §7.9): favorites viewer + shared decks via #ids=
// (fragment, not query — ids never hit server logs). Mixed-language decks
// work because ids are language-scoped; missing languages resolve through
// the id→lang index on demand.

import { DECKS, MAX_SHARED_DECK } from "../config";
import { tr } from "./apply-i18n";
import {
  detectLang,
  findQuestion,
  loadIndex,
  loadPayload,
} from "./data";
import { bindHearts, rowHtml, type RowItem } from "./rows";
import { getFavs, setFavs } from "./store";

interface DeckItem extends RowItem {
  lang: string;
}

function parseSharedIds(): string[] | null {
  const m = location.hash.match(/^#ids=(.+)$/);
  if (!m) return null;
  return m[1]!
    .split(",")
    .map((s) => decodeURIComponent(s.trim()))
    .filter((s) => /^q-[0-9a-f]{8}$/.test(s));
}

/** Resolve ids (possibly across languages) to questions; unknown ids are
 * silently skipped (spec §7.9). */
async function resolve(ids: string[], activeLang: string): Promise<DeckItem[]> {
  const items = new Map<string, DeckItem>();
  const unresolved: string[] = [];

  const tryLang = async (lang: string, subset: string[]) => {
    try {
      const payload = await loadPayload(lang);
      for (const id of subset) {
        const hit = findQuestion(payload, id);
        if (hit) items.set(id, { q: hit, lang });
      }
    } catch {
      /* language payload unavailable — those ids stay unresolved */
    }
  };

  await tryLang(activeLang, ids);
  for (const id of ids) if (!items.has(id)) unresolved.push(id);

  if (unresolved.length > 0) {
    try {
      const index = await loadIndex();
      const byLang = new Map<string, string[]>();
      for (const id of unresolved) {
        const lang = index[id];
        if (lang && lang !== activeLang) {
          byLang.set(lang, [...(byLang.get(lang) ?? []), id]);
        }
      }
      for (const [lang, subset] of byLang) await tryLang(lang, subset);
    } catch {
      /* index unavailable — cross-language ids are skipped */
    }
  }

  return ids
    .map((id) => items.get(id))
    .filter((x): x is DeckItem => x !== undefined);
}

export async function initDeck(): Promise<void> {
  const rowsEl = document.getElementById("d-rows");
  if (!rowsEl) return;
  const titleEl = document.getElementById("d-title")!;
  const countEl = document.getElementById("d-count")!;
  const emptyEl = document.getElementById("d-empty")!;
  const statusEl = document.getElementById("d-status")!;
  const shareBtn = document.getElementById("d-share") as HTMLButtonElement;
  const exportBtn = document.getElementById("d-export") as HTMLButtonElement;
  const importBtn = document.getElementById("d-import") as HTMLButtonElement;
  const importFile = document.getElementById("d-file") as HTMLInputElement;
  const saveAllBtn = document.getElementById("d-saveall") as HTMLButtonElement;
  const ownActions = document.getElementById("d-own-actions")!;
  const chips = Array.from(
    document.querySelectorAll<HTMLButtonElement>("#deck-chips .chip"),
  );

  const lang = detectLang();
  const sharedIds = parseSharedIds();
  const shared = sharedIds !== null;
  let ids = shared ? sharedIds! : getFavs();

  if (shared) {
    titleEl.textContent = tr(lang, "deck.shared.title");
    ownActions.hidden = true;
    saveAllBtn.hidden = false;
  }

  let items: DeckItem[] = await resolve(ids, lang);
  let deck = "all";

  function render(): void {
    const visible =
      deck === "all" ? items : items.filter((i) => i.q.decks.includes(deck));
    rowsEl!.innerHTML = visible.map((i) => rowHtml(i, lang)).join("");
    countEl.textContent = `${visible.length} ${tr(lang, "deck.count")}`;
    const empty = items.length === 0;
    emptyEl.hidden = !empty;
    countEl.hidden = empty;
    if (!shared) {
      shareBtn.disabled = empty;
      exportBtn.disabled = empty;
    }
  }

  const setActiveChip = () => {
    for (const chip of chips)
      chip.setAttribute("aria-checked", String(chip.dataset.deck === deck));
  };

  setActiveChip();
  render();
  if (!shared) bindHearts(rowsEl); // shared view is read-only

  for (const chip of chips)
    chip.addEventListener("click", () => {
      deck = chip.dataset.deck && chip.dataset.deck !== "all" &&
        (DECKS as readonly string[]).includes(chip.dataset.deck)
        ? chip.dataset.deck
        : "all";
      setActiveChip();
      render();
    });

  let statusTimer: ReturnType<typeof setTimeout> | undefined;
  function flash(msg: string): void {
    statusEl.textContent = msg;
    clearTimeout(statusTimer);
    statusTimer = setTimeout(() => (statusEl.textContent = ""), 2500);
  }

  shareBtn?.addEventListener("click", async () => {
    const current = getFavs();
    const capped = current.slice(0, MAX_SHARED_DECK);
    const url = `${location.origin}/deck#ids=${capped.join(",")}`;
    try {
      await navigator.clipboard.writeText(url);
      flash(
        current.length > MAX_SHARED_DECK
          ? tr(lang, "deck.share.truncated")
          : tr(lang, "deck.share.copied"),
      );
    } catch {
      /* clipboard unavailable */
    }
  });

  exportBtn?.addEventListener("click", () => {
    const blob = new Blob([JSON.stringify({ ids: getFavs() }, null, 2)], {
      type: "application/json",
    });
    const a = document.createElement("a");
    a.href = URL.createObjectURL(blob);
    a.download = "deck.json";
    a.click();
    URL.revokeObjectURL(a.href);
  });

  importBtn?.addEventListener("click", () => importFile.click());
  importFile?.addEventListener("change", async () => {
    const file = importFile.files?.[0];
    if (!file) return;
    try {
      const parsed = JSON.parse(await file.text()) as unknown;
      const incoming = Array.isArray(parsed)
        ? parsed
        : ((parsed as { ids?: unknown }).ids ?? []);
      if (!Array.isArray(incoming)) return;
      const merged = getFavs();
      for (const id of incoming)
        if (typeof id === "string" && /^q-[0-9a-f]{8}$/.test(id) && !merged.includes(id))
          merged.push(id);
      setFavs(merged);
      ids = merged;
      items = await resolve(ids, lang);
      render();
    } catch {
      /* not a deck file — ignore */
    }
    importFile.value = "";
  });

  saveAllBtn?.addEventListener("click", () => {
    const merged = getFavs();
    for (const id of ids) if (!merged.includes(id)) merged.push(id);
    setFavs(merged);
    flash(tr(lang, "deck.saved"));
  });
}
