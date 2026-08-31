import { collapse, questionTextIssue } from "./rules";

export const CC0_CONSENT_VERSION = "cc0-1.0-2026-08-31";
export const SUGGESTION_SOURCES = ["suggest", "browse", "browse-empty", "play"] as const;

export type SuggestionSource = (typeof SUGGESTION_SOURCES)[number];

export interface SuggestionDraft {
  question: string;
  language: string;
  credit: string;
  cc0: boolean;
  consentVersion: string;
  submissionId: string;
  source: SuggestionSource;
  turnstileToken: string;
  website: string;
}

export interface ValidSuggestion {
  question: string;
  language: string;
  credit: string;
  submissionId: string;
  source: SuggestionSource;
  turnstileToken: string;
  website: string;
}

export type SuggestionValidation =
  { ok: true; suggestion: ValidSuggestion } | { ok: false; error: string };

const SUBMISSION_ID = /^[0-9a-f]{8}-[0-9a-f]{4}-4[0-9a-f]{3}-[89ab][0-9a-f]{3}-[0-9a-f]{12}$/i;

export function isSuggestionSource(value: string): value is SuggestionSource {
  return (SUGGESTION_SOURCES as readonly string[]).includes(value);
}

export function validateSuggestion(
  value: unknown,
  languages: readonly string[],
): SuggestionValidation {
  if (!value || typeof value !== "object") return { ok: false, error: "invalid request" };
  const draft = value as Partial<SuggestionDraft>;
  const question = typeof draft.question === "string" ? collapse(draft.question) : "";
  if (questionTextIssue(question) !== null) return { ok: false, error: "invalid question" };

  const language = typeof draft.language === "string" ? draft.language : "";
  if (!languages.includes(language)) return { ok: false, error: "invalid language" };

  const credit = typeof draft.credit === "string" ? collapse(draft.credit) : "";
  if (credit.length > 40) return { ok: false, error: "credit is too long" };
  if (draft.cc0 !== true || draft.consentVersion !== CC0_CONSENT_VERSION)
    return { ok: false, error: "CC0 consent is required" };

  const submissionId = typeof draft.submissionId === "string" ? draft.submissionId : "";
  if (!SUBMISSION_ID.test(submissionId)) return { ok: false, error: "invalid submission id" };

  const source = typeof draft.source === "string" ? draft.source : "";
  if (!isSuggestionSource(source)) return { ok: false, error: "invalid submission source" };

  const turnstileToken =
    typeof draft.turnstileToken === "string" ? draft.turnstileToken.slice(0, 2048) : "";
  if (!turnstileToken) return { ok: false, error: "verification is required" };

  return {
    ok: true,
    suggestion: {
      question,
      language,
      credit,
      submissionId,
      source,
      turnstileToken,
      website: typeof draft.website === "string" ? draft.website.slice(0, 200) : "",
    },
  };
}
