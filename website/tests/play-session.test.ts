// @vitest-environment happy-dom
import { beforeEach, expect, it } from "vitest";
import { getPlaySession, setPlaySession } from "../src/lib/play-session";
beforeEach(() => {
  sessionStorage.clear();
  localStorage.clear();
});
it("defaults off and ignores obsolete category bags", () => {
  localStorage.setItem("cicala.deck", "wild");
  localStorage.setItem("cicala.bag.en.wild", JSON.stringify({ b: ["a"], r: ["b"] }));
  const state = getPlaySession("en", "v1");
  expect(Object.values(state.permissions)).toEqual([false, false, false]);
  expect(state.bag.seen).toEqual([]);
});
it("retains a draft and selected row across reload without applying it", () => {
  const state = getPlaySession("en", "v1");
  state.menu = true;
  state.cursor = 2;
  state.draft.sexual = true;
  state.bag.seen = ["a"];
  state.shown = "a";
  setPlaySession("en", state);
  expect(getPlaySession("en", "v1")).toEqual(state);
  expect(getPlaySession("en", "v1").permissions.sexual).toBe(false);
});
it("resets corpus-dependent state on version change while keeping explicit permissions", () => {
  const state = getPlaySession("en", "v1");
  state.permissions.dark = true;
  state.bag.seen = ["a"];
  state.shown = "a";
  setPlaySession("en", state);
  const next = getPlaySession("en", "v2");
  expect(next.bag.seen).toEqual([]);
  expect(next.shown).toBe("");
  expect(next.permissions.dark).toBe(true);
  expect(getPlaySession("de", "v1").bag.seen).toEqual([]);
});
it("clears permissions with a new session and tolerates corrupt storage", () => {
  sessionStorage.setItem("cicala.permissions.v1", '{"dark":"true"}');
  sessionStorage.setItem("cicala.play.v1.en", "{bad");
  expect(getPlaySession("en", "v1").permissions.dark).toBe(false);
});
