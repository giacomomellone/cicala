// @vitest-environment happy-dom
// Corrupt cicala.* storage values fall back to defaults.
import { beforeEach, describe, expect, it } from "vitest";
import { getFavs, isFav, setFavs, toggleFav } from "../src/lib/store";

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
    localStorage.setItem("cicala.favs", "{not json");
    expect(getFavs()).toEqual([]);
    localStorage.setItem("cicala.favs", JSON.stringify({ nope: 1 }));
    expect(getFavs()).toEqual([]);
    localStorage.setItem("cicala.favs", JSON.stringify(["ok", 42, "also-ok"]));
    expect(getFavs()).toEqual(["ok", "also-ok"]);
  });

  it("setFavs round-trips", () => {
    setFavs(["q-aaaaaaaa", "q-bbbbbbbb"]);
    expect(getFavs()).toEqual(["q-aaaaaaaa", "q-bbbbbbbb"]);
  });
});
