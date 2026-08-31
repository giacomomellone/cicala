import { expect, test, type Page, type Route } from "@playwright/test";
import { payload } from "./fixtures";

const GOOD = "Which unremarkable Tuesday would you happily live again?";

async function mockSubmissionApi(page: Page, post?: (route: Route) => Promise<void>) {
  await page.route("**/api/suggestions", async (route) => {
    if (route.request().method() === "GET") {
      await route.fulfill({
        json: {
          available: true,
          turnstileRequired: false,
          turnstileSiteKey: "",
          turnstileAction: "suggest-question",
        },
      });
      return;
    }
    if (post) await post(route);
    else await route.fulfill({ status: 201, json: { accepted: true, reference: 42 } });
  });
}

async function fillValid(page: Page, question = GOOD) {
  await page.locator("#s-text").fill(question);
  await page.locator("#s-cc0").check();
}

test.describe("native suggestion form", () => {
  test.beforeEach(async ({ page }) => {
    await mockSubmissionApi(page);
    await page.goto("/suggest");
  });

  test("asks for the question and consent, not editorial metadata", async ({ page }) => {
    await expect(page.locator("#s-submit")).toBeDisabled();
    await page.locator("#s-text").fill(GOOD);
    await expect(page.locator("#s-submit")).toBeDisabled();
    await page.locator("#s-cc0").check();
    await expect(page.locator("#s-submit")).toBeEnabled();

    await expect(page.locator('input[name="decks"]')).toHaveCount(0);
    await expect(page.locator('[name="depth"], [name="tags"], [name="tag"]')).toHaveCount(0);
  });

  test("tracks text rules and duplicates before submission", async ({ page }) => {
    await page.locator("#s-text").fill("short?");
    await expect(page.locator("#s-rule")).toHaveText(/at least 10/i);

    await page.locator("#s-text").fill("This is a statement about something.");
    await expect(page.locator("#s-rule")).toHaveText(/must end with \?/i);

    const existing = payload("en")[0]!;
    await page.locator("#s-text").fill(existing.text);
    await expect(page.locator("#s-rule")).toHaveText(/already/i);
    await expect(page.locator("#s-dupe-link")).toHaveAttribute("href", `/q/${existing.id}`);
  });

  test("posts to Cicala and replaces the form with a receipt", async ({ page }) => {
    let submitted: Record<string, unknown> = {};
    await page.unroute("**/api/suggestions");
    await mockSubmissionApi(page, async (route) => {
      submitted = route.request().postDataJSON() as Record<string, unknown>;
      await route.fulfill({ status: 201, json: { accepted: true, reference: 42 } });
    });
    await page.reload();

    await fillValid(page);
    await page.locator("#s-name").fill("Ada");
    await page.locator("#s-submit").click();

    await expect(page.locator("#s-form")).toBeHidden();
    await expect(page.locator("#s-received")).toBeVisible();
    await expect(page.locator("#s-received-question")).toHaveText(GOOD);
    await expect(page.locator("#s-reference")).toContainText("#42");
    expect(submitted).toMatchObject({ question: GOOD, language: "en", credit: "Ada", cc0: true });
    expect(submitted).not.toHaveProperty("decks");
    expect(submitted).not.toHaveProperty("depth");
    expect(submitted).not.toHaveProperty("tags");
  });

  test("carries a contextual question into the sheet", async ({ page }) => {
    await page.goto(`/suggest?source=browse-empty&text=${encodeURIComponent(GOOD)}`);
    await expect(page.locator("#s-text")).toHaveValue(GOOD);
  });
});

test("a failed API request keeps the question available for retry", async ({ page }) => {
  await mockSubmissionApi(page, async (route) => {
    await route.fulfill({ status: 502, json: { error: "submission service unavailable" } });
  });
  await page.goto("/suggest");
  await fillValid(page);
  await page.locator("#s-submit").click();
  await expect(page.locator("#s-status")).toHaveText(/could not be sent/i);
  await expect(page.locator("#s-text")).toHaveValue(GOOD);
});

test("the legacy contribute URL redirects to the suggestion sheet", async ({ page }) => {
  await mockSubmissionApi(page);
  await page.goto("/contribute");
  await expect.poll(() => new URL(page.url()).pathname).toBe("/suggest");
});
