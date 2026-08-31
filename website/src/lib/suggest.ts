import { tr } from "./apply-i18n";
import { detectLang, loadPayload, siteConfig } from "./data";
import { buildTextIndex, collapse, duplicateOf, questionTextIssue, TEXT_MAX } from "./rules";
import { CC0_CONSENT_VERSION, isSuggestionSource, type SuggestionSource } from "./submission";

interface SuggestionConfig {
  available: boolean;
  turnstileRequired: boolean;
  turnstileSiteKey: string;
  turnstileAction: string;
}

interface TurnstileApi {
  render(
    target: HTMLElement,
    options: {
      sitekey: string;
      action: string;
      appearance: "interaction-only";
      size: "flexible";
      callback: (token: string) => void;
      "expired-callback": () => void;
      "error-callback": () => void;
    },
  ): string;
  reset(widget: string): void;
}

declare global {
  interface Window {
    turnstile?: TurnstileApi;
  }
}

function loadTurnstile(): Promise<TurnstileApi> {
  if (window.turnstile) return Promise.resolve(window.turnstile);
  return new Promise((resolve, reject) => {
    const existing = document.querySelector<HTMLScriptElement>("script[data-cicala-turnstile]");
    const script = existing ?? document.createElement("script");
    const ready = () =>
      window.turnstile ? resolve(window.turnstile) : reject(new Error("missing API"));
    script.addEventListener("load", ready, { once: true });
    script.addEventListener("error", () => reject(new Error("failed to load")), { once: true });
    if (!existing) {
      script.src = "https://challenges.cloudflare.com/turnstile/v0/api.js?render=explicit";
      script.async = true;
      script.defer = true;
      script.dataset.cicalaTurnstile = "";
      document.head.append(script);
    }
  });
}

