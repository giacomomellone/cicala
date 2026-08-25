import { describe, expect, it } from "vitest";
import { DECKS, DEVICE_DECKS, DEPTH_OPTIONS, PLAYBACK_DEPTH_MAX, TAGS } from "../src/config";
import schema from "../../questions/schema.json";

const cfg = schema["x-kveld"];

describe("decks", () => {
  it("keeps the category order from questions/schema.json", () => {
    expect(DECKS).toEqual(["new_people", "close", "family", "work", "here", "wild"]);
  });

  it("the player offers the device cycle, in order, without work", () => {
    expect(DEVICE_DECKS).toEqual(["new_people", "close", "family", "here", "wild"]);
    expect(DEVICE_DECKS.every((deck) => (DECKS as readonly string[]).includes(deck))).toBe(true);
  });

  it("keeps depth 3 outside initial playback", () => {
    expect(PLAYBACK_DEPTH_MAX).toBe(2);
  });
});

// The website embeds schema vocabulary instead of parsing it at runtime.
describe("the schema is the source of truth", () => {
  it("agrees on the deck order", () => {
    expect([...DECKS]).toEqual(cfg.decks);
  });

  it("agrees on the tags", () => {
    expect([...TAGS]).toEqual(cfg.tags);
  });

  it("agrees on the playback depth", () => {
    expect(PLAYBACK_DEPTH_MAX).toBe(cfg.playbackDepthMax);
  });

  it("agrees on the issue form's depth options", () => {
    expect([...DEPTH_OPTIONS]).toEqual(cfg.depthLabels);
  });

  it("has one depth option per allowed depth", () => {
    expect(DEPTH_OPTIONS).toHaveLength(3);
    for (const [i, label] of DEPTH_OPTIONS.entries())
      expect(label.startsWith(String(i + 1))).toBe(true);
  });
});
