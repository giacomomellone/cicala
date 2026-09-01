import { devices, expect, test } from "@playwright/test";
import { payload, playable, playReady, schema, seedStorage, smallestDeck } from "./fixtures";

const questionText = "#q-text";

test.describe("play", () => {
  test("publishes the live lockup and identity assets", async ({ page }) => {
    await page.setViewportSize({ width: 375, height: 812 });
    await page.addInitScript(() => {
      (window as Window & { identityLayoutShift?: number }).identityLayoutShift = 0;
      new PerformanceObserver((list) => {
        for (const entry of list.getEntries()) {
          const shift = entry as PerformanceEntry & { hadRecentInput: boolean; value: number };
          if (!shift.hadRecentInput) {
            (window as Window & { identityLayoutShift: number }).identityLayoutShift += shift.value;
          }
        }
      }).observe({ type: "layout-shift", buffered: true });
    });
    await page.goto(`/q/${payload("en")[0]!.id}`);

    const wordmark = page.getByRole("link", { name: "Cicala home" });
    await expect(wordmark).toHaveText("cicala");
    await expect(wordmark).toHaveCSS("font-family", /Literata/);
    await expect(wordmark.locator("svg")).toHaveCount(0);
    // The separators are the first thing a narrow header drops; they must not.
    await expect(page.locator(".site-nav .nav-dot").first()).toBeVisible();
    await wordmark.focus();
    await expect(wordmark).toBeFocused();

    await expect(page.locator("#q-text")).toHaveCSS("font-family", /Zilla Slab/);

    const iconHrefs = await page
      .locator('link[rel="icon"]')
      .evaluateAll((links) => links.map((link) => (link as HTMLLinkElement).href));
    expect(iconHrefs).toHaveLength(2);
    for (const href of iconHrefs) {
      const response = await page.request.get(href);
      expect(response.ok()).toBe(true);
      expect(response.headers()["content-type"]).toContain("image/png");
    }

    const touchHref = await page.locator('link[rel="apple-touch-icon"]').getAttribute("href");
    const ogHref = await page.locator('meta[property="og:image"]').getAttribute("content");
    for (const href of [touchHref, ogHref]) {
      expect(href).toBeTruthy();
      const asset = new URL(new URL(href!, page.url()).pathname, page.url()).href;
      const response = await page.request.get(asset);
      expect(response.ok()).toBe(true);
      expect(response.headers()["content-type"]).toContain("image/png");
    }

    const sizes = await page.evaluate(() => ({
      viewport: window.innerWidth,
      page: document.documentElement.scrollWidth,
      wordmark: Number.parseFloat(getComputedStyle(document.querySelector(".wordmark")!).fontSize),
      question: Number.parseFloat(getComputedStyle(document.querySelector("#q-text")!).fontSize),
    }));
    expect(sizes.page).toBeLessThanOrEqual(sizes.viewport);
    expect(sizes.question).toBeGreaterThan(sizes.wordmark);

    const fontState = await page.evaluate(async () => {
      await document.fonts.ready;
      const externalResources = performance
        .getEntriesByType("resource")
        .map((entry) => new URL(entry.name))
        .filter((url) => url.origin !== location.origin)
        .map((url) => url.href);
      return {
        literata: document.fonts.check("16px Literata"),
        zilla: document.fonts.check('16px "Zilla Slab"'),
        plex: document.fonts.check('16px "IBM Plex Mono"'),
        externalResources,
        layoutShift: (window as Window & { identityLayoutShift?: number }).identityLayoutShift ?? 0,
      };
    });
    expect(fontState.literata).toBe(true);
    expect(fontState.zilla).toBe(true);
    expect(fontState.plex).toBe(true);
    expect(fontState.externalResources).toEqual([]);
    expect(fontState.layoutShift).toBeLessThan(0.01);
  });

  test("serves a question without JavaScript", async ({ browser }) => {
    const context = await browser.newContext({ javaScriptEnabled: false });
    const page = await context.newPage();
    await page.goto("/");
    await expect(page.locator(questionText)).not.toBeEmpty();
    await context.close();
  });

  test("keeps the suggestion entry point outside the panel controls", async ({ page }) => {
    await page.goto("/");
    const link = page.locator('a[href="/suggest?source=play"]');
    await expect(link).toBeVisible();
    await expect(page.locator('.panel a[href="/suggest?source=play"]')).toHaveCount(0);
    await expect(page.locator('.device-controls a[href="/suggest?source=play"]')).toHaveCount(0);
  });

  test("the phone trades the labels for a full-bleed sheet", async ({ page }) => {
    await page.goto("/");
    const panel = page.locator(".panel");
    const skipNote = page.locator(".skip-note");
    const language = page.locator("#lang-switch button").first();

    await expect(skipNote).toContainText(/feel free to skip/i);
    await expect(panel).toHaveCSS("border-left-width", "1px");
    await expect(page.locator("#q-fav-label")).toBeVisible();
    await expect(language.locator(".lang-full")).toBeVisible();
    await expect(language.locator(".lang-code")).toBeHidden();

    await page.setViewportSize({ width: 375, height: 812 });

    await expect(skipNote).toBeVisible();
    await expect(panel).toHaveCSS("border-left-width", "0px");
    await expect(language.locator(".lang-full")).toBeHidden();
    await expect(language.locator(".lang-code")).toBeVisible();
  });

  test("the card carries no blanket provenance note, only the footer", async ({ page }) => {
    await page.goto("/");
    await playReady(page);

    await expect(page.locator(".panel__stamp")).toHaveCount(0);
    await expect(page.locator(".play .disclaimer")).toHaveCount(0);
    await expect(page.locator(".play .hint")).toHaveCount(0);
    await expect(page.locator(".footer-license")).toContainText(/human originals/i);

    // Every shipped question is human-written today, so nothing is marked.
    await expect(page.locator("#q-origin")).toBeHidden();
  });

  test("the shortcut rides its key and leaves the label centred", async ({ page }) => {
    await page.goto("/");
    const next = page.locator("#q-next");

    await expect(next.locator(".key__kbd")).toHaveText("space");
    await expect(page.locator("#q-category .key__kbd")).toHaveText("c");

    // Absolute, so the label keeps the key's centre rather than shifting up.
    await expect(next.locator(".key__kbd")).toHaveCSS("position", "absolute");
  });

  /* The shortcut hides on `pointer: coarse`, which a resized desktop viewport
     does not report — it needs a touch context. */
  test("a touch device is not offered a keyboard shortcut", async ({ browser }) => {
    const context = await browser.newContext({ ...devices["Pixel 5"] });
    const page = await context.newPage();
    await page.goto("/");

    await expect(page.locator("#q-next .key__kbd")).toBeHidden();
    await expect(page.locator("#q-next")).toContainText(/next question/i);

    await context.close();
  });

  test("next draws a different question", async ({ page }) => {
    await page.goto("/");
    await playReady(page);
    const first = await page.locator(questionText).innerText();
    await page.getByRole("button", { name: /next question/i }).click();
    await expect(page.locator(questionText)).not.toHaveText(first);
  });

  test("space advances and arrow-left walks back through history", async ({ page }) => {
    await page.goto("/");
    await playReady(page);
    const first = await page.locator(questionText).innerText();
    await page.locator("body").press(" ");
    await expect(page.locator(questionText)).not.toHaveText(first);
    const second = await page.locator(questionText).innerText();

    await page.locator("body").press("ArrowLeft");
    await expect(page.locator(questionText)).toHaveText(first);
    await page.locator("body").press("ArrowRight");
    await expect(page.locator(questionText)).toHaveText(second);
  });

  test("the shuffle bag never repeats before the deck is exhausted", async ({ page }) => {
    const deck = smallestDeck("en");
    const expected = playable("en", deck);
    await seedStorage(page, { "cicala.deck": deck });
    await page.goto("/");
    await playReady(page);

    const seen: string[] = [await page.locator(questionText).innerText()];
    for (let i = 1; i < expected.length; i++) {
      await page.getByRole("button", { name: /next question/i }).click();
      await expect(page.locator(questionText)).not.toHaveText(seen.at(-1)!);
      seen.push(await page.locator(questionText).innerText());
    }

    expect(new Set(seen).size).toBe(expected.length);
    const texts = new Set(expected.map((q) => q.text));
    for (const shown of seen) expect(texts).toContain(shown);
  });

  test("the category button cycles the five decks, skipping work, and wraps", async ({ page }) => {
    await page.goto("/");
    await playReady(page);
    const category = page.getByRole("button", { name: /^category$/i });

    // From the New People default, one full turn lands back on it.
    for (const name of ["close", "family", "here", "wild", "new people"]) {
      await category.click();
      await expect(page.locator(questionText)).toHaveText(name);
    }
  });

  test("a category choice sticks and only serves that deck", async ({ page }) => {
    await page.goto("/");
    await playReady(page);
    const category = page.getByRole("button", { name: /^category$/i });
    await category.click(); // close
    await category.click(); // family
    await expect(page.locator(questionText)).toHaveText("family");
    await expect
      .poll(() => page.evaluate(() => localStorage.getItem("cicala.deck")))
      .toBe("family");

    const eligible = new Set(playable("en", "family").map((q) => q.text));
    for (let i = 0; i < 5; i++) {
      await page.getByRole("button", { name: /next question/i }).click();
      await expect
        .poll(async () => eligible.has(await page.locator(questionText).innerText()))
        .toBe(true);
    }
  });

  test("the player never serves a question above the playback depth", async ({ page }) => {
    const deck = smallestDeck("en");
    const tooDeep = new Set(
      payload("en")
        .filter((q) => q.depth > schema.playbackDepthMax && q.decks.includes(deck))
        .map((q) => q.text),
    );
    test.skip(tooDeep.size === 0, `no depth-3 question in the ${deck} deck`);

    await seedStorage(page, { "cicala.deck": deck });
    await page.goto("/");
    await playReady(page);
    for (let i = 0; i < playable("en", deck).length; i++) {
      expect(tooDeep).not.toContain(await page.locator(questionText).innerText());
      await page.getByRole("button", { name: /next question/i }).click();
    }
  });

  test("saving a question stores it and survives a reload", async ({ page }) => {
    await page.goto("/");
    await playReady(page);
    const shown = await page.locator(questionText).innerText();
    const id = payload("en").find((q) => q.text === shown)!.id;

    await page.locator("#q-fav").click();
    await expect(page.locator("#q-fav")).toHaveAttribute("aria-pressed", "true");
    await expect.poll(() => page.evaluate(() => localStorage.getItem("cicala.favs"))).toContain(id);

    await page.goto(`/q/${id}`);
    await expect(page.locator("#q-fav")).toHaveAttribute("aria-pressed", "true");
  });

  test("share copies the permalink of the question on screen", async ({ page }) => {
    await page.goto("/");
    await playReady(page);
    const shown = await page.locator(questionText).innerText();
    const id = payload("en").find((q) => q.text === shown)!.id;

    await page.locator("#q-share").click();
    await expect(page.locator("#q-share-label")).toHaveText(/copied/i);
    const clipboard = await page.evaluate(() => navigator.clipboard.readText());
    expect(clipboard).toBe(`${new URL(page.url()).origin}/q/${id}`);
  });

  test("keys do nothing while a form control has focus", async ({ page }) => {
    await page.goto("/browse");
    const before = await page.locator("#b-search").inputValue();
    await page.locator("#b-search").focus();
    await page.keyboard.type(" ");
    expect(await page.locator("#b-search").inputValue()).toBe(`${before} `);
  });
});

test.describe("permalink", () => {
  test("renders the question it names and adopts its language", async ({ page }) => {
    const question = payload("de")[0]!;
    await page.goto(`/q/${question.id}`);
    await expect(page.locator(questionText)).toHaveText(question.text);
    await expect.poll(() => page.evaluate(() => localStorage.getItem("cicala.lang"))).toBe("de");
  });

  test("next leaves the permalink for normal play", async ({ page }) => {
    const question = payload("en")[0]!;
    await page.goto(`/q/${question.id}`);
    await expect(page.locator(questionText)).toHaveText(question.text);
    await page.getByRole("button", { name: /next question/i }).click();
    await expect.poll(() => new URL(page.url()).pathname).toBe("/");
  });

  test("an unknown id is a 404", async ({ page }) => {
    const response = await page.goto("/q/q-00000000");
    expect(response?.status()).toBe(404);
  });
});
