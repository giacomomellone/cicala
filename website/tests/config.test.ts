import { describe, expect, it } from "vitest";
import { DECKS, PLAYBACK_DEPTH_MAX } from "../src/config";

describe("decks", () => {
  it("keeps the absolute-selector order from questions/schema.json", () => {
    expect(DECKS).toEqual([
      "new_people",
      "close",
      "family",
      "work",
      "here",
      "wild",
    ]);
  });

  it("keeps depth 3 outside initial playback", () => {
    expect(PLAYBACK_DEPTH_MAX).toBe(2);
  });
});
