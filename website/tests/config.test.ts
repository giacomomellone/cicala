// The display id must match the device formula exactly (docs/sync_protocol.md):
// decimal of the first 2 id-hash bytes mod 10000.
import { describe, expect, it } from "vitest";
import { CATEGORIES, displayId } from "../src/config";

describe("displayId", () => {
  it("matches the worked example from the sync protocol", () => {
    // q-8f3a2c1d → 0x8f3a = 36666 → mod 10000 = 6666
    expect(displayId("q-8f3a2c1d")).toBe("#6666");
  });

  it("stays within #0…#9999", () => {
    expect(displayId("q-00000000")).toBe("#0");
    expect(displayId("q-ffffffff")).toBe("#" + (0xffff % 10000));
  });
});

describe("categories", () => {
  it("keeps the knob order from questions/schema.json", () => {
    expect(CATEGORIES).toEqual(["party", "family", "love", "work", "deep"]);
  });
});
