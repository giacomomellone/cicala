import { expect, test, type Page, type Route } from "@playwright/test";
import { payload, skipWithoutQuestions } from "./fixtures";

const target = () => payload("en")[0]!;
/* Mechanically valid, absent from the database, and not the target's wording. */
const REWORDED = "Which unremarkable Tuesday would you happily live again?";

async function mockEditApi(page: Page, post?: (route: Route) => Promise<void>) {
  await page.route("**/api/edits", async (route) => {
    if (route.request().method() === "GET") {
      await route.fulfill({
        json: {
          available: true,
          turnstileRequired: false,
          turnstileSiteKey: "",
          turnstileAction: "suggest-edit",
        },
      });
      return;
    }
    if (post) await post(route);
    else await route.fulfill({ status: 201, json: { accepted: true, reference: 77 } });
  });
}

test.describe("native edit form", () => {
  test("reaches the form from a permalink without leaving the site", async ({ page }) => {
    skipWithoutQuestions("en");
    const question = target();
    await mockEditApi(page);
    await page.goto(`/q/${question.id}`);
    await page.locator("#q-edit a").click();

    await expect(page).toHaveURL(new RegExp(`/edit\\?q=${question.id}$`));
    await expect(page.locator("#e-current")).toHaveText(question.text);
  });

  test("refuses an id that is not in the database", async ({ page }) => {
    await mockEditApi(page);
    await page.goto("/edit?q=q-00000000");
    await expect(page.locator("#e-notfound")).toBeVisible();
    await expect(page.locator("#e-form")).toBeHidden();
  });

  test("needs a reason plus either wording or a metadata box", async ({ page }) => {
    skipWithoutQuestions("en");
    const question = target();
    await mockEditApi(page);
    await page.goto(`/edit?q=${question.id}`);

    await expect(page.locator("#e-submit")).toBeDisabled();
    await page.locator("#e-reason").fill("This asks for a ranking rather than a memory.");
    await expect(page.locator("#e-submit")).toBeDisabled();
    await expect(page.locator("#e-nothing")).toBeVisible();

    await page.locator("#e-aspect-depth").check();
    await expect(page.locator("#e-submit")).toBeEnabled();
  });

  test("asks for consent only once wording is proposed", async ({ page }) => {
    skipWithoutQuestions("en");
    const question = target();
    await mockEditApi(page);
    await page.goto(`/edit?q=${question.id}`);
    await page.locator("#e-reason").fill("This asks for a ranking rather than a memory.");
    await page.locator("#e-aspect-depth").check();
    await expect(page.locator("#e-consent")).toBeHidden();

    await page.locator("#e-text").fill(REWORDED);
    await expect(page.locator("#e-consent")).toBeVisible();
    await expect(page.locator("#e-submit")).toBeDisabled();
    await page.locator("#e-human").check();
    await page.locator("#e-cc0").check();
    await expect(page.locator("#e-submit")).toBeEnabled();
  });

  test("flags wording that is unchanged or already in the database", async ({ page }) => {
    skipWithoutQuestions("en");
    const question = target();
    const other = payload("en")[1]!;
    await mockEditApi(page);
    await page.goto(`/edit?q=${question.id}`);

    await page.locator("#e-text").fill(question.text);
    await expect(page.locator("#e-rule")).toHaveText(/current wording/i);

    await page.locator("#e-text").fill(other.text);
    await expect(page.locator("#e-rule")).toHaveText(/already/i);
    await expect(page.locator("#e-dupe-link")).toHaveAttribute("href", `/q/${other.id}`);

    await page.locator("#e-text").fill("short?");
    await expect(page.locator("#e-rule")).toHaveText(/at least 10/i);
  });

  test("posts the edit and replaces the form with a receipt", async ({ page }) => {
    skipWithoutQuestions("en");
    const question = target();
    let submitted: Record<string, unknown> = {};
    await mockEditApi(page, async (route) => {
      submitted = route.request().postDataJSON() as Record<string, unknown>;
      await route.fulfill({ status: 201, json: { accepted: true, reference: 77 } });
    });
    await page.goto(`/edit?q=${question.id}`);

    await page.locator("#e-text").fill(REWORDED);
    await page.locator("#e-reason").fill("This asks for a ranking rather than a memory.");
    await page.locator("#e-aspect-tags").check();
    await page.locator("#e-human").check();
    await page.locator("#e-cc0").check();
    await page.locator("#e-submit").click();

    await expect(page.locator("#e-received")).toBeVisible();
    await expect(page.locator("#e-form")).toBeHidden();
    await expect(page.locator("#e-reference")).toContainText("77");

    expect(submitted.questionId).toBe(question.id);
    expect(submitted.language).toBe("en");
    expect(submitted.proposedText).toBe(REWORDED);
    expect(submitted.aspects).toEqual(["tags"]);
    expect(submitted.humanWritten).toBe(true);
    expect(submitted.cc0).toBe(true);
  });

  test("reports an unavailable endpoint instead of failing silently", async ({ page }) => {
    skipWithoutQuestions("en");
    await page.route("**/api/edits", (route) =>
      route.fulfill({ json: { available: false, turnstileRequired: false } }),
    );
    await page.goto(`/edit?q=${target().id}`);
    await expect(page.locator("#e-status")).toHaveText(/unavailable/i);
    await expect(page.locator("#e-submit")).toBeDisabled();
  });
});
