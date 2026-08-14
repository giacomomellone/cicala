import { expect, test } from "@playwright/test";
import { captureWindowOpen, openedUrls, payload, schema } from "./fixtures";

// Deliberately not in questions/en/questions.yaml — the form refuses a text
// the database already holds, and these tests are about the other rules.
const GOOD = "Which unremarkable Tuesday would you happily live again?";

/** Fill the form to a submittable state, then apply the overrides. */
async function fillValid(page: import("@playwright/test").Page, text = GOOD): Promise<void> {
  await page.locator("#c-text").fill(text);
  await page.locator('input[name="decks"][value="close"]').check();
  await page.locator("#c-cc0").check();
}

test.describe("contribute form validation", () => {
  test.beforeEach(async ({ page }) => {
    await captureWindowOpen(page);
    await page.goto("/contribute");
  });

  test("submit stays disabled until every rule is met", async ({ page }) => {
    const submit = page.locator("#c-submit");
    await expect(submit).toBeDisabled();

    await page.locator("#c-text").fill(GOOD);
    await expect(submit).toBeDisabled(); // no deck yet

    await page.locator('input[name="decks"][value="close"]').check();
    await expect(submit).toBeDisabled(); // no CC0 yet

    await page.locator("#c-cc0").check();
    await expect(submit).toBeEnabled();
  });

  test("the counter and hint track the length rules", async ({ page }) => {
    await page.locator("#c-text").fill("too short?");
    await expect(page.locator("#c-counter")).toHaveText("10/140");
    await expect(page.locator("#c-rule")).toHaveText(/looks good/i);

    await page.locator("#c-text").fill("short?");
    await expect(page.locator("#c-rule")).toHaveText(/at least 10/i);
    await expect(page.locator("#c-counter")).toHaveClass(/is-bad/);

    await page.locator("#c-text").fill(`${"a".repeat(140)}?`);
    await expect(page.locator("#c-rule")).toHaveText(/140 characters max/i);
  });

  test("a question that does not end in a question mark is rejected", async ({ page }) => {
    await fillValid(page, "This is a statement about something.");
    await expect(page.locator("#c-rule")).toHaveText(/must end with \?/i);
    await expect(page.locator("#c-submit")).toBeDisabled();
  });

  test("a closing quote after the question mark is accepted", async ({ page }) => {
    await fillValid(page, '"What did you mean by that?"');
    await expect(page.locator("#c-rule")).toHaveText(/looks good/i);
    await expect(page.locator("#c-submit")).toBeEnabled();
  });

  test("dark and spicy questions are confined to wild", async ({ page }) => {
    await fillValid(page);
    await page.locator('input[name="tags"][value="dark"]').check();
    await expect(page.locator("#c-deck-rule")).toHaveText(/only in wild/i);
    await expect(page.locator("#c-submit")).toBeDisabled();

    await page.locator('input[name="decks"][value="close"]').uncheck();
    await page.locator('input[name="decks"][value="wild"]').check();
    await expect(page.locator("#c-submit")).toBeEnabled();
  });

  test("a new language routes to the incubator instead of the form", async ({ page }) => {
    await fillValid(page);
    await expect(page.locator("#c-newlang")).toBeHidden();
    await page.locator("#c-lang").selectOption("__other__");
    await expect(page.locator("#c-newlang")).toBeVisible();
    await expect(page.locator("#c-submit")).toBeDisabled();
  });
});

