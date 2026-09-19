import { expect, test, type Page } from "@playwright/test";
import { readFileSync } from "node:fs";
import { playReady } from "./fixtures";
import { permitted } from "../src/lib/bag";
import { DEFAULT_PERMISSIONS } from "../src/config";
// Reuse the firmware's existing question text. These responses never enter shipped data.
const source = readFileSync(
  new URL("../../firmware/tests/corpus/en/questions.yaml", import.meta.url),
  "utf8",
);
const questions = [
  ...source.matchAll(/- id: (\S+)\n  text: ("[^\n]+")\n  depth: (\d)\n  tags: \[([^\]]*)\]/g),
].map((m) => ({
  id: m[1]!,
  text: JSON.parse(m[2]!) as string,
  depth: Number(m[3]),
  tags: m[4]!
    .split(",")
    .map((t) => t.trim())
    .filter(Boolean),
}));
async function fixture(page: Page, corpus = questions) {
  await page.addInitScript((questions) => {
    const original = window.fetch.bind(window);
    window.fetch = async (input, init) => {
      const config = JSON.parse(document.getElementById("cicala-config")?.textContent ?? "{}");
      if (String(input) === config.payloads?.en)
        return new Response(JSON.stringify({ version: "filters-fixture", questions }), {
          headers: { "Content-Type": "application/json" },
        });
      return original(input, init);
    };
  }, corpus);
  await page.emulateMedia({ reducedMotion: "reduce" });
}
async function done(page: Page) {
  for (let i = 0; i < 3; i++) await page.locator("#q-filters").click();
  await page.locator("#q-next").click();
  await expect(page.locator("#q-menu")).toBeHidden();
}
test("a draft survives reload and applies only at Done", async ({ page }) => {
  await fixture(page);
  await page.goto("/play");
  await playReady(page);
  const shown = await page.locator("#q-text").innerText();
  expect(questions.filter((q) => permitted(q, DEFAULT_PERMISSIONS)).map((q) => q.text)).toContain(
    shown,
  );
  await page.locator("body").press("f");
  await expect(page.locator("#q-menu")).toBeVisible();
  await expect(page.locator("#q-meta")).toBeHidden();
  await page.locator("#q-next").click();
  await expect(page.locator("#q-filter-0")).toContainText("allowed");
  await expect(page.locator("#q-permissions")).toContainText("dark −");
  await page.reload();
  await playReady(page);
  await expect(page.locator("#q-filter-0")).toContainText("allowed");
  await done(page);
  await expect(page.locator("#q-text")).toHaveText(shown);
  await expect(page.locator("#q-permissions")).toContainText("dark +");
});
test("clickable switches share the draft and cursor with the device controls", async ({ page }) => {
  await fixture(page);
  await page.goto("/play");
  await playReady(page);
  const shown = await page.locator("#q-text").innerText();
  await page.locator("#q-filters").click();
  const dark = page.getByRole("switch", { name: "dark", exact: true });
  const sexual = page.getByRole("switch", { name: "sexual", exact: true });
  const heavy = page.getByRole("switch", { name: "heavy", exact: true });
  await sexual.click();
  await expect(sexual).toBeChecked();
  await expect(sexual).toHaveAttribute("aria-current", "true");
  await page.locator("#q-next").click();
  await expect(sexual).not.toBeChecked();
  await page.locator("#q-filters").click();
  await expect(heavy).toHaveAttribute("aria-current", "true");
  await page.locator("#q-next").click();
  await expect(heavy).toBeChecked();
  await heavy.click();
  await expect(heavy).not.toBeChecked();
  await dark.press("Space");
  await expect(dark).toBeChecked();
  await expect(dark).toBeFocused();
  await sexual.press("Enter");
  await expect(sexual).toBeChecked();
  await expect(page.locator("#q-permissions")).toHaveText("dark − · sexual − · heavy −");
  await page.getByRole("button", { name: "done", exact: true }).click();
  await expect(page.locator("#q-menu")).toBeHidden();
  await expect(page.locator("#q-text")).toHaveText(shown);
  await expect(page.locator("#q-permissions")).toHaveText("dark + · sexual + · heavy −");
  await expect(page.locator("#q-filters")).toBeFocused();
  await page.reload();
  await playReady(page);
  await page.locator("#q-filters").click();
  await expect(dark).toBeChecked();
  await expect(sexual).toBeChecked();
  await expect(heavy).not.toBeChecked();
});
test("an empty permitted pool keeps Filters reachable", async ({ page }) => {
  const dark = questions.filter(
    (q) => q.tags.includes("dark") && q.depth <= 2 && !q.tags.includes("sexual"),
  );
  expect(dark.length).toBeGreaterThan(0);
  await fixture(page, dark);
  await page.goto("/play");
  await playReady(page);
  await expect(page.locator("#q-text")).toContainText("no questions match");
  await expect(page.locator("#q-meta")).toBeHidden();
  await page.locator("#q-filters").click();
  await page.locator("#q-next").click();
  await done(page);
  expect(dark.map((q) => q.text)).toContain(await page.locator("#q-text").innerText());
  await page.locator("#q-filters").click();
  await page.locator("#q-next").click();
  await done(page);
  await expect(page.locator("#q-text")).toContainText("no questions match");
});
test("Browse and saved collections include restricted questions", async ({ page }) => {
  await fixture(page);
  await page.goto("/browse");
  await page.locator("#b-depth").selectOption("3");
  await expect(page.locator("#rows .row")).toHaveCount(
    questions.filter((q) => q.depth === 3).length,
  );
  const row = page.locator("#rows .row").first();
  const text = await row.locator(".row-q").innerText();
  await row.locator(".row-fav").click();
  await page.goto("/deck");
  await expect(page.locator("#d-rows .row-q")).toHaveText([text]);
});
for (const width of [320, 390, 768, 1440])
  test(`Filters fits at ${width}px`, async ({ page }) => {
    await fixture(page);
    await page.setViewportSize({ width, height: 900 });
    await page.goto("/play");
    await playReady(page);
    await page.locator("#q-filters").click();
    await expect(page.locator("#q-filter-3")).toBeVisible();
    expect(await page.evaluate(() => document.documentElement.scrollWidth)).toBeLessThanOrEqual(
      width,
    );
    await page.screenshot({ path: `/tmp/cicala-filters-${width}.png`, fullPage: true });
  });

