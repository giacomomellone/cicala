// A cycle has no repeats and avoids the five most recent questions when refilled.
import { describe, expect, it } from "vitest";
import { drawFromBag, type TextureMeta } from "../src/lib/bag";
import type { Bag } from "../src/lib/store";

const ids = (n: number) => Array.from({ length: n }, (_, i) => `q-${String(i).padStart(8, "0")}`);

const seeded = (start: number) => {
  let state = start;
  return () => {
    state = (state * 1103515245 + 12345) % 2147483648;
    return state / 2147483648;
  };
};

const metaOf =
  (spec: Record<string, TextureMeta>) =>
  (id: string): TextureMeta | undefined =>
    spec[id];

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

describe("drawFromBag texture", () => {
  it("alternates depth bands across a refill when the pool is balanced", () => {
    const all = ids(10);
    const spec = Object.fromEntries(
      all.map((id, i) => [id, { depth: (i % 2) + 1, tags: [] }]),
    ) as Record<string, TextureMeta>;
    const bag: Bag = { b: [], r: [] };
    const served: string[] = [];
    for (let i = 0; i < all.length; i++) {
      served.push(drawFromBag(bag, all, seeded(7), { meta: metaOf(spec) })!);
    }
    expect(new Set(served).size).toBe(all.length);
    for (let i = 1; i < served.length; i++) {
      expect(spec[served[i]!]!.depth).not.toBe(spec[served[i - 1]!]!.depth);
    }
  });

  it("avoids serving the same form twice in a row while an alternative exists", () => {
    const all = ids(6);
    // One depth band only, so the band preference cannot be satisfied; the
    // form preference must survive that relaxation.
    const spec = Object.fromEntries(
      all.map((id, i) => [id, { depth: 2, tags: [i % 2 === 0 ? "memory" : "reflective"] }]),
    ) as Record<string, TextureMeta>;
    const bag: Bag = { b: [], r: [] };
    const served: string[] = [];
    for (let i = 0; i < all.length; i++) {
      served.push(drawFromBag(bag, all, seeded(3), { meta: metaOf(spec) })!);
    }
    for (let i = 1; i < served.length; i++) {
      expect(spec[served[i]!]!.tags).not.toEqual(spec[served[i - 1]!]!.tags);
    }
  });

  it("respects the seed question from the previous cycle or deck", () => {
    const all = ids(4);
    const spec = Object.fromEntries(
      all.map((id, i) => [id, { depth: (i % 2) + 1, tags: [] }]),
    ) as Record<string, TextureMeta>;
    const bag: Bag = { b: [], r: [] };
    const seed: TextureMeta = { depth: 1, tags: [] };
    const first = drawFromBag(bag, all, seeded(11), { seed, meta: metaOf(spec) })!;
    expect(spec[first]!.depth).toBe(2);
  });

  it("still draws everything when every candidate repeats the texture", () => {
    const all = ids(5);
    const spec = Object.fromEntries(
      all.map((id) => [id, { depth: 2, tags: ["memory"] }]),
    ) as Record<string, TextureMeta>;
    const bag: Bag = { b: [], r: [] };
    for (let i = 0; i < 12; i++) {
      expect(drawFromBag(bag, all, seeded(5), { meta: metaOf(spec) })).not.toBeNull();
    }
  });

  it("ignores tone tags when comparing forms", () => {
    const all = ids(4);
    const spec = Object.fromEntries(
      all.map((id, i) => [id, { depth: (i % 2) + 1, tags: i < 2 ? ["dark"] : ["dark", "memory"] }]),
    ) as Record<string, TextureMeta>;
    const bag: Bag = { b: [], r: [] };
    // Seed shares the memory form with half the pool; "dark" must not count
    // as a form, so the draw prefers a memory-free candidate.
    const seed: TextureMeta = { depth: 2, tags: ["memory"] };
    const first = drawFromBag(bag, all, seeded(13), { seed, meta: metaOf(spec) })!;
    expect(spec[first]!.tags).toEqual(["dark"]);
  });
});
