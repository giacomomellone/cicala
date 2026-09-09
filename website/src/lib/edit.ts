import { collapse, questionTextIssue } from "./rules";
import { CC0_CONSENT_VERSION } from "./submission";

export const REASON_MIN = 10;
export const REASON_MAX = 1000;

/* The three editorial properties a maintainer can be asked to reconsider.
   They mirror the checkbox options in .github/ISSUE_TEMPLATE/edit-question.yml
   so both routes produce one issue shape. */
export const EDIT_ASPECTS = ["depth", "tags", "translation"] as const;

export type EditAspect = (typeof EDIT_ASPECTS)[number];

export const EDIT_ASPECT_LABELS: Record<EditAspect, string> = {
  depth: "Depth",
  tags: "Tags",
  translation: "Translation provenance or origin",
};

export interface EditDraft {
  questionId: string;
  language: string;
  proposedText: string;
  aspects: EditAspect[];
  reason: string;
  humanWritten: boolean;
  cc0: boolean;
  consentVersion: string;
  submissionId: string;
  turnstileToken: string;
  website: string;
}

export interface ValidEdit {
  questionId: string;
  language: string;
  proposedText: string;
  aspects: EditAspect[];
  reason: string;
  submissionId: string;
  turnstileToken: string;
  website: string;
}

export type EditValidation = { ok: true; edit: ValidEdit } | { ok: false; error: string };

const QUESTION_ID = /^q-[0-9a-f]{8}$/;
const SUBMISSION_ID = /^[0-9a-f]{8}-[0-9a-f]{4}-4[0-9a-f]{3}-[89ab][0-9a-f]{3}-[0-9a-f]{12}$/i;

export function isEditAspect(value: string): value is EditAspect {
  return (EDIT_ASPECTS as readonly string[]).includes(value);
}

export function isQuestionId(value: string): boolean {
  return QUESTION_ID.test(value);
}

/* Keeps paragraph breaks, which carry meaning in an explanation, and drops
   the runs of blank lines and trailing spaces a paste tends to bring. */
export function normalizeReason(raw: string): string {
  return raw
    .replace(/\r\n?/g, "\n")
    .replace(/[^\S\n]+/g, " ")
    .replace(/ *\n */g, "\n")
    .replace(/\n{3,}/g, "\n\n")
    .trim();
}

export function validateEdit(value: unknown, languages: readonly string[]): EditValidation {
  if (!value || typeof value !== "object") return { ok: false, error: "invalid request" };
  const draft = value as Partial<EditDraft>;

  const questionId = typeof draft.questionId === "string" ? draft.questionId.trim() : "";
  if (!isQuestionId(questionId)) return { ok: false, error: "invalid question id" };

  const language = typeof draft.language === "string" ? draft.language : "";
  if (!languages.includes(language)) return { ok: false, error: "invalid language" };

  const proposedText = typeof draft.proposedText === "string" ? collapse(draft.proposedText) : "";
  if (proposedText && questionTextIssue(proposedText) !== null)
    return { ok: false, error: "invalid proposed wording" };

  const rawAspects = Array.isArray(draft.aspects) ? draft.aspects : [];
  if (rawAspects.length > EDIT_ASPECTS.length) return { ok: false, error: "invalid metadata" };
  const aspects: EditAspect[] = [];
  for (const aspect of rawAspects) {
    if (typeof aspect !== "string" || !isEditAspect(aspect))
      return { ok: false, error: "invalid metadata" };
    if (!aspects.includes(aspect)) aspects.push(aspect);
  }

  /* An edit that proposes neither wording nor a metadata change asks for
     nothing a maintainer could act on. */
  if (!proposedText && aspects.length === 0) return { ok: false, error: "nothing to change" };

  const reason = typeof draft.reason === "string" ? normalizeReason(draft.reason) : "";
  if (reason.length < REASON_MIN) return { ok: false, error: "a reason is required" };
  if (reason.length > REASON_MAX) return { ok: false, error: "the reason is too long" };

  /* Consent is asked for only when wording is contributed. A metadata-only
     request donates no text, so there is nothing to dedicate and no original
     writing to attest to. */
  if (proposedText) {
    if (draft.humanWritten !== true)
      return { ok: false, error: "Human Reserved confirmation is required" };
    if (draft.cc0 !== true || draft.consentVersion !== CC0_CONSENT_VERSION)
      return { ok: false, error: "CC0 consent is required" };
  }

  const submissionId = typeof draft.submissionId === "string" ? draft.submissionId : "";
  if (!SUBMISSION_ID.test(submissionId)) return { ok: false, error: "invalid submission id" };

  const turnstileToken =
    typeof draft.turnstileToken === "string" ? draft.turnstileToken.slice(0, 2048) : "";
  if (!turnstileToken) return { ok: false, error: "verification is required" };

  return {
    ok: true,
    edit: {
      questionId,
      language,
      proposedText,
      aspects,
      reason,
      submissionId,
      turnstileToken,
      website: typeof draft.website === "string" ? draft.website.slice(0, 200) : "",
    },
  };
}