test("a direct link does not enable permissions and Next returns to the allowed pool", async ({
  page,
}) => {
  const restricted = questions.find((q) => q.depth === 3)!;
  await fixture(page);
  await page.route(`**/q/${restricted.id}`, async (route) => {
    const home = await page.request.get("/play");
    const html = (await home.text())
      .replace('id="play-root"', `id="play-root" data-seed-id="${restricted.id}"`)
      .replace('data-permalink="0"', 'data-permalink="1"');
    await route.fulfill({ contentType: "text/html", body: html });
  });
  await page.goto(`/q/${restricted.id}`);
  await playReady(page);
  await expect(page.locator("#q-text")).toHaveText(restricted.text);
  await expect(page.locator("#q-permissions")).toContainText("heavy −");
  await page.locator("#q-next").click();
  await expect(page).toHaveURL(/\/play$/);
  expect(questions.filter((q) => permitted(q, DEFAULT_PERMISSIONS)).map((q) => q.text)).toContain(
    await page.locator("#q-text").innerText(),
  );
});

test("new tabs start excluded while the current tab retains permissions", async ({
  page,
  context,
}) => {
  await fixture(page);
  await page.goto("/play");
  await playReady(page);
  await page.locator("#q-filters").click();
  await page.locator("#q-next").click();
  await done(page);
  await page.reload();
  await playReady(page);
  await expect(page.locator("#q-permissions")).toContainText("dark +");
  const another = await context.newPage();
  await fixture(another);
  await another.goto("/play");
  await playReady(another);
  await expect(another.locator("#q-permissions")).toContainText("dark −");
  await another.close();
});

test("the fixture stream exhausts ordinary questions before repeating", async ({ page }) => {
  await fixture(page);
  await page.goto("/play");
  await playReady(page);
  const allowed = questions.filter((q) => permitted(q, DEFAULT_PERMISSIONS));
  const seen = new Set<string>();
  for (let i = 0; i < allowed.length; i++) {
    const text = await page.locator("#q-text").innerText();
    expect(seen.has(text)).toBe(false);
    seen.add(text);
    expect(allowed.map((q) => q.text)).toContain(text);
    await page.locator("#q-next").click();
  }
  expect(seen.size).toBe(allowed.length);
});
