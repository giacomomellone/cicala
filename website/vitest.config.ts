import { defineConfig } from "vitest/config";

// Keep Playwright's e2e directory out of the happy-dom unit suite.
export default defineConfig({
  test: {
    include: ["tests/**/*.test.ts"],
    environment: "happy-dom",
  },
});
