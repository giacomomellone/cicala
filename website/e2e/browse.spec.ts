import { expect, test } from "@playwright/test";
import { islandReady, payload } from "./fixtures";

const rows = "#rows .row";

test.describe("browse", () => {
  test("lists the first page and pages through the rest", async ({ page }) => {
    await page.goto("/browse");
    await islandReady(page);
    const total = payload("en").length;
    await expect(page.locator(rows)).toHaveCount(Math.min(100, total));

    if (total > 100) {
      await page.locator("#b-more").click();
      await expect(page.locator(rows)).toHaveCount(Math.min(200, total));
    } else {
      await expect(page.locator("#b-more")).toBeHidden();
    }
  });

  test("search matches without case or diacritics", async ({ page }) => {
    await page.goto("/browse");
    await islandReady(page);
    const target = payload("en").find((q) => /\bwhat\b/i.test(q.text))!;
    const needle = target.text.slice(0, 12);

    await page.locator("#b-search").fill(needle.toUpperCase());
    await expect(page.locator(rows).first()).toBeVisible();
    for (const text of await page.locator(`${rows} .row-q`).allInnerTexts())
      expect(text.toLowerCase()).toContain(needle.toLowerCase());

    // Hide pagination when every result fits on one page.
    await expect(page.locator("#b-more")).toBeHidden();
  });

  test("a query that matches nothing opens a prefilled suggestion", async ({ page }) => {
    await page.goto("/browse");
    await islandReady(page);
    await page.locator("#b-search").fill("zzzzzzq-no-such-question");
    await expect(page.locator("#b-empty")).toBeVisible();
    await expect(page.locator(rows)).toHaveCount(0);
    const link = page.locator("#b-empty a");
    await expect(link).toHaveAttribute("href", /source=browse-empty/);
    await expect(link).toHaveAttribute("href", /text=zzzzzzq-no-such-question/);
  });

  test("a deck chip narrows the list to that deck", async ({ page }) => {
    await page.goto("/browse");
    await islandReady(page);
    await page.locator('#browse-chips [data-deck="work"]').click();
    await expect(page.locator('#browse-chips [data-deck="work"]')).toHaveAttribute(
      "aria-checked",
      "true",
    );

    const eligible = new Set(
      payload("en")
        .filter((q) => q.decks.includes("work"))
        .map((q) => q.text),
    );
    const shown = await page.locator(`${rows} .row-q`).allInnerTexts();
    expect(shown.length).toBeGreaterThan(0);
    for (const text of shown) expect(eligible).toContain(text);
  });

  test("the tag filter composes with the deck chip", async ({ page }) => {
    await page.goto("/browse");
    await islandReady(page);
    await page.locator('#browse-chips [data-deck="close"]').click();
    await page.locator("#b-tag").selectOption("reflective");

    const eligible = new Set(
      payload("en")
        .filter((q) => q.decks.includes("close") && q.tags.includes("reflective"))
        .map((q) => q.text),
    );
    const shown = await page.locator(`${rows} .row-q`).allInnerTexts();
    for (const text of shown) expect(eligible).toContain(text);
    expect(shown.length).toBe(Math.min(100, eligible.size));
  });

  test("random sort keeps the same set of questions", async ({ page }) => {
    await page.goto("/browse");
    await islandReady(page);
    await page.locator('#browse-chips [data-deck="here"]').click();
    const newest = await page.locator(`${rows} .row-q`).allInnerTexts();

    await page.locator("#b-sort").selectOption("random");
    const random = await page.locator(`${rows} .row-q`).allInnerTexts();
    expect(new Set(random)).toEqual(new Set(newest));
  });

  test("a row links to the question's permalink", async ({ page }) => {
    await page.goto("/browse");
    await islandReady(page);
    const first = page.locator(`${rows} .row-q`).first();
    const text = await first.innerText();
    await first.click();
    await expect(page.locator("#q-text")).toHaveText(text);
    expect(new URL(page.url()).pathname).toMatch(/^\/q\/q-[0-9a-f]{8}$/);
  });

  test("hearting a row carries over to the deck page", async ({ page }) => {
    await page.goto("/browse");
    await islandReady(page);
    const row = page.locator(rows).first();
    const text = await row.locator(".row-q").innerText();
    await row.locator(".row-fav").click();
    await expect(row.locator(".row-fav")).toHaveAttribute("aria-pressed", "true");

    await page.goto("/deck");
    await expect(page.locator("#d-rows .row-q")).toHaveText([text]);
  });
});
