// @ts-check
import { defineConfig } from "astro/config";

// Static output stays independent of a hosting vendor.
export default defineConfig({
  // Canonical origin until a custom domain is configured.
  site: "https://kveld.pages.dev",
  output: "static",
  trailingSlash: "never",
  build: {
    format: "file",
  },
  vite: {
    build: {
      // Keep small data payloads as separately cacheable files.
      assetsInlineLimit: 0,
    },
  },
});
