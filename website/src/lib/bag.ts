// The shuffle bag (spec §7.4): per lang+category, every question is shown
// once before any repeats; on refill, the last 5 shown are excluded so a
// fresh cycle never opens with something just seen. Pure logic — persistence
// lives in store.ts, DOM wiring in play.ts.

import type { Bag } from "./store";

export function shuffle<T>(arr: T[], rand: () => number = Math.random): T[] {
  for (let i = arr.length - 1; i > 0; i--) {
    const j = Math.floor(rand() * (i + 1));
    [arr[i], arr[j]] = [arr[j]!, arr[i]!];
  }
  return arr;
}

/** Draw the next id. Mutates and returns the bag (caller persists it).
 * Ids no longer present in `aliveIds` (removed upstream) are dropped.
 * Returns null only when `aliveIds` is empty. */
export function drawFromBag(
  bag: Bag,
  aliveIds: string[],
  rand: () => number = Math.random,
): string | null {
  if (aliveIds.length === 0) return null;
  const alive = new Set(aliveIds);
  bag.b = bag.b.filter((id) => alive.has(id));
  let id = bag.b.pop();
  if (!id) {
    const recent = new Set(bag.r);
    let pool = aliveIds.filter((x) => !recent.has(x));
    if (pool.length === 0) pool = aliveIds.slice();
    shuffle(pool, rand);
    id = pool.pop()!;
    bag.b = pool;
  }
  bag.r = [...bag.r, id].slice(-5);
  return id;
}