export async function initSuggest(): Promise<void> {
  const form = document.getElementById("s-form") as HTMLFormElement | null;
  if (!form) return;
  const lang = detectLang();
  const cfg = siteConfig();
  const langEl = document.getElementById("s-lang") as HTMLSelectElement;
  const textEl = document.getElementById("s-text") as HTMLTextAreaElement;
  const nameEl = document.getElementById("s-name") as HTMLInputElement;
  const websiteEl = document.getElementById("s-website") as HTMLInputElement;
  const cc0El = document.getElementById("s-cc0") as HTMLInputElement;
  const ruleEl = document.getElementById("s-rule")!;
  const counterEl = document.getElementById("s-counter")!;
  const dupeEl = document.getElementById("s-dupe")!;
  const dupeLink = document.getElementById("s-dupe-link") as HTMLAnchorElement;
  const submitEl = document.getElementById("s-submit") as HTMLButtonElement;
  const statusEl = document.getElementById("s-status")!;
  const turnstileEl = document.getElementById("s-turnstile")!;
  const receivedEl = document.getElementById("s-received")!;
  const receivedQuestion = document.getElementById("s-received-question")!;
  const referenceEl = document.getElementById("s-reference")!;
  const againEl = document.getElementById("s-again") as HTMLButtonElement;

  for (const item of cfg.langs) {
    const option = document.createElement("option");
    option.value = item.code;
    option.textContent = `${item.name} (${item.code})`;
    option.selected = item.code === lang;
    langEl.append(option);
  }

  const params = new URLSearchParams(location.search);
  const sourceParam = params.get("source") ?? "suggest";
  const source: SuggestionSource = isSuggestionSource(sourceParam) ? sourceParam : "suggest";
  const prefill = params.get("text")?.slice(0, 200) ?? "";
  if (prefill) textEl.value = prefill;

  let available = false;
  let sending = false;
  let token = "";
  let turnstileRequired = true;
  let widget = "";
  let index: Map<string, string> | null = null;

  function textState(): { ok: boolean; question: string } {
    const question = collapse(textEl.value);
    const issue = questionTextIssue(textEl.value);
    const duplicate = issue === null && index ? duplicateOf(question, index) : null;
    counterEl.textContent = `${question.length}/${TEXT_MAX}`;
    counterEl.classList.toggle(
      "is-bad",
      question.length > 0 && (issue === "short" || issue === "long"),
    );
    dupeEl.hidden = duplicate === null;
    if (duplicate) dupeLink.href = `/q/${duplicate}`;

    const key =
      question.length === 0
        ? "suggest.question.hint"
        : issue === "short"
          ? "suggest.rule.short"
          : issue === "long"
            ? "suggest.rule.long"
            : issue === "multiline"
              ? "suggest.rule.multiline"
              : issue === "mark"
                ? "suggest.rule.mark"
                : duplicate
                  ? "suggest.rule.duplicate"
                  : "suggest.rule.ok";
    ruleEl.textContent = tr(lang, key);
    const ok = issue === null && duplicate === null;
    ruleEl.classList.toggle("is-bad", question.length > 0 && !ok);
    return { ok, question };
  }

  function validate(): boolean {
    const { ok } = textState();
    const ready =
      ok && index !== null && cc0El.checked && available && (!turnstileRequired || Boolean(token));
    submitEl.disabled = sending || !ready;
    return ready;
  }

  textEl.addEventListener("input", validate);
  cc0El.addEventListener("change", validate);
  langEl.addEventListener("change", async () => {
    index = null;
    validate();
    try {
      index = buildTextIndex((await loadPayload(langEl.value)).questions);
      statusEl.textContent = "";
    } catch {
      statusEl.textContent = tr(lang, "suggest.unavailable");
    }
    validate();
  });
  validate();

  try {
    const [payload, configResponse] = await Promise.all([
      loadPayload(langEl.value),
      fetch("/api/suggestions", { headers: { Accept: "application/json" } }),
    ]);
    index = buildTextIndex(payload.questions);
    if (!configResponse.ok) throw new Error("configuration unavailable");
    const config = (await configResponse.json()) as SuggestionConfig;
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
    statusEl.textContent = tr(lang, "suggest.unavailable");
    validate();
  }

  form.addEventListener("submit", async (event) => {
    event.preventDefault();
    if (!validate()) return;
    const { question } = textState();
    sending = true;
    submitEl.textContent = tr(lang, "suggest.sending");
    statusEl.textContent = "";
    validate();
    try {
      const response = await fetch("/api/suggestions", {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({
          question,
          language: langEl.value,
          credit: nameEl.value,
          cc0: cc0El.checked,
          consentVersion: CC0_CONSENT_VERSION,
          submissionId: crypto.randomUUID(),
          source,
          turnstileToken: token,
          website: websiteEl.value,
        }),
      });
      const result = (await response.json().catch(() => ({}))) as {
        accepted?: boolean;
        reference?: number;
      };
      if (!response.ok || !result.accepted) throw new Error("submission failed");

      receivedQuestion.textContent = question;
      referenceEl.textContent = result.reference
        ? `${tr(lang, "suggest.reference")} #${result.reference}`
        : "";
      form.hidden = true;
      receivedEl.hidden = false;
      const behavior = matchMedia("(prefers-reduced-motion: reduce)").matches ? "auto" : "smooth";
      receivedEl.scrollIntoView({ behavior, block: "start" });
    } catch {
      statusEl.textContent = tr(lang, "suggest.failed");
      token = turnstileRequired ? "" : "test-verification";
      if (turnstileRequired && widget && window.turnstile) window.turnstile.reset(widget);
    } finally {
      sending = false;
      submitEl.textContent = tr(lang, "suggest.submit");
      validate();
    }
  });

  againEl.addEventListener("click", () => {
    form.reset();
    langEl.value = lang;
    receivedEl.hidden = true;
    form.hidden = false;
    receivedQuestion.textContent = "";
    referenceEl.textContent = "";
    token = turnstileRequired ? "" : "test-verification";
    if (turnstileRequired && widget && window.turnstile) window.turnstile.reset(widget);
    validate();
    textEl.focus();
  });
}
