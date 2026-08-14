// @vitest-environment happy-dom
// Corrupt tk.* storage values fall back to defaults.
import { beforeEach, describe, expect, it } from "vitest";
import {
  getBag,
  getDeck,
  getFavs,
  isFav,
  setBag,
  setDeck,
  setFavs,
  toggleFav,
} from "../src/lib/store";

beforeEach(() => localStorage.clear());

describe("favorites", () => {
  it("toggles on and off, preserving insertion order", () => {
    expect(toggleFav("q-00000001")).toBe(true);
    expect(toggleFav("q-00000002")).toBe(true);
    expect(getFavs()).toEqual(["q-00000001", "q-00000002"]);
    expect(isFav("q-00000001")).toBe(true);
    expect(toggleFav("q-00000001")).toBe(false);
    expect(getFavs()).toEqual(["q-00000002"]);
  });

  it("survives corrupted storage", () => {
    localStorage.setItem("tk.favs", "{not json");
    expect(getFavs()).toEqual([]);
    localStorage.setItem("tk.favs", JSON.stringify({ nope: 1 }));
    expect(getFavs()).toEqual([]);
    localStorage.setItem("tk.favs", JSON.stringify(["ok", 42, "also-ok"]));
    expect(getFavs()).toEqual(["ok", "also-ok"]);
  });

  it("setFavs round-trips", () => {
    setFavs(["q-aaaaaaaa", "q-bbbbbbbb"]);
    expect(getFavs()).toEqual(["q-aaaaaaaa", "q-bbbbbbbb"]);
  });
});

describe("bags", () => {
  it("round-trips per lang+deck under the tk.bag.* key", () => {
    setBag("de", "wild", { b: ["q-00000001"], r: ["q-00000002"] });
    expect(localStorage.getItem("tk.bag.de.wild")).not.toBeNull();
    expect(getBag("de", "wild")).toEqual({ b: ["q-00000001"], r: ["q-00000002"] });
    // Preserve unrelated storage keys.
    expect(getBag("en", "wild")).toEqual({ b: [], r: [] });
  });

  it("degrades corrupted bags to empty", () => {
    localStorage.setItem("tk.bag.en.all", "not json at all");
    expect(getBag("en", "all")).toEqual({ b: [], r: [] });
  });
});

describe("deck selection", () => {
  it("defaults to new people and persists an absolute deck", () => {
    expect(getDeck()).toBe("new_people");
    setDeck("here");
    expect(getDeck()).toBe("here");
    expect(localStorage.getItem("tk.deck")).toBe("here");
  });
});
