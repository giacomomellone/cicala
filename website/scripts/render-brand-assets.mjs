import { chromium } from "@playwright/test";
import { readFile } from "node:fs/promises";
import { dirname, join } from "node:path";
import { fileURLToPath } from "node:url";

const websiteDir = join(dirname(fileURLToPath(import.meta.url)), "..");
const publicDir = join(websiteDir, "public");
const font = await readFile(
  join(websiteDir, "node_modules/@fontsource/literata/files/literata-latin-400-normal.woff2"),
  "base64",
);
const sourceMark = await readFile(join(publicDir, "brand/cicala-mark.svg"), "utf8");
const mark = (colour) =>
  sourceMark.replace("<title>Cicala cicada mark</title>", "").replaceAll("#1f1f1d", colour);

const paper = "#faf8f2";
const ink = "#1f1f1d";

const browser = await chromium.launch();

async function render(path, width, height, body, transparent = false) {
  const context = await browser.newContext({
    viewport: { width, height },
    deviceScaleFactor: 1,
  });
  const page = await context.newPage();
  await page.setContent(`
    <style>
      @font-face {
        font-family: Literata;
        font-style: normal;
        font-weight: 400;
        src: url(data:font/woff2;base64,${font}) format("woff2");
      }
      * { box-sizing: border-box; }
      html, body { width: 100%; height: 100%; margin: 0; }
      body { overflow: hidden; }
      svg { display: block; }
    </style>
    ${body}
  `);
  await page.evaluate(() => document.fonts.ready);
  await page.screenshot({ path: join(publicDir, path), omitBackground: transparent });
  await context.close();
}

const lockup = (colour) => `
  <main style="height:100%;display:flex;align-items:center;justify-content:center;color:${colour}">
    <div style="display:flex;align-items:center;gap:44px">
      <div style="width:190px">${mark(colour)}</div>
      <div style="font:400 206px/1 Literata,serif;letter-spacing:-0.035em;padding-bottom:20px">cicala</div>
    </div>
  </main>
`;

await render(
  "brand/cicala-avatar-1024.png",
  1024,
  1024,
  `<main style="height:100%;display:grid;place-items:center;background:${paper}"><div style="width:560px">${mark(ink)}</div></main>`,
);
await render("brand/cicala-wordmark-dark.png", 1019, 378, lockup(ink), true);
await render("brand/cicala-wordmark-reversed.png", 1019, 378, lockup(paper), true);
await render(
  "apple-touch-icon.png",
  180,
  180,
  `<main style="height:100%;display:grid;place-items:center;background:${paper}"><div style="width:112px">${mark(ink)}</div></main>`,
);
await render(
  "favicon-32.png",
  32,
  32,
  `<main style="height:100%;display:grid;place-items:center;background:${paper}"><div style="width:27px">${mark(ink)}</div></main>`,
);
await render(
  "favicon-16.png",
  16,
  16,
  `<main style="height:100%;display:grid;place-items:center;background:${paper}"><div style="width:15px">${mark(ink)}</div></main>`,
);
await render(
  "og.png",
  1200,
  630,
  `<main style="height:100%;display:flex;flex-direction:column;align-items:center;justify-content:center;gap:42px;background:${paper};color:${ink}">
    <div style="display:flex;align-items:center;gap:34px">
      <div style="width:132px">${mark(ink)}</div>
      <div style="font:400 152px/1 Literata,serif;letter-spacing:-0.035em;padding-bottom:16px">cicala</div>
    </div>
    <div style="font:400 42px/1.25 Literata,serif">one question to break the silence.</div>
  </main>`,
);

await browser.close();
