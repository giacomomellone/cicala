// Editorial hints are advisory: they name a pattern and never block a send.
import { describe, expect, it } from "vitest";
import { styleHint } from "../src/lib/style-hints";

describe("styleHint", () => {
  it("says nothing about an empty or clean question", () => {
    expect(styleHint("", "en")).toBeNull();
    expect(styleHint("Which book do you still think about at odd moments?", "en")).toBeNull();
    expect(styleHint("Wann hast du zuletzt deine Meinung geändert?", "de")).toBeNull();
  });

  it("flags a superlative frame", () => {
    expect(styleHint("What's the best book you've ever read?", "en")).toBe("ranking");
    expect(styleHint("What's your favourite holiday memory so far?", "en")).toBe("ranking");
    expect(styleHint("What's your favorite holiday memory so far?", "en")).toBe("ranking");
    expect(styleHint("Was war der beste Rat, den du je bekommen hast?", "de")).toBe("ranking");
  });

  // The two-part shape both style guides recommend asks for a story, not a
  // verdict, so the superlative in it is not the problem.
  it("exempts a superlative followed by a story clause", () => {
    expect(
      styleHint("What's the best party you've ever been to, and what made it great?", "en"),
    ).toBeNull();
    expect(styleHint("Wo ist hier der beste Platz, und warum?", "de")).toBeNull();
  });

  it("reads superlatives in the question's own language only", () => {
    expect(styleHint("Was war der beste Rat, den du je bekommen hast?", "en")).toBeNull();
  });

  it("flags two questions in one entry", () => {
    expect(
      styleHint("What do you do for work? Which part would surprise your younger self?", "en"),
    ).toBe("stacked");
  });

  it("flags length above the target, well under the technical limit", () => {
    const long = `Which part of your work ${"still ".repeat(14)}surprises you?`;
    expect(long.length).toBeGreaterThan(95);
    expect(long.length).toBeLessThan(140);
    expect(styleHint(long, "en")).toBe("long");
  });

  it("reports the mechanical problem's editorial cousin only once", () => {
    // A ranking that is also too long reports the ranking: one hint at a time.
    const both = `What's the best excuse you've ever used ${"to leave a party early ".repeat(4)}?`;
    expect(both.length).toBeGreaterThan(95);
    expect(styleHint(both, "en")).toBe("ranking");
  });
});
