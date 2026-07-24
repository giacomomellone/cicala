// @vitest-environment happy-dom
// Row rendering feeds innerHTML — escaping is load-bearing even though the
// database is reviewed, and hearts must reflect + mutate favorites.
import { beforeEach, describe, expect, it } from "vitest";
import { bindHearts, rowHtml } from "../src/lib/rows";
import { getFavs, setFavs } from "../src/lib/store";

beforeEach(() => localStorage.clear());

const item = {
  q: {
    id: "q-8f3a2c1d",
    text: 'What would <script>alert("x")</script> & "quotes" ask?',
    tags: ["spicy"],
  },
  category: "deep",
};

describe("rowHtml", () => {
  it("escapes question text", () => {
    const html = rowHtml(item, "en");
    expect(html).not.toContain("<script>");
    expect(html).toContain("&lt;script&gt;");
    expect(html).toContain("&amp;");
  });

  it("links to the permalink and shows translated labels", () => {
    const container = document.createElement("ul");
    container.innerHTML = rowHtml(item, "de");
    const link = container.querySelector<HTMLAnchorElement>("a.row-q")!;
    expect(link.getAttribute("href")).toBe("/q/q-8f3a2c1d");
    expect(container.textContent).toContain("tiefgang"); // de label for deep
    expect(container.textContent).toContain("gewagt"); // de label for spicy
  });

  it("renders the heart pressed when the question is a favorite", () => {
    setFavs(["q-8f3a2c1d"]);
    const container = document.createElement("ul");
    container.innerHTML = rowHtml(item, "en");
    expect(
      container.querySelector(".row-fav")!.getAttribute("aria-pressed"),
    ).toBe("true");
  });
});

describe("bindHearts", () => {
  it("toggles the favorite and the aria state on click", () => {
    const container = document.createElement("ul");
    container.innerHTML = rowHtml(item, "en");
    document.body.append(container);
    bindHearts(container);
    const heart = container.querySelector<HTMLButtonElement>(".row-fav")!;
    heart.click();
    expect(getFavs()).toEqual(["q-8f3a2c1d"]);
    expect(heart.getAttribute("aria-pressed")).toBe("true");
    heart.click();
    expect(getFavs()).toEqual([]);
    expect(heart.getAttribute("aria-pressed")).toBe("false");
  });
});
