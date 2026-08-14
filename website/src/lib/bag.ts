// Draw each eligible question once per cycle and avoid the five most recent.

import type { Bag } from "./store";

export function shuffle<T>(arr: T[], rand: () => number = Math.random): T[] {
  for (let i = arr.length - 1; i > 0; i--) {
    const j = Math.floor(rand() * (i + 1));
    [arr[i], arr[j]] = [arr[j]!, arr[i]!];
  }
  return arr;
}

/* Draw the next id. */
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
