// The contribute form's pre-validation must mirror tools/validate.py —
// these cases are the same ones tools/tests/test_tools.py checks in Python.
import { describe, expect, it } from "vitest";
import { questionTextIssue } from "../src/lib/rules";

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

  it("rejects multi-line text", () => {
    expect(questionTextIssue("What matters\nmost to you today?")).toBe("mark");
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
});
