import { expect, it } from "vitest";
import fixture from "../../core/tests/fixtures/selection.json";
import { drawFromBag, permitted, type Bag } from "../src/lib/bag";

const permissions = (mask: number) => ({
  dark: !!(mask & 1),
  sexual: !!(mask & 2),
  heavy: !!(mask & 4),
});
const allowed = (mask: number) =>
  fixture.questions.flatMap((q, i) => (permitted(q, permissions(mask)) ? [i] : []));

it("agrees with the portable core on permissions and excluded seen history", () => {
  for (let mask = 0; mask < 8; mask++) expect(allowed(mask)).toEqual(fixture.eligible[mask]);
  const bag: Bag = { seen: [], r: [], last: "" };
  for (const [step, mask] of fixture.permissions.entries()) {
    const chosen = drawFromBag(bag, allowed(mask).map(String), () => 0, {
      seed: fixture.questions[Number(bag.last)],
      meta: (id) => fixture.questions[Number(id)],
    });
    expect(Number(chosen)).toBe(fixture.draws[step]);
  }
  expect(bag.seen.map(Number).sort()).toEqual(fixture.seen);
});
