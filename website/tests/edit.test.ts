import { describe, expect, it } from "vitest";
import { normalizeReason, REASON_MAX, validateEdit } from "../src/lib/edit";
import { CC0_CONSENT_VERSION } from "../src/lib/submission";

const LANGS = ["en", "de"] as const;

const draft = (overrides: Record<string, unknown> = {}) => ({
  questionId: "q-5a3fb181",
  language: "en",
  proposedText: "Which ordinary day would you happily live again?",
  aspects: ["depth"],
  reason: "The current wording asks for a ranking rather than a memory.",
  humanWritten: true,
  cc0: true,
  consentVersion: CC0_CONSENT_VERSION,
  submissionId: "4c527b9a-65a6-4c45-9a17-0b07620aebd0",
  turnstileToken: "verified-token",
  website: "",
  ...overrides,
});

describe("validateEdit", () => {
  it("accepts a wording change with both consents", () => {
    const result = validateEdit(draft(), LANGS);
    expect(result.ok).toBe(true);
    if (result.ok) {
      expect(result.edit.questionId).toBe("q-5a3fb181");
      expect(result.edit.aspects).toEqual(["depth"]);
    }
  });

  it("accepts a metadata-only change without asking for consent", () => {
    const result = validateEdit(
      draft({ proposedText: "", humanWritten: false, cc0: false, consentVersion: "" }),
      LANGS,
    );
    expect(result.ok).toBe(true);
    if (result.ok) expect(result.edit.proposedText).toBe("");
  });

  it("rejects a request that proposes nothing", () => {
    const result = validateEdit(draft({ proposedText: "", aspects: [] }), LANGS);
    expect(result).toEqual({ ok: false, error: "nothing to change" });
  });

  it.each([
    ["q-123", "invalid question id"],
    ["Q-5A3FB181", "invalid question id"],
    ["q-5a3fb1811", "invalid question id"],
    ["", "invalid question id"],
  ])("rejects the question id %s", (questionId, error) => {
    expect(validateEdit(draft({ questionId }), LANGS)).toEqual({ ok: false, error });
  });

  it("rejects a language the site does not ship", () => {
    expect(validateEdit(draft({ language: "fr" }), LANGS)).toEqual({
      ok: false,
      error: "invalid language",
    });
  });

  it.each([
    ["Short?"],
    ["Which ordinary day would you happily live again"],
    [`${"a".repeat(140)}?`],
  ])("rejects proposed wording %j", (proposedText) => {
    expect(validateEdit(draft({ proposedText }), LANGS)).toEqual({
      ok: false,
      error: "invalid proposed wording",
    });
  });

  /* The client warns about a second line; the endpoint normalizes it, the
     same way validateSuggestion treats a pasted question. */
  it("collapses wording onto one line rather than rejecting it", () => {
    const result = validateEdit(draft({ proposedText: "Two lines?\nIn one field?" }), LANGS);
    expect(result.ok).toBe(true);
    if (result.ok) expect(result.edit.proposedText).toBe("Two lines? In one field?");
  });

  it("requires a reason long enough to act on", () => {
    expect(validateEdit(draft({ reason: "bad" }), LANGS)).toEqual({
      ok: false,
      error: "a reason is required",
    });
  });

  it("caps the reason", () => {
    expect(validateEdit(draft({ reason: "a".repeat(REASON_MAX + 1) }), LANGS)).toEqual({
      ok: false,
      error: "the reason is too long",
    });
  });

  it("requires the Human Reserved confirmation when wording is contributed", () => {
    expect(validateEdit(draft({ humanWritten: false }), LANGS)).toEqual({
      ok: false,
      error: "Human Reserved confirmation is required",
    });
  });

  it.each([[{ cc0: false }], [{ consentVersion: "cc0-1.0-2020-01-01" }]])(
    "requires a current CC0 consent when wording is contributed (%j)",
    (overrides) => {
      expect(validateEdit(draft(overrides), LANGS)).toEqual({
        ok: false,
        error: "CC0 consent is required",
      });
    },
  );

  it("rejects an unknown metadata aspect", () => {
    expect(validateEdit(draft({ aspects: ["licence"] }), LANGS)).toEqual({
      ok: false,
      error: "invalid metadata",
    });
  });

  it("keeps one entry per repeated aspect", () => {
    const result = validateEdit(draft({ aspects: ["tags", "tags", "depth"] }), LANGS);
    expect(result.ok).toBe(true);
    if (result.ok) expect(result.edit.aspects).toEqual(["tags", "depth"]);
  });

  it("rejects a submission id that is not a v4 UUID", () => {
    expect(validateEdit(draft({ submissionId: "1234" }), LANGS)).toEqual({
      ok: false,
      error: "invalid submission id",
    });
  });

  it("requires a verification token", () => {
    expect(validateEdit(draft({ turnstileToken: "" }), LANGS)).toEqual({
      ok: false,
      error: "verification is required",
    });
  });

  it("carries the honeypot through instead of rejecting it", () => {
    const result = validateEdit(draft({ website: "https://spam.example" }), LANGS);
    expect(result.ok).toBe(true);
    if (result.ok) expect(result.edit.website).toBe("https://spam.example");
  });
});

describe("normalizeReason", () => {
  it("keeps paragraph breaks and drops padding", () => {
    expect(normalizeReason("  first   line \n\n\n\n  second  line  ")).toBe(
      "first line\n\nsecond line",
    );
  });

  it("normalizes CRLF", () => {
    expect(normalizeReason("one\r\ntwo")).toBe("one\ntwo");
  });
});
