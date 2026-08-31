// Browse controller for search, filters, sorting, and pagination.

import { tr } from "./apply-i18n";
import { shuffle } from "./bag";
import { detectLang, loadPayload, type Payload } from "./data";
import { bindHearts, rowHtml, type RowItem } from "./rows";

const PAGE = 100;

function fold(s: string): string {
  return s
    .normalize("NFD")
    .replace(/[\u0300-\u036f]/g, "")
    .toLowerCase();
}

export async function initBrowse(): Promise<void> {
  const rowsEl = document.getElementById("rows");
  if (!rowsEl) return;
  const searchEl = document.getElementById("b-search") as HTMLInputElement;
  const tagEl = document.getElementById("b-tag") as HTMLSelectElement;
  const sortEl = document.getElementById("b-sort") as HTMLSelectElement;
  const moreBtn = document.getElementById("b-more") as HTMLButtonElement;
  const emptyEl = document.getElementById("b-empty")!;
  const emptySuggest = document.getElementById("b-suggest-empty") as HTMLAnchorElement;
  const chips = Array.from(document.querySelectorAll<HTMLButtonElement>("#browse-chips .chip"));

  const lang = detectLang();
  let payload: Payload;
  try {
    payload = await loadPayload(lang);
  } catch {
    return; // SSR list stays usable
  }

  // Question files are append-only, so reverse file order is newest first.
  const newestFirst: RowItem[] = payload.questions.map((q) => ({ q })).reverse();

  let deck = "all";
  let tag = "";
  let sort: "newest" | "random" = "newest";
  let query = "";
  let shown = PAGE;
  let randomOrder: RowItem[] = [];

  const setActiveChip = () => {
    for (const chip of chips) chip.setAttribute("aria-checked", String(chip.dataset.deck === deck));
  };

  function filtered(): RowItem[] {
    const source = sort === "random" ? randomOrder : newestFirst;
    const q = fold(query.trim());
    return source.filter(
      (item) =>
        (deck === "all" || item.q.decks.includes(deck)) &&
        (tag === "" || item.q.tags.includes(tag)) &&
        (q === "" || fold(item.q.text).includes(q)),
    );
  }

  function render(): void {
    const items = filtered();
    rowsEl!.innerHTML = items
      .slice(0, shown)
      .map((item) => rowHtml(item, lang))
      .join("");
    moreBtn.hidden = items.length <= shown;
    emptyEl.hidden = items.length > 0;
    const suggestion = new URL("/suggest", location.origin);
    suggestion.searchParams.set("source", "browse-empty");
    if (query.trim()) suggestion.searchParams.set("text", query.trim());
    emptySuggest.href = `${suggestion.pathname}${suggestion.search}`;
  }

  setActiveChip();
  render();
  bindHearts(rowsEl);

  searchEl.addEventListener("input", () => {
    query = searchEl.value;
    shown = PAGE;
    render();
  });

  for (const chip of chips)
    chip.addEventListener("click", () => {
      deck = chip.dataset.deck ?? "all";
      setActiveChip();
      shown = PAGE;
      render();
    });

  tagEl.addEventListener("change", () => {
    tag = tagEl.value;
    shown = PAGE;
    render();
  });

  sortEl.addEventListener("change", () => {
    sort = sortEl.value === "random" ? "random" : "newest";
    if (sort === "random") randomOrder = shuffle(newestFirst.slice());
    shown = PAGE;
    render();
  });

  moreBtn.addEventListener("click", () => {
    shown += PAGE;
    render();
  });

  moreBtn.textContent = tr(lang, "browse.more");
}
