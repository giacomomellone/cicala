import type { Lang } from "../i18n";
import { isLang, tr } from "./apply-i18n";
import { detectLang, findQuestion, loadIndex, loadPayload, type Question } from "./data";
import {
  EDIT_ASPECTS,
  type EditAspect,
  isQuestionId,
  normalizeReason,
  REASON_MAX,
  REASON_MIN,
} from "./edit";
import {
  buildTextIndex,
  collapse,
  duplicateOf,
  normalizeQuestion,
  questionTextIssue,
  TEXT_MAX,
} from "./rules";
import { styleHint } from "./style-hints";
import { CC0_CONSENT_VERSION } from "./submission";
import { loadTurnstile } from "./turnstile-widget";

interface EditConfig {
  available: boolean;
  turnstileRequired: boolean;
  turnstileSiteKey: string;
  turnstileAction: string;
}

interface WordingState {
  proposed: string;
  /* Empty wording is legal — it means a metadata-only request — so "not
     blocking" and "contributes text" are two different questions. */
  blocking: boolean;
  contributes: boolean;
}

export async function initEditForm(): Promise<void> {
  const maybeForm = document.getElementById("e-form") as HTMLFormElement | null;
  if (!maybeForm) return;
  const form = maybeForm;

  const lang = detectLang();
  const uiLang: Lang = isLang(lang) ? lang : "en";

  const notFoundEl = document.getElementById("e-notfound")!;
  const currentEl = document.getElementById("e-current")!;
  const metaEl = document.getElementById("e-meta")!;
  const textEl = document.getElementById("e-text") as HTMLTextAreaElement;
  const ruleEl = document.getElementById("e-rule")!;
  const counterEl = document.getElementById("e-counter")!;
  const styleEl = document.getElementById("e-style")!;
  const dupeEl = document.getElementById("e-dupe")!;
  const dupeLink = document.getElementById("e-dupe-link") as HTMLAnchorElement;
  const reasonEl = document.getElementById("e-reason") as HTMLTextAreaElement;
  const reasonRuleEl = document.getElementById("e-reason-rule")!;
  const reasonCounterEl = document.getElementById("e-reason-counter")!;
  const consentEl = document.getElementById("e-consent")!;
  const humanEl = document.getElementById("e-human") as HTMLInputElement;
  const cc0El = document.getElementById("e-cc0") as HTMLInputElement;
  const websiteEl = document.getElementById("e-website") as HTMLInputElement;
  const nothingEl = document.getElementById("e-nothing")!;
  const turnstileEl = document.getElementById("e-turnstile")!;
  const submitEl = document.getElementById("e-submit") as HTMLButtonElement;
  const statusEl = document.getElementById("e-status")!;
  const receivedEl = document.getElementById("e-received")!;
  const referenceEl = document.getElementById("e-reference")!;
  const aspectEls = new Map<EditAspect, HTMLInputElement>(
    EDIT_ASPECTS.map((aspect) => [
      aspect,
      document.getElementById(`e-aspect-${aspect}`) as HTMLInputElement,
    ]),
  );

  const questionId = (new URLSearchParams(location.search).get("q") ?? "").trim();

  function unavailableQuestion(): void {
    form.hidden = true;
    notFoundEl.hidden = false;
  }

  if (!isQuestionId(questionId)) {
    unavailableQuestion();
    return;
  }

  let current: Question | null = null;
  let questionLang = "";
  let textIndex: Map<string, string> | null = null;
  try {
    questionLang = (await loadIndex())[questionId] ?? "";
    if (!questionLang) throw new Error("unknown question");
    const payload = await loadPayload(questionLang);
    current = findQuestion(payload, questionId);
    if (!current) throw new Error("unknown question");
    textIndex = buildTextIndex(payload.questions);
  } catch {
    unavailableQuestion();
    return;
  }

  currentEl.textContent = current.text;
  metaEl.textContent = `${questionId} · ${questionLang}`;

  let available = false;
  let sending = false;
  let token = "";
  let turnstileRequired = true;
  let widget = "";

  function wordingState(): WordingState {
    const proposed = collapse(textEl.value);
    counterEl.textContent = `${proposed.length}/${TEXT_MAX}`;

    if (!proposed) {
      counterEl.classList.remove("is-bad");
      ruleEl.textContent = tr(lang, "edit.wording.hint");
      ruleEl.classList.remove("is-bad");
      styleEl.hidden = true;
      dupeEl.hidden = true;
      return { proposed: "", blocking: false, contributes: false };
    }

    const issue = questionTextIssue(textEl.value);
    const same = normalizeQuestion(proposed) === normalizeQuestion(current!.text);
    const duplicate =
      issue === null && !same && textIndex ? duplicateOf(proposed, textIndex) : null;

    counterEl.classList.toggle("is-bad", issue === "short" || issue === "long");
    dupeEl.hidden = duplicate === null;
    if (duplicate) dupeLink.href = `/q/${duplicate}`;

    const key =
      issue === "short"
        ? "suggest.rule.short"
        : issue === "long"
          ? "suggest.rule.long"
          : issue === "multiline"
            ? "suggest.rule.multiline"
            : issue === "mark"
              ? "suggest.rule.mark"
              : same
                ? "edit.same"
                : duplicate
                  ? "suggest.rule.duplicate"
                  : "suggest.rule.ok";
    ruleEl.textContent = tr(lang, key);

    const ok = issue === null && !same && duplicate === null;
    ruleEl.classList.toggle("is-bad", !ok);

    /* Editorial hints read the question's language, not the interface
       language, and are advisory: they never block submission. */
    const hintLang: Lang = isLang(questionLang) ? questionLang : uiLang;
    const hint = ok ? styleHint(proposed, hintLang) : null;
    styleEl.hidden = hint === null;
    if (hint) styleEl.textContent = tr(lang, `suggest.style.${hint}`);

    return { proposed, blocking: !ok, contributes: ok };
  }

  function reasonOk(): boolean {
    const reason = normalizeReason(reasonEl.value);
    reasonCounterEl.textContent = `${reason.length}/${REASON_MAX}`;
    const short = reason.length > 0 && reason.length < REASON_MIN;
    const long = reason.length > REASON_MAX;
    reasonCounterEl.classList.toggle("is-bad", short || long);
    reasonRuleEl.textContent = tr(
      lang,
      short ? "edit.reason.short" : long ? "edit.reason.long" : "edit.reason.hint",
    );
    reasonRuleEl.classList.toggle("is-bad", short || long);
    return reason.length >= REASON_MIN && reason.length <= REASON_MAX;
  }

  function checkedAspects(): EditAspect[] {
    return EDIT_ASPECTS.filter((aspect) => aspectEls.get(aspect)!.checked);
  }

  function validate(): boolean {
    const wording = wordingState();
    const aspects = checkedAspects();
    const reason = reasonOk();

    /* Consent is asked for only when wording is contributed; a metadata-only
       request donates no text to dedicate. */
    consentEl.hidden = !wording.contributes;
    const consented = !wording.contributes || (humanEl.checked && cc0El.checked);

    const proposesSomething = wording.contributes || aspects.length > 0;
    nothingEl.hidden = wording.blocking || proposesSomething;

    const ready =
      !wording.blocking &&
      proposesSomething &&
      reason &&
      consented &&
      available &&
      (!turnstileRequired || Boolean(token));
    submitEl.disabled = sending || !ready;
    return ready;
  }

  textEl.addEventListener("input", validate);
  reasonEl.addEventListener("input", validate);
  humanEl.addEventListener("change", validate);
  cc0El.addEventListener("change", validate);
  for (const box of aspectEls.values()) box.addEventListener("change", validate);
  validate();

  try {
    const response = await fetch("/api/edits", { headers: { Accept: "application/json" } });
    if (!response.ok) throw new Error("configuration unavailable");
    const config = (await response.json()) as EditConfig;
    if (!config.available) throw new Error("configuration unavailable");
    turnstileRequired = config.turnstileRequired;
    if (turnstileRequired) {
      const turnstile = await loadTurnstile();
      widget = turnstile.render(turnstileEl, {
        sitekey: config.turnstileSiteKey,
        action: config.turnstileAction,
        appearance: "interaction-only",
        size: "flexible",
        callback: (value) => {
          token = value;
          statusEl.textContent = "";
          validate();
        },
        "expired-callback": () => {
          token = "";
          validate();
        },
        "error-callback": () => {
          token = "";
          statusEl.textContent = tr(lang, "suggest.verification.error");
          validate();
        },
      });
    } else {
      token = "test-verification";
    }
    available = true;
    statusEl.textContent = "";
    validate();
  } catch {
    available = false;
    statusEl.textContent = tr(lang, "edit.unavailable");
    validate();
  }

  form.addEventListener("submit", async (event) => {
    event.preventDefault();
    if (!validate()) return;
    const wording = wordingState();
    sending = true;
    submitEl.textContent = tr(lang, "suggest.sending");
    statusEl.textContent = "";
    validate();
    try {
      const response = await fetch("/api/edits", {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({
          questionId,
          language: questionLang,
          proposedText: wording.contributes ? wording.proposed : "",
          aspects: checkedAspects(),
          reason: normalizeReason(reasonEl.value),
          humanWritten: wording.contributes ? humanEl.checked : false,
          cc0: wording.contributes ? cc0El.checked : false,
          consentVersion: CC0_CONSENT_VERSION,
          submissionId: crypto.randomUUID(),
          turnstileToken: token,
          website: websiteEl.value,
        }),
      });
      const result = (await response.json().catch(() => ({}))) as {
        accepted?: boolean;
        reference?: number;
      };
      if (!response.ok || !result.accepted) throw new Error("submission failed");

      referenceEl.textContent = result.reference
        ? `${tr(lang, "suggest.reference")} #${result.reference}`
        : "";
      form.hidden = true;
      receivedEl.hidden = false;
      const behavior = matchMedia("(prefers-reduced-motion: reduce)").matches ? "auto" : "smooth";
      receivedEl.scrollIntoView({ behavior, block: "start" });
    } catch {
      statusEl.textContent = tr(lang, "edit.failed");
      token = turnstileRequired ? "" : "test-verification";
      if (turnstileRequired && widget && window.turnstile) window.turnstile.reset(widget);
    } finally {
      sending = false;
      submitEl.textContent = tr(lang, "edit.submit");
      validate();
    }
  });
}
