import { describe, expect, it } from "vitest";
import { drawFromBag, permitted, type Bag } from "../src/lib/bag";
const fresh = (): Bag => ({ seen: [], r: [], last: "" });
const ordinary = { dark: false, sexual: false, heavy: false };
describe("independent permissions", () => {
  it("requires every applicable permission, including combinations", () => {
    for (let mask = 0; mask < 8; mask++) {
      const p = { dark: !!(mask & 1), sexual: !!(mask & 2), heavy: !!(mask & 4) };
      for (let required = 0; required < 8; required++) {
        const q = {
          depth: required & 4 ? 3 : 2,
          tags: [...(required & 1 ? ["dark"] : []), ...(required & 2 ? ["sexual"] : [])],
        };
        expect(permitted(q, p)).toBe((required & ~mask) === 0);
      }
    }
    expect(permitted({ depth: 1, tags: [] }, ordinary)).toBe(true);
  });
});
describe("mixed stream", () => {
  it("exhausts unseen questions before refilling and avoids the current one", () => {
    const bag = fresh();
    const pool = ["a", "b", "c"];
    expect(pool.map(() => drawFromBag(bag, pool, () => 0))).toEqual(pool);
    expect(drawFromBag(bag, pool, () => 0)).toBe("a");
  });
  it("keeps excluded seen history across permission changes", () => {
    const bag = fresh();
    expect(drawFromBag(bag, ["sexual", "a"], () => 0)).toBe("sexual");
    drawFromBag(bag, ["a"], () => 0);
    drawFromBag(bag, ["a"], () => 0);
    expect(bag.seen).toContain("sexual");
    expect(drawFromBag(bag, ["sexual", "a", "b"], () => 0)).toBe("b");
  });
  it("prefers a breather, then depth-band and form variation", () => {
    const bag = fresh();
    const meta = (id: string) => ({
      depth: id === "heavy" ? 3 : 2,
      tags: id === "breather" ? ["memory"] : ["reflective"],
    });
    expect(
      drawFromBag(bag, ["heavy", "same", "breather"], () => 0, {
        seed: { depth: 3, tags: ["reflective"] },
        meta,
      }),
    ).toBe("breather");
  });
  it("does not force an early repeat to provide a breather", () => {
    const bag = fresh();
    bag.seen = ["breather"];
    expect(
      drawFromBag(bag, ["heavy", "breather"], () => 0, {
        seed: { depth: 3, tags: [] },
        meta: (id) => ({ depth: id === "heavy" ? 3 : 1, tags: [] }),
      }),
    ).toBe("heavy");
  });
  it("avoids the last twenty when the unseen pool permits it", () => {
    const bag = fresh();
    const pool = Array.from({ length: 25 }, (_, i) => String(i));
    for (let i = 0; i < 100; i++) {
      const recent = bag.r.slice();
      expect(recent).not.toContain(drawFromBag(bag, pool, () => 0));
      expect(bag.r.length).toBeLessThanOrEqual(20);
    }
  });
  it("relaxes texture and recency for a singleton but never an empty pool", () => {
    const bag = fresh();
    expect(drawFromBag(bag, ["a"])).toBe("a");
    expect(drawFromBag(bag, ["a"])).toBe("a");
    const before = structuredClone(bag);
    expect(drawFromBag(bag, [])).toBeNull();
    expect(bag).toEqual(before);
  });
});
