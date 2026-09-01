// Shared question-row rendering for browse and deck pages.

import { tr } from "./apply-i18n";
import type { Question } from "./data";
import { heartIcon } from "./icons";
import { isFav, toggleFav } from "./store";

export interface RowItem {
  q: Question;
}

function escapeHtml(s: string): string {
  return s
    .replace(/&/g, "&amp;")
    .replace(/</g, "&lt;")
    .replace(/>/g, "&gt;")
    .replace(/"/g, "&quot;");
}

export function rowHtml(item: RowItem, lang: string): string {
  const decks = item.q.decks
    .map((deck) => `<span>${escapeHtml(tr(lang, `deck.${deck}` as never))}</span>`)
    .join("");
  const tags = item.q.tags
    .map((tag) => `<span>${escapeHtml(tr(lang, `tag.${tag}` as never))}</span>`)
    .join("");
  const provenance =
    item.q.translated_by === "google" && item.q.origin
      ? `<a href="/q/${escapeHtml(item.q.origin)}">${escapeHtml(tr(lang, "question.translated"))}</a>`
      : "";
  const saved = isFav(item.q.id);
  return `<li class="row" data-id="${item.q.id}">
  <div class="row-main">
    <a class="row-q" href="/q/${item.q.id}">${escapeHtml(item.q.text)}</a>
    <div class="row-meta">
      ${decks}<span>${escapeHtml(tr(lang, `depth.${item.q.depth}` as never))}</span>${tags}${provenance}
    </div>
  </div>
  <button type="button" class="iconbtn row-fav" aria-pressed="${saved}"
    aria-label="${escapeHtml(tr(lang, "play.save"))}">${heartIcon}</button>
  <span data-vote-slot hidden></span>
</li>`;
}

/* One delegated listener per list container handles every heart. */
export function bindHearts(container: HTMLElement): void {
  container.addEventListener("click", (e) => {
    const btn = (e.target as HTMLElement).closest<HTMLButtonElement>(".row-fav");
    if (!btn) return;
    e.preventDefault();
    const row = btn.closest<HTMLElement>(".row");
    const id = row?.dataset.id;
    if (!id) return;
    btn.setAttribute("aria-pressed", String(toggleFav(id)));
  });
}
