// Draw each eligible question once per cycle and avoid the five most recent.
// A refill is also texture-aware: it prefers a different depth band and form
// than the previous question and relaxes to a plain shuffle when the pool
// cannot offer one. Texture is a shuffle property, not session state.

import type { Bag } from "./store";

export interface TextureMeta {
  depth: number;
  tags: string[];
}

export interface Texture {
  /** The question shown just before this draw, when known. */
  seed?: TextureMeta | undefined;
  /** Metadata lookup for a candidate id. */
  meta: (id: string) => TextureMeta | undefined;
}

export function shuffle<T>(arr: T[], rand: () => number = Math.random): T[] {
  for (let i = arr.length - 1; i > 0; i--) {
    const j = Math.floor(rand() * (i + 1));
    [arr[i], arr[j]] = [arr[j]!, arr[i]!];
  }
  return arr;
}

// Tone tags are not forms: dark and spicy mark tone, not question shape.
const TONE_TAGS = new Set(["dark", "spicy"]);

function bandOf(depth: number): number {
  return depth >= 2 ? 2 : 1;
}

function formsOf(meta: TextureMeta): Set<string> {
  return new Set(meta.tags.filter((t) => !TONE_TAGS.has(t)));
}

/* Cost of serving `candidate` right after the previous question: one point
   for repeating the depth band, one for overlapping form. */
function textureCost(candidate: TextureMeta, band: number, forms: Set<string>): number {
  let cost = 0;
  if (band !== 0 && bandOf(candidate.depth) === band) cost++;
  if (forms.size > 0) {
    for (const form of formsOf(candidate)) {
      if (forms.has(form)) {
        cost++;
        break;
      }
    }
  }
  return cost;
}

/* Order a shuffled refill so consecutive serves keep texture where the pool
   allows. Taking the first lowest-cost candidate from a shuffled list is a
   uniform choice within the cheapest class. Draws pop from the end, so the
   serve order is reversed into the bag. */
function arrange(pool: string[], texture: Texture): string[] {
  const remaining = pool.slice();
  const served: string[] = [];
  let band = texture.seed ? bandOf(texture.seed.depth) : 0;
  let forms = texture.seed ? formsOf(texture.seed) : new Set<string>();

  while (remaining.length > 0) {
    let best = -1;
    let bestCost = Infinity;
    for (let i = 0; i < remaining.length; i++) {
      const meta = texture.meta(remaining[i]!);
      const cost = meta === undefined ? 0 : textureCost(meta, band, forms);
      if (cost < bestCost) {
        bestCost = cost;
        best = i;
      }
    }
    const id = remaining.splice(best, 1)[0]!;
    served.push(id);
    const meta = texture.meta(id);
    if (meta !== undefined) {
      band = bandOf(meta.depth);
      forms = formsOf(meta);
    }
  }
  return served.reverse();
}

/* Draw the next id. */
export function drawFromBag(
  bag: Bag,
  aliveIds: string[],
  rand: () => number = Math.random,
  texture?: Texture,
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
    if (texture) pool = arrange(pool, texture);
    id = pool.pop()!;
    bag.b = pool;
  }
  bag.r = [...bag.r, id].slice(-5);
  return id;
}
