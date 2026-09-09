import { describe, expect, it } from "vitest";
import { DEFAULT_PERMISSIONS, RECENT_WINDOW, FORMS, LANGUAGES, TAGS } from "../src/config";
import schema from "../../questions/schema.json";
const cfg = schema["x-cicala"];
describe("the schema is the source of truth", () => {
  it("agrees on permissions, recent window and vocabulary", () => {
    expect(DEFAULT_PERMISSIONS).toEqual(cfg.defaultPermissions);
    expect(RECENT_WINDOW).toBe(cfg.recentWindow);
    expect([...TAGS]).toEqual(cfg.tags);
    expect([...FORMS]).toEqual(cfg.tags.filter((t) => !["dark", "sexual"].includes(t)));
    expect(schema.items.properties).not.toHaveProperty("decks");
  });
  it("agrees on shipped languages", () => {
    expect(LANGUAGES).toEqual(
      Object.entries(cfg.languages).map(([code, language]) => ({ code, name: language.name })),
    );
  });
});
