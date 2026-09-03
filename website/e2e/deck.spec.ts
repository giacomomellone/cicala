import { expect, test } from "@playwright/test";
import { payload, seedStorage, skipWithoutQuestions } from "./fixtures";

test.describe("your deck", () => {
  test("an empty deck explains itself and disables the actions", async ({ page }) => {
    await page.goto("/deck");
    await expect(page.locator("#d-empty")).toBeVisible();
    await expect(page.locator("#d-share")).toBeDisabled();
    await expect(page.locator("#d-export")).toBeDisabled();
  });

  test("saved questions appear and the deck chips filter them", async ({ page }) => {
    skipWithoutQuestions("en");
    const questions = payload("en");
    const work = questions.find((q) => q.decks.includes("work"))!;
    const notWork = questions.find((q) => !q.decks.includes("work"))!;
    await seedStorage(page, {
      "cicala.favs": JSON.stringify([work.id, notWork.id]),
    });

    await page.goto("/deck");
    await expect(page.locator("#d-rows .row")).toHaveCount(2);
    await expect(page.locator("#d-count")).toHaveText(/^2\b/);

    await page.locator('#deck-chips [data-deck="work"]').click();
    await expect(page.locator("#d-rows .row-q")).toHaveText([work.text]);
  });

  test("share copies a link that reproduces the deck", async ({ page }) => {
    skipWithoutQuestions("en");
    const ids = payload("en")
      .slice(0, 3)
      .map((q) => q.id);
    await seedStorage(page, { "cicala.favs": JSON.stringify(ids) });
    await page.goto("/deck");

    await page.locator("#d-share").click();
    await expect(page.locator("#d-status")).not.toBeEmpty();
    const link = await page.evaluate(() => navigator.clipboard.readText());
    expect(link).toBe(`${new URL(page.url()).origin}/deck#ids=${ids.join(",")}`);

    await page.goto(link.replace(new URL(link).origin, ""));
    await expect(page.locator("#d-rows .row")).toHaveCount(3);
  });

  test("a shared deck is read-only and offers to save all", async ({ page }) => {
    skipWithoutQuestions("en");
    const ids = payload("en")
      .slice(0, 2)
      .map((q) => q.id);
    await page.goto(`/deck#ids=${ids.join(",")}`);

    await expect(page.locator("#d-own-actions")).toBeHidden();
    await expect(page.locator("#d-saveall")).toBeVisible();
    await expect(page.locator("#d-rows .row")).toHaveCount(2);

    await page.locator("#d-rows .row-fav").first().click();
    await expect.poll(() => page.evaluate(() => localStorage.getItem("cicala.favs"))).toBe(null);

    await page.locator("#d-saveall").click();
    await expect
      .poll(() => page.evaluate(() => localStorage.getItem("cicala.favs")))
      .toBe(JSON.stringify(ids));
  });

  test("a shared deck resolves ids across languages", async ({ page }) => {
    skipWithoutQuestions("en", "de");
    const english = payload("en")[0]!;
    const german = payload("de")[0]!;
    await page.goto(`/deck#ids=${english.id},${german.id}`);
    await expect(page.locator("#d-rows .row-q")).toHaveText([english.text, german.text]);
  });

  test("unknown ids in a shared link are skipped", async ({ page }) => {
    skipWithoutQuestions("en");
    const known = payload("en")[0]!;
    await page.goto(`/deck#ids=q-00000000,${known.id},not-an-id`);
    await expect(page.locator("#d-rows .row")).toHaveCount(1);
    await expect(page.locator("#d-rows .row-q")).toHaveText([known.text]);
  });

  test("export writes the deck as json", async ({ page }) => {
    skipWithoutQuestions("en");
    const ids = payload("en")
      .slice(0, 2)
      .map((q) => q.id);
    await seedStorage(page, { "cicala.favs": JSON.stringify(ids) });
    await page.goto("/deck");

    const [download] = await Promise.all([
      page.waitForEvent("download"),
      page.locator("#d-export").click(),
    ]);
    expect(download.suggestedFilename()).toBe("deck.json");
  });
});

test.describe("language", () => {
  test("switching language reloads into the other corpus", async ({ page }) => {
    skipWithoutQuestions("en", "de");
    await page.goto("/browse");
    await page.locator('#lang-switch button[data-lang="de"]').click();

    await expect.poll(() => page.evaluate(() => localStorage.getItem("cicala.lang"))).toBe("de");

    // The English rows are statically rendered; the island swaps them once the German payload lands.
    const german = new Set(payload("de").map((q) => q.text));
    await expect
      .poll(async () => {
        const shown = await page.locator("#rows .row-q").allInnerTexts();
        return shown.length > 0 && shown.every((text) => german.has(text));
      })
      .toBe(true);
  });

  test("the nav is translated after a switch", async ({ page }) => {
    await page.goto("/");
    await page.locator('#lang-switch button[data-lang="de"]').click();
    await expect(page.locator('a[href="/browse"]')).toHaveText("stöbern");
  });
});
