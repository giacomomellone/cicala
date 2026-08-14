import { defineConfig } from "vitest/config";

// Unit tests live in tests/ and run in happy-dom. e2e/ is Playwright's — it
// drives a real browser against the built site and must not be collected here.
export default defineConfig({
  test: {
    include: ["tests/**/*.test.ts"],
    environment: "happy-dom",
  },
});
