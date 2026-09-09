// Selection happens at draw time, with the same priorities as firmware/lib/qdb.
import { FORMS, RECENT_WINDOW, type Permissions } from "../config";

export interface TextureMeta {
  depth: number;
  tags: string[];
}
export interface Texture {
  seed?: TextureMeta | undefined;
  meta: (id: string) => TextureMeta | undefined;
}
export interface Bag {
  seen: string[];
  r: string[];
  last: string;
}

export function permitted(q: TextureMeta, p: Permissions): boolean {
  return (
    (q.depth <= 2 || p.heavy) &&
    (!q.tags.includes("dark") || p.dark) &&
    (!q.tags.includes("sexual") || p.sexual)
  );
}
export function shuffle<T>(arr: T[], rand: () => number = Math.random): T[] {
  for (let i = arr.length - 1; i > 0; i--) {
    const j = Math.floor(rand() * (i + 1));
    [arr[i], arr[j]] = [arr[j]!, arr[i]!];
  }
  return arr;
}
function cost(q: TextureMeta | undefined, last: TextureMeta | undefined): number {
  if (!q || !last) return 0;
  // A breather outranks the two texture preferences, within unseen candidates.
  return (
    (last.depth === 3 && q.depth === 3 ? 3 : 0) +
    (Math.min(q.depth, 2) === Math.min(last.depth, 2) ? 1 : 0) +
    (FORMS.some((form) => q.tags.includes(form) && last.tags.includes(form)) ? 1 : 0)
  );
}
export function drawFromBag(
  bag: Bag,
  allowedIds: string[],
  rand: () => number = Math.random,
  texture?: Texture,
): string | null {
  if (!allowedIds.length) return null;
  const seen = new Set(bag.seen);
  let pool = allowedIds.filter((id) => !seen.has(id));
  if (!pool.length) {
    // Reopen only the eligible cycle. Excluded questions keep their seen state.
    bag.seen = bag.seen.filter((id) => !allowedIds.includes(id));
    pool = allowedIds.slice();
  }
  const fresh = pool.filter((id) => !bag.r.includes(id) && id !== bag.last);
  if (fresh.length) pool = fresh;
  else {
    const other = pool.filter((id) => id !== bag.last);
    if (other.length) pool = other;
  }
  if (texture) {
    const scores = pool.map((id) => cost(texture.meta(id), texture.seed));
    const best = Math.min(...scores);
    pool = pool.filter((_, i) => scores[i] === best);
  }
  const id = pool[Math.min(pool.length - 1, Math.floor(rand() * pool.length))]!;
  bag.seen.push(id);
  bag.r = [...bag.r, id].slice(-RECENT_WINDOW);
  bag.last = id;
  return id;
}
