// Contribute controller (spec §7.7): live validation mirroring
// tools/validate.py (10–140 chars, ends with "?", single line), then a
// prefilled GitHub issue-form URL — the whole flow is zero-backend.

import { DEPTH_OPTIONS, REPO_URL } from "../config";
import { tr } from "./apply-i18n";
import { detectLang, loadPayload, loadRecent, siteConfig } from "./data";
import {
  buildTextIndex,
  collapse,
  duplicateOf,
  questionTextIssue,
  TEXT_MAX,
} from "./rules";

export async function initContribute(): Promise<void> {
  const form = document.getElementById("c-form") as HTMLFormElement | null;
  if (!form) return;
  const langSel = document.getElementById("c-lang") as HTMLSelectElement;
  const depthSel = document.getElementById("c-depth") as HTMLSelectElement;
  const deckRule = document.getElementById("c-deck-rule")!;
  const textEl = document.getElementById("c-text") as HTMLTextAreaElement;
  const counter = document.getElementById("c-counter")!;
  const ruleHint = document.getElementById("c-rule")!;
  const nameEl = document.getElementById("c-name") as HTMLInputElement;
  const cc0El = document.getElementById("c-cc0") as HTMLInputElement;
  const submit = document.getElementById("c-submit") as HTMLButtonElement;
  const newlangNote = document.getElementById("c-newlang")!;
  const dupeNote = document.getElementById("c-dupe")!;
  const dupeLink = document.getElementById("c-dupe-link") as HTMLAnchorElement;
  const recentList = document.getElementById("c-recent")!;
  const recentEmpty = document.getElementById("c-recent-empty")!;

  const lang = detectLang();
  const cfg = siteConfig();

  // Normalized text -> id, per language. A submission that duplicates an
  // existing question is rejected by the validator at the very end of the
  // pipeline; catching it here saves the whole round trip. Languages are
  // independent corpora, so each is indexed separately and only the selected
  // one is consulted.
  const indexes = new Map<string, Map<string, string>>();
  const indexing = new Set<string>();

  function indexFor(code: string): Map<string, string> | null {
    const ready = indexes.get(code);
    if (ready) return ready;
    if (!indexing.has(code)) {
      indexing.add(code);
      // The corpus arrives after first paint; re-validate once it lands so a
      // question typed in the meantime is still checked.
      loadPayload(code)
        .then((payload) => {
          indexes.set(code, buildTextIndex(payload.questions));
          validate();
        })
        .catch(() => indexing.delete(code));
    }
    return null;
  }

  // language select: shipped languages + "another language…"
  langSel.innerHTML = "";
  for (const l of cfg.langs) {
    const opt = document.createElement("option");
    opt.value = l.code;
    opt.textContent = `${l.name} (${l.code})`;
    if (l.code === lang) opt.selected = true;
    langSel.append(opt);
  }
  const other = document.createElement("option");
  other.value = "__other__";
  other.textContent = tr(lang, "contribute.lang.other");
  langSel.append(other);

  function validate(): boolean {
    // The counter measures what would be stored, so it agrees with the rule
    // beside it at the boundary.
    const n = collapse(textEl.value).length;
    const issue = questionTextIssue(textEl.value);
    counter.textContent = `${n}/${TEXT_MAX}`;
    counter.classList.toggle("is-bad", n > 0 && (issue === "short" || issue === "long"));

    const isOther = langSel.value === "__other__";
    const index = isOther ? null : indexFor(langSel.value);
    const dupeId = issue === null && index ? duplicateOf(textEl.value, index) : null;
    dupeNote.hidden = dupeId === null;
    if (dupeId) dupeLink.href = `/q/${dupeId}`;

    let rule = "";
    if (n === 0) rule = tr(lang, "contribute.question.hint");
    else if (issue === "short") rule = tr(lang, "contribute.rule.short");
    else if (issue === "long") rule = tr(lang, "contribute.rule.long");
    else if (issue === "multiline") rule = tr(lang, "contribute.rule.multiline");
    else if (issue === "mark") rule = tr(lang, "contribute.rule.mark");
    else if (dupeId) rule = tr(lang, "contribute.rule.duplicate");
    else rule = tr(lang, "contribute.rule.ok");
    ruleHint.textContent = rule;
    const textOk = issue === null && dupeId === null;
    ruleHint.classList.toggle("is-bad", n > 0 && !textOk);

    newlangNote.hidden = !isOther;

    const decks = Array.from(
      form.querySelectorAll<HTMLInputElement>("input[name=decks]:checked"),
    ).map((input) => input.value);
    const toneTags = Array.from(
      form.querySelectorAll<HTMLInputElement>("input[name=tags]:checked"),
    ).some((input) => input.value === "dark" || input.value === "spicy");
    const decksOk = decks.length > 0 &&
      (!toneTags || (decks.length === 1 && decks[0] === "wild"));
    deckRule.textContent = tr(
      lang,
      decks.length === 0
        ? "contribute.decks.required"
        : decksOk
          ? "contribute.decks.hint"
          : "contribute.decks.wild",
    );
    deckRule.classList.toggle("is-bad", !decksOk);

    const ok = textOk && decksOk && cc0El.checked && !isOther;
    submit.disabled = !ok;
    return ok;
  }

  textEl.addEventListener("input", validate);
  cc0El.addEventListener("change", validate);
  langSel.addEventListener("change", validate);
  form.addEventListener("change", validate);
  validate();

  form.addEventListener("submit", (e) => {
    e.preventDefault();
    if (!validate()) return;
    const chosen = cfg.langs.find((l) => l.code === langSel.value);
    if (!chosen) return;
    const tags = Array.from(
      form.querySelectorAll<HTMLInputElement>("input[name=tags]:checked"),
    ).map((t) => t.value);
    const decks = Array.from(
      form.querySelectorAll<HTMLInputElement>("input[name=decks]:checked"),
    ).map((input) => input.value);
    const depthOption =
      DEPTH_OPTIONS[Number(depthSel.value) - 1] ?? DEPTH_OPTIONS[1];
    const params = new URLSearchParams({
      template: "new-question.yml",
      labels: "question-submission",
      language: `${chosen.name} (${chosen.code})`,
      decks: decks.join(","),
      depth: depthOption,
      "question-text": collapse(textEl.value),
    });
    if (tags.length) params.set("tags", tags.join(","));
    const credit = nameEl.value.trim().slice(0, 40);
    if (credit) params.set("credit", credit);
    window.open(
      `${REPO_URL}/issues/new?${params.toString()}`,
      "_blank",
      "noopener",
    );
  });

  // recently added — the social-proof loop. SSR shows the English list;
  // re-render for the active language.
  try {
    const recent = await loadRecent(lang);
    if (recent.length === 0) {
      recentList.innerHTML = "";
      recentEmpty.hidden = false;
    } else if (lang !== "en") {
      recentList.innerHTML = recent
        .map(
          (r) =>
            `<li class="row"><div class="row-main">` +
            `<a class="row-q" href="/q/${r.id}">${r.text
              .replace(/&/g, "&amp;")
              .replace(/</g, "&lt;")}</a>` +
            `<div class="row-meta">${r.decks
              .map((deck) => `<span>${tr(lang, `deck.${deck}` as never)}</span>`)
              .join("")}` +
            `<span>${tr(lang, `depth.${r.depth}` as never)}</span>` +
            `<span>${r.added}</span></div></div></li>`,
        )
        .join("");
    }
  } catch {
    /* recent list is decoration — never block the form on it */
  }
}
