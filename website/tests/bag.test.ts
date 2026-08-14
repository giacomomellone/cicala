// A cycle has no repeats and avoids the five most recent questions when refilled.
import { describe, expect, it } from "vitest";
import { drawFromBag } from "../src/lib/bag";
import type { Bag } from "../src/lib/store";

const ids = (n: number) => Array.from({ length: n }, (_, i) => `q-${String(i).padStart(8, "0")}`);

describe("drawFromBag", () => {
  it("draws every id exactly once before any repeat", () => {
    const all = ids(20);
    const bag: Bag = { b: [], r: [] };
    const seen = new Set<string>();
    for (let i = 0; i < all.length; i++) {
      const id = drawFromBag(bag, all);
      expect(id).not.toBeNull();
      expect(seen.has(id!)).toBe(false);
      seen.add(id!);
    }
    expect(seen.size).toBe(all.length);
  });

  it("excludes the last 5 shown from the next cycle's opening draws", () => {
    const all = ids(30);
    const bag: Bag = { b: [], r: [] };
    const lastFive: string[] = [];
    for (let i = 0; i < all.length; i++) {
      const id = drawFromBag(bag, all)!;
      lastFive.push(id);
    }
    const recent = new Set(lastFive.slice(-5));
    // The first draw of a refilled cycle must avoid the recent ring.
    const next = drawFromBag(bag, all)!;
    expect(recent.has(next)).toBe(false);
  });

  it("drops ids that no longer exist upstream", () => {
    const bag: Bag = { b: ["q-deadbeef", "q-00000001"], r: [] };
    const id = drawFromBag(bag, ["q-00000001"]);
    expect(id).toBe("q-00000001");
    expect(bag.b).not.toContain("q-deadbeef");
  });

  it("returns null only for an empty category", () => {
    const bag: Bag = { b: [], r: [] };
    expect(drawFromBag(bag, [])).toBeNull();
  });

  it("still cycles when the category is smaller than the recent window", () => {
    const all = ids(3);
    const bag: Bag = { b: [], r: [] };
    for (let i = 0; i < 12; i++) {
      expect(drawFromBag(bag, all)).not.toBeNull();
    }
  });

  it("caps the recent list at 5", () => {
    const all = ids(50);
    const bag: Bag = { b: [], r: [] };
    for (let i = 0; i < 20; i++) drawFromBag(bag, all);
    expect(bag.r.length).toBe(5);
  });
});
