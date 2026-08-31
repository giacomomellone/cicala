import { describe, expect, it } from "vitest";
import { CC0_CONSENT_VERSION, isSuggestionSource, validateSuggestion } from "../src/lib/submission";

const valid = {
  question: "Which ordinary day would you happily live again?",
  language: "en",
  credit: "Ada",
  cc0: true,
  consentVersion: CC0_CONSENT_VERSION,
  submissionId: "4c527b9a-65a6-4c45-9a17-0b07620aebd0",
  source: "suggest",
  turnstileToken: "verified-token",
  website: "",
};

describe("validateSuggestion", () => {
  it("normalizes a valid browser submission", () => {
    const result = validateSuggestion(
      { ...valid, question: `  ${valid.question}  `, credit: "  Ada   Lovelace " },
      ["en", "de"],
    );
    expect(result).toEqual({
      ok: true,
      suggestion: {
        question: valid.question,
        language: "en",
        credit: "Ada Lovelace",
        submissionId: valid.submissionId,
        source: "suggest",
        turnstileToken: "verified-token",
        website: "",
      },
    });
  });

  it("refuses missing consent and unsupported languages", () => {
    expect(validateSuggestion({ ...valid, cc0: false }, ["en", "de"])).toEqual({
      ok: false,
      error: "CC0 consent is required",
    });
    expect(validateSuggestion({ ...valid, language: "fr" }, ["en", "de"])).toEqual({
      ok: false,
      error: "invalid language",
    });
  });

  it("refuses invalid questions, identifiers, sources, and long credit", () => {
    expect(validateSuggestion({ ...valid, question: "A statement." }, ["en"])).toMatchObject({
      ok: false,
    });
    expect(validateSuggestion({ ...valid, submissionId: "7" }, ["en"])).toMatchObject({
      ok: false,
    });
    expect(validateSuggestion({ ...valid, source: "advert" }, ["en"])).toMatchObject({
      ok: false,
    });
    expect(validateSuggestion({ ...valid, credit: "A".repeat(41) }, ["en"])).toMatchObject({
      ok: false,
    });
  });
});

describe("isSuggestionSource", () => {
  it("recognizes only the entry points the site emits", () => {
    expect(isSuggestionSource("browse-empty")).toBe(true);
    expect(isSuggestionSource("email")).toBe(false);
  });
});
