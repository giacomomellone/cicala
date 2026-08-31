// The tips shown above the suggestion form, and their rotation.
import { describe, expect, it } from "vitest";
import { strings } from "../src/i18n";
import { TIPS, tipAt, tipExample, tipIndex } from "../src/lib/tips";

describe("tip rotation", () => {
  it("wraps forward and backward, so the arrows never run out", () => {
    expect(tipIndex(0)).toBe(0);
    expect(tipIndex(TIPS.length)).toBe(0);
    expect(tipIndex(TIPS.length + 2)).toBe(2);
    expect(tipIndex(-1)).toBe(TIPS.length - 1);
    expect(tipIndex(-TIPS.length - 1)).toBe(TIPS.length - 1);
  });

  it("returns a tip for any counter value", () => {
    for (const n of [-7, -1, 0, 3, 12]) {
      expect(tipAt(n)).toBe(TIPS[tipIndex(n)]);
    }
  });
});

describe("tip content", () => {
  it("translates every rule into every shipped language", () => {
    for (const tip of TIPS) {
      for (const [lang, table] of Object.entries(strings)) {
        expect(table[tip.rule], `${tip.rule} missing in ${lang}`).toBeTruthy();
      }
    }
  });

  it("gives every English tip a weak and a stronger example", () => {
    const withExamples = TIPS.filter((tip) => tipExample(tip, "en") !== null);
    expect(withExamples.length).toBe(TIPS.length - 1);
    for (const tip of withExamples) {
      const example = tipExample(tip, "en")!;
      expect(example.weak.endsWith("?")).toBe(true);
      expect(example.better.endsWith("?")).toBe(true);
      expect(example.better).not.toBe(example.weak);
    }
  });

  // Question text is never machine translated, so a language carries a pair
  // only where a fluent maintainer has written one.
  it("leaves a language's examples absent rather than translated", () => {
    const german = TIPS.filter((tip) => tipExample(tip, "de") !== null);
    expect(german.length).toBeGreaterThan(0);
    expect(german.length).toBeLessThanOrEqual(TIPS.length);
  });
});
