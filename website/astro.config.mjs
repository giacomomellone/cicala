// @ts-check
import { defineConfig } from "astro/config";

// Host-agnostic static build (spec §7.1): no adapters, no vendor features.
// The site URL is a placeholder until the real domain exists — see
// docs/DECISIONS.md "Placeholder org and domain".
export default defineConfig({
  site: "https://tischkarte.pages.dev",
  output: "static",
  trailingSlash: "never",
  build: {
    format: "file",
  },
  vite: {
    build: {
      // keep every data payload an addressable, cacheable file — never a
      // base64 inline (the per-language recent lists sit under Vite's
      // default 4 KB inline threshold)
      assetsInlineLimit: 0,
    },
  },
});