test.describe("contribute handoff to GitHub", () => {
  test.beforeEach(async ({ page }) => {
    await captureWindowOpen(page);
    await page.goto("/contribute");
  });

  test("every prefilled value is one the issue form accepts", async ({ page }) => {
    await fillValid(page);
    await page.locator('input[name="decks"][value="family"]').check();
    await page.locator('input[name="tags"][value="memory"]').check();
    await page.locator("#c-depth").selectOption("3");
    await page.locator("#c-name").fill("Ada");
    await page.locator("#c-submit").click();

    const opened = await openedUrls(page);
    expect(opened).toHaveLength(1);
    const url = new URL(opened[0]!);
    expect(url.pathname).toContain("/issues/new");

    const params = url.searchParams;
    expect(params.get("template")).toBe("new-question.yml");
    expect(params.get("labels")).toBe("question-submission");
    expect(params.get("question-text")).toBe(GOOD);
    expect(params.get("credit")).toBe("Ada");

    // GitHub silently drops a dropdown prefill whose value is not one of the
    // declared options, leaving a required field blank.
    const languages = Object.entries(schema.languages).map(
      ([code, { name }]) => `${name} (${code})`,
    );
    expect(languages).toContain(params.get("language"));
    expect(schema.depthLabels).toContain(params.get("depth"));
    for (const deck of params.get("decks")!.split(",")) expect(schema.decks).toContain(deck);
    for (const tag of params.get("tags")!.split(",")) expect(schema.tags).toContain(tag);
  });

  test("each depth prefills a value the issue form declares", async ({ page }) => {
    for (const depth of ["1", "2", "3"]) {
      await page.locator("#c-depth").selectOption(depth);
      await fillValid(page);
      await page.locator("#c-submit").click();
    }
    const opened = await openedUrls(page);
    expect(opened).toHaveLength(3);
    for (const [i, raw] of opened.entries()) {
      const value = new URL(raw).searchParams.get("depth");
      expect(value).toBe(schema.depthLabels[i]);
    }
  });

  test("whitespace is collapsed the way the database stores it", async ({ page }) => {
    await fillValid(page, `   What   did you  learn today?   `);
    await page.locator("#c-submit").click();
    const url = new URL((await openedUrls(page))[0]!);
    expect(url.searchParams.get("question-text")).toBe("What did you learn today?");
  });

  test("padding does not count against the length limit", async ({ page }) => {
    const exactly140 = `${"W".repeat(139)}?`;
    await fillValid(page, `    ${exactly140}    `);
    await expect(page.locator("#c-counter")).toHaveText("140/140");
    await expect(page.locator("#c-submit")).toBeEnabled();
  });

  test("a pasted line break is named, not blamed on the question mark", async ({ page }) => {
    await fillValid(page, "What matters\nmost to you today?");
    await expect(page.locator("#c-rule")).toHaveText(/one question per entry/i);
    await expect(page.locator("#c-submit")).toBeDisabled();
  });
});

test.describe("contribute duplicate guard", () => {
  test.beforeEach(async ({ page }) => {
    await captureWindowOpen(page);
    await page.goto("/contribute");
  });

  test("a question already in the database is refused before GitHub", async ({ page }) => {
    const existing = payload("en")[0]!;
    await expect(page.locator("#c-dupe")).toBeHidden();

    await fillValid(page, existing.text);
    await expect(page.locator("#c-rule")).toHaveText(/already/i);
    await expect(page.locator("#c-submit")).toBeDisabled();

    // and it says which question it already has
    await expect(page.locator("#c-dupe-link")).toHaveAttribute("href", `/q/${existing.id}`);
  });

  test("the duplicate check ignores case and spacing", async ({ page }) => {
    const existing = payload("en")[0]!;
    await fillValid(page, `  ${existing.text.toUpperCase()}  `);
    await expect(page.locator("#c-rule")).toHaveText(/already/i);
    await expect(page.locator("#c-submit")).toBeDisabled();
  });

  test("a question from another language is not a duplicate", async ({ page }) => {
    const german = payload("de")[0]!;
    await fillValid(page, german.text);
    await expect(page.locator("#c-submit")).toBeEnabled();
  });
});

test.describe("contribute page furniture", () => {
  test("recently added links to real permalinks", async ({ page }) => {
    await page.goto("/contribute");
    const links = page.locator("#c-recent .row-q");
    await expect(links.first()).toBeVisible();
    const ids = new Set(payload("en").map((q) => q.id));
    for (const href of await links.evaluateAll((nodes) =>
      nodes.map((n) => (n as HTMLAnchorElement).getAttribute("href")),
    ))
      expect(ids).toContain(href!.replace("/q/", ""));
  });
});
