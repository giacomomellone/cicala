// Contribute controller (spec §7.7): live validation mirroring
// tools/validate.py (10–140 chars, ends with "?", single line), then a
// prefilled GitHub issue-form URL — the whole flow is zero-backend.

import { REPO_URL } from "../config";
import { tr } from "./apply-i18n";
import { detectLang, loadRecent, siteConfig } from "./data";
import { questionTextIssue, TEXT_MAX } from "./rules";

export async function initContribute(): Promise<void> {
  const form = document.getElementById("c-form") as HTMLFormElement | null;
  if (!form) return;
  const langSel = document.getElementById("c-lang") as HTMLSelectElement;
  const catSel = document.getElementById("c-cat") as HTMLSelectElement;
  const textEl = document.getElementById("c-text") as HTMLTextAreaElement;
  const counter = document.getElementById("c-counter")!;
  const ruleHint = document.getElementById("c-rule")!;
  const nameEl = document.getElementById("c-name") as HTMLInputElement;
  const cc0El = document.getElementById("c-cc0") as HTMLInputElement;
  const submit = document.getElementById("c-submit") as HTMLButtonElement;
  const newlangNote = document.getElementById("c-newlang")!;
  const recentList = document.getElementById("c-recent")!;
  const recentEmpty = document.getElementById("c-recent-empty")!;

  const lang = detectLang();
  const cfg = siteConfig();

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
    const n = textEl.value.length;
    const issue = questionTextIssue(textEl.value);
    counter.textContent = `${n}/${TEXT_MAX}`;
    counter.classList.toggle("is-bad", n > 0 && (issue === "short" || issue === "long"));

    let rule = "";
    if (n === 0) rule = tr(lang, "contribute.question.hint");
    else if (issue === "short") rule = tr(lang, "contribute.rule.short");
    else if (issue === "long") rule = tr(lang, "contribute.rule.long");
    else if (issue === "mark") rule = tr(lang, "contribute.rule.mark");
    else rule = tr(lang, "contribute.rule.ok");
    ruleHint.textContent = rule;
    const textOk = issue === null;
    ruleHint.classList.toggle("is-bad", n > 0 && !textOk);

    const isOther = langSel.value === "__other__";
    newlangNote.hidden = !isOther;

    const ok = textOk && cc0El.checked && !isOther;
    submit.disabled = !ok;
    return ok;
  }

  textEl.addEventListener("input", validate);
  cc0El.addEventListener("change", validate);
  langSel.addEventListener("change", validate);
  validate();

  form.addEventListener("submit", (e) => {
    e.preventDefault();
    if (!validate()) return;
    const chosen = cfg.langs.find((l) => l.code === langSel.value);
    if (!chosen) return;
    const tags = Array.from(
      form.querySelectorAll<HTMLInputElement>("input[name=tags]:checked"),
    ).map((t) => t.value);
    const params = new URLSearchParams({
      template: "new-question.yml",
      labels: "question-submission",
      language: `${chosen.name} (${chosen.code})`,
      category: catSel.value,
      "question-text": textEl.value.replace(/\s+/g, " ").trim(),
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
            `<div class="row-meta"><span>${tr(lang, `cat.${r.category}` as never)}</span>` +
            `<span>${r.added}</span></div></div></li>`,
        )
        .join("");
    }
  } catch {
    /* recent list is decoration — never block the form on it */
  }
}
