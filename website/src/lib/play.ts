// Filters / Next is the complete tabletop interaction, shared with the device.
import { BAG_TEXTURE, type Permissions } from "../config";
import { tr } from "./apply-i18n";
import { drawFromBag, permitted } from "./bag";
import { detectLang, loadPayload, type Payload } from "./data";
import { getPlaySession, setPlaySession } from "./play-session";
import { isFav, toggleFav } from "./store";

const FILTERS = ["dark", "sexual", "heavy"] as const;
export async function initPlay(): Promise<void> {
  const root = document.getElementById("play-root");
  if (!root) return;
  const el = (id: string) => document.getElementById(id)!;
  const qText = el("q-text"),
    qSwap = el("q-swap"),
    qMeta = el("q-meta");
  const menu = el("q-menu"),
    summary = el("q-permissions");
  const origin = el("q-origin") as HTMLAnchorElement;
  const fav = el("q-fav") as HTMLButtonElement;
  const share = el("q-share") as HTMLButtonElement;
  const next = el("q-next") as HTMLButtonElement;
  const filters = el("q-filters") as HTMLButtonElement;
  const lang = detectLang();
  let payload: Payload;
  try {
    payload = await loadPayload(lang);
  } catch {
    return;
  }
  const byId = new Map(payload.questions.map((q) => [q.id, q]));
  const state = getPlaySession(lang, payload.version);
  let direct = root.dataset.permalink === "1";
  if (direct) {
    state.menu = false;
    state.shown = root.dataset.seedId ?? "";
    state.bag.last = state.shown;
    state.bag.r = [...state.bag.r, state.shown].slice(-20);
  }
  const label = (key: string) => tr(lang, key as never);
  function draw(): void {
    const allowed = payload.questions
      .filter((q) => permitted(q, state.permissions))
      .map((q) => q.id);
    state.shown =
      drawFromBag(
        state.bag,
        allowed,
        Math.random,
        BAG_TEXTURE
          ? {
              seed: byId.get(state.bag.last),
              meta: (id) => byId.get(id),
            }
          : undefined,
      ) ?? "";
  }
  function render(): void {
    const q = byId.get(state.shown);
    qText.hidden = state.menu;
    menu.hidden = !state.menu;
    qMeta.hidden = state.menu || !q;
    const edit = el("q-edit");
    if (edit) edit.hidden = state.menu;
    origin.hidden = state.menu || !q?.origin || q.translated_by !== "google";
    if (q?.origin) origin.href = `/q/${q.origin}`;
    qText.textContent = q?.text ?? label("play.empty");
    fav.setAttribute("aria-pressed", String(!!q && isFav(q.id)));
    el("q-fav-label").textContent = label(q && isFav(q.id) ? "play.saved" : "play.save");
    summary.textContent = FILTERS.map(
      (key) => `${label(`filter.${key}`)} ${state.permissions[key] ? "+" : "−"}`,
    ).join(" · ");
    summary.setAttribute(
      "aria-label",
      FILTERS.map(
        (key) =>
          `${label(`filter.${key}`)}: ${label(state.permissions[key] ? "filter.allow" : "filter.exclude")}`,
      ).join(", "),
    );
    filters.setAttribute("aria-expanded", String(state.menu));
    for (let i = 0; i < 4; i++) {
      const row = el(`q-filter-${i}`);
      row.classList.toggle("is-selected", i === state.cursor);
      row.setAttribute("aria-current", String(i === state.cursor));
      const key = FILTERS[i];
      row.setAttribute("aria-label", label(key ? `filter.${key}` : "filter.done"));
      if (key) {
        row.setAttribute("role", "switch");
        row.setAttribute("aria-checked", String(state.draft[key]));
      }
      row.textContent = `${i === state.cursor ? "› " : ""}${key ? `${label(`filter.${key}`)} — ${label(state.draft[key] ? "filter.allow" : "filter.exclude")}` : label("filter.done")}`;
    }
    document.documentElement.lang = lang;
    setPlaySession(lang, state);
  }
  function leaveDirect(): void {
    direct = false;
    if (location.pathname.startsWith("/q/")) window.history.replaceState({}, "", "/");
    document.getElementById("q-edit")?.remove();
  }
  let busy = false;
  async function press(which: "filters" | "next", row?: number): Promise<void> {
    if (busy || (row !== undefined && !state.menu)) return;
    busy = true;
    try {
      if (row === undefined && !matchMedia("(prefers-reduced-motion: reduce)").matches) {
        qSwap.classList.add("is-fading");
        await new Promise((r) => setTimeout(r, 120));
      }
      if (which === "filters") {
        if (!state.menu) {
          state.menu = true;
          state.draft = { ...state.permissions };
          state.cursor = 0;
        } else state.cursor = (state.cursor + 1) % 4;
      } else if (state.menu) {
        if (row !== undefined) state.cursor = row;
        const key: keyof Permissions | undefined = FILTERS[state.cursor];
        if (key) state.draft[key] = !state.draft[key];
        else {
          state.permissions = { ...state.draft };
          state.menu = false;
          const current = byId.get(state.shown);
          if (!current || !permitted(current, state.permissions)) {
            leaveDirect();
            draw();
          }
        }
      } else {
        leaveDirect();
        draw();
      }
      render();
      if (row !== undefined && !state.menu) filters.focus();
    } finally {
      qSwap.classList.remove("is-fading");
      busy = false;
    }
  }
  if (!direct && !state.menu) {
    const current = byId.get(state.shown);
    if (!current || !permitted(current, state.permissions)) draw();
  }
  render();
  filters.addEventListener("click", () => void press("filters"));
  next.addEventListener("click", () => void press("next"));
  for (let i = 0; i < 4; i++)
    el(`q-filter-${i}`).addEventListener("click", () => void press("next", i));
  fav.addEventListener("click", () => {
    if (state.menu || !state.shown || busy) return;
    toggleFav(state.shown);
    render();
  });
  let shareTimer: ReturnType<typeof setTimeout> | undefined;
  share.addEventListener("click", async () => {
    if (state.menu || !state.shown || busy) return;
    try {
      await navigator.clipboard.writeText(`${location.origin}/q/${state.shown}`);
      el("q-share-label").textContent = label("play.copied");
      clearTimeout(shareTimer);
      shareTimer = setTimeout(() => {
        el("q-share-label").textContent = label("play.share");
      }, 1500);
    } catch {
      /* Clipboard access is optional. */
    }
  });
  document.addEventListener("keydown", (e) => {
    if (e.repeat || e.ctrlKey || e.metaKey || e.altKey) return;
    if (
      (e.target as HTMLElement | null)?.closest(
        "input, textarea, select, button, a, [contenteditable]",
      )
    )
      return;
    if (e.key === " " || e.key === "ArrowRight") {
      e.preventDefault();
      void press("next");
    } else if (e.key.toLowerCase() === "f") {
      e.preventDefault();
      void press("filters");
    }
  });
}
