import { test, expect } from "@playwright/test";

for (const viewport of [
  { width: 1440, height: 900 },
  { width: 390, height: 844 },
  { width: 320, height: 740 },
]) {
  test(`landing opens the question app at ${viewport.width}px`, async ({ page }) => {
    await page.setViewportSize(viewport);
    await page.goto("/");
    await expect(page.getByRole("heading", { level: 1 })).toHaveText(
      "Grab a chair, I have a question for you",
    );
    const enter = page.getByRole("link", { name: "Try a question", exact: true });
    await expect(enter).toHaveText("");
    await expect(enter).toBeInViewport({ ratio: 1 });
    expect(await page.evaluate(() => document.documentElement.scrollWidth)).toBe(viewport.width);
    await expect(page.locator(".landing-photo img")).toBeVisible();
    expect(
      await page
        .locator(".landing-photo img")
        .evaluate((img: HTMLImageElement) => img.complete && img.naturalWidth > 0),
    ).toBe(true);
    await enter.click();
    await expect(page).toHaveURL(/\/play$/);
    await expect(page.locator("#q-text")).not.toBeEmpty();
    const nav = page.getByRole("navigation", { name: "site", exact: true });
    await nav.getByRole("link", { name: "info", exact: true }).click();
    await expect(page.getByRole("heading", { name: "About Cicala" })).toBeVisible();
    expect(await page.evaluate(() => document.documentElement.scrollWidth)).toBe(viewport.width);
    await nav.getByRole("link", { name: "device", exact: true }).click();
    await expect(page.getByRole("heading", { name: "the device", exact: true })).toBeVisible();
    await expect(page.getByRole("link", { name: "Check here for some updates" })).toHaveAttribute(
      "href",
      "https://github.com/giacomomellone/cicala/tree/main/hardware#readme",
    );
    await page
      .getByRole("navigation", { name: "site", exact: true })
      .getByRole("link", { name: "contribute", exact: true })
      .click();
    await expect(page).toHaveURL(/\/suggest$/);
    await expect(page.locator("textarea")).toBeVisible();
    await page.getByRole("link", { name: "Cicala home" }).click();
    await expect(enter).toBeVisible();
  });
}

test("the cicada responds to touch and respects reduced motion", async ({ page }) => {
  await page.goto("/");
  const stamp = page.getByRole("button", { name: "Give the cicada a nudge" });
  await stamp.click();
  expect(await stamp.evaluate((el) => el.getAnimations({ subtree: true }).length)).toBeGreaterThan(
    0,
  );
  await page.emulateMedia({ reducedMotion: "reduce" });
  await stamp.click();
  expect(await stamp.evaluate((el) => el.getAnimations({ subtree: true }).length)).toBe(0);
});

test("Info keeps its source and contribution links after changing language", async ({ page }) => {
  await page.goto("/info");
  const article = page.locator("article");
  await expect(article.locator("p")).toHaveText([
    "Cicala is a collection of questions for conversations around a table.",
    "Use them as you like.",
    "The project is open source, and anyone can contribute a question.",
  ]);
  await expect(article.getByRole("link", { name: "open source", exact: true })).toHaveAttribute(
    "href",
    "https://github.com/giacomomellone/cicala",
  );
  for (const lang of ["it", "de", "en"]) {
    await page.locator(`#lang-switch [data-lang="${lang}"]`).click();
    await expect(page.locator("html")).toHaveAttribute("lang", lang);
    await expect(article.locator('a[href="/suggest"]')).toBeVisible();
    await expect(
      article.locator('a[href="https://github.com/giacomomellone/cicala"]'),
    ).toBeVisible();
  }
  await article.getByRole("link", { name: "contribute", exact: true }).click();
  await expect(page).toHaveURL(/\/suggest$/);
  await expect(page.locator("textarea")).toBeVisible();
});

for (const width of [1440, 390]) {
  test(`device CAD and PCB assets load at ${width}px`, async ({ page }) => {
    await page.setViewportSize({ width, height: 900 });
    await page.goto("/device");
    await expect(page.locator(".device-assembly img")).toBeVisible();
    await expect(page.locator("#pcb img")).toHaveCount(2);
    for (const figure of await page.locator(".device-page figure").all()) {
      const img = figure.locator("img");
      await img.scrollIntoViewIfNeeded();
      await expect
        .poll(() => img.evaluate((el: HTMLImageElement) => el.complete && el.naturalWidth > 0))
        .toBe(true);
      const original = await figure.locator("a").first().getAttribute("href");
      expect((await page.request.get(original!)).ok()).toBe(true);
    }
    for (const link of await page.locator(".device-silkscreen").all()) {
      const response = await page.request.get((await link.getAttribute("href"))!);
      expect(response.ok()).toBe(true);
      expect(response.headers()["content-type"]).toContain("image/svg+xml");
    }
    expect(await page.evaluate(() => document.documentElement.scrollWidth)).toBe(width);
  });
}

test("the landing-to-app link works without JavaScript", async ({ browser }) => {
  const context = await browser.newContext({ javaScriptEnabled: false });
  const page = await context.newPage();
  await page.goto("/");
  await page.getByRole("link", { name: "Try a question", exact: true }).click();
  await expect(page).toHaveURL(/\/play$/);
  await expect(page.locator("#q-text")).not.toBeEmpty();
  await context.close();
});
