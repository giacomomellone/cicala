// Form validation follows the same rules as tools/validate.py.
import { describe, expect, it } from "vitest";
import { buildTextIndex, collapse, duplicateOf, questionTextIssue } from "../src/lib/rules";

describe("questionTextIssue", () => {
  it("accepts a plain valid question", () => {
    expect(
      questionTextIssue("When did you last change your mind about something important?"),
    ).toBeNull();
  });

  it("accepts a terminator followed by a closing quote", () => {
    expect(questionTextIssue('When did you last ask yourself "why not me?"')).toBeNull();
    expect(questionTextIssue("Wann hast du zuletzt gefragt: „warum eigentlich?”")).toBeNull();
  });

  it("rejects text that does not end with ?", () => {
    expect(questionTextIssue("Tell me about your best day ever.")).toBe("mark");
  });

  it("rejects multi-line text, and says so rather than blaming the ?", () => {
    expect(questionTextIssue("What matters\nmost to you today?")).toBe("multiline");
  });

  it("rejects too-short and too-long text", () => {
    expect(questionTextIssue("Why not?")).toBe("short");
    expect(questionTextIssue("W" + "h".repeat(140) + "y?")).toBe("long");
  });

  it("boundary lengths: exactly 10 and exactly 140 are valid", () => {
    const ten = "Why is it?";
    expect(ten.length).toBe(10);
    expect(questionTextIssue(ten)).toBeNull();
    const q140 = "W".repeat(139) + "?";
    expect(q140.length).toBe(140);
    expect(questionTextIssue(q140)).toBeNull();
  });

  // Validate the normalized text written by promote_issue.py.
  it("measures the stored text, not the padding around it", () => {
    const q140 = "W".repeat(139) + "?";
    expect(questionTextIssue(`   ${q140}   `)).toBeNull();
    expect(questionTextIssue("  Why is it?  ")).toBeNull();
    expect(questionTextIssue("What  did   you  learn today?")).toBeNull();
  });
});

describe("collapse", () => {
  it("matches what the database stores", () => {
    expect(collapse("  What   did\n you  learn today?  ")).toBe("What did you learn today?");
  });
});

describe("duplicateOf", () => {
  const index = buildTextIndex([
    { id: "q-11111111", text: "What did you learn today?" },
    { id: "q-22222222", text: "Wofür bist du heute dankbar?" },
  ]);

  it("finds an exact match", () => {
    expect(duplicateOf("What did you learn today?", index)).toBe("q-11111111");
  });

  it("ignores case, padding and repeated spaces, like validate.py", () => {
    expect(duplicateOf("  WHAT   did you LEARN today?  ", index)).toBe("q-11111111");
  });

  it("compares composed and decomposed accents as equal", () => {
    const decomposed = "Wofür bist du heute dankbar?".normalize("NFD");
    expect(decomposed).not.toBe("Wofür bist du heute dankbar?");
    expect(duplicateOf(decomposed, index)).toBe("q-22222222");
  });

  it("returns null for a question the database does not have", () => {
    expect(duplicateOf("Which unremarkable Tuesday would you relive?", index)).toBeNull();
  });
});
