import { chromium } from "@playwright/test";
import { readFile } from "node:fs/promises";
import { dirname, join } from "node:path";
import { fileURLToPath } from "node:url";

const websiteDir = join(dirname(fileURLToPath(import.meta.url)), "..");
const publicDir = join(websiteDir, "public");
const cicada = await readFile(join(publicDir, "brand/cicala-mark.svg"), "utf8");
const paper = "#f46b45";
const ink = "#341e24";

const browser = await chromium.launch();

async function render(path, width, height, body, transparent = false) {
  const context = await browser.newContext({
    viewport: { width, height },
    deviceScaleFactor: 1,
  });
  const page = await context.newPage();
  await page.setContent(`
    <style>
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

const mark = (colour, size) =>
  cicada
    .replace("<svg ", `<svg style="width:${size}px;height:${size}px;flex:none" `)
    .replaceAll("currentColor", colour);

async function renderMark(path, size) {
  const artworkSize = size <= 32 ? size : Math.round(size * 0.6);
  await render(
    path,
    size,
    size,
    `<main style="height:100%;display:flex;align-items:center;justify-content:center;background:${paper}">
      ${mark(ink, artworkSize)}
    </main>`,
  );
}

const lockup = (colour) => `
  <main style="height:100%;display:flex;align-items:center;justify-content:center;color:${colour}">
    <div style="font:900 206px/1 Arial,Helvetica,sans-serif;letter-spacing:-0.075em">cicala</div>
  </main>
`;

await renderMark("brand/cicala-avatar-1024.png", 1024);
await render("brand/cicala-wordmark-dark.png", 1019, 378, lockup(ink), true);
await render("brand/cicala-wordmark-reversed.png", 1019, 378, lockup(paper), true);
const logo = (colour) => `
  <main style="height:100%;display:flex;align-items:center;justify-content:center;gap:30px;color:${colour}">
    ${mark(colour, 206)}
    <div style="font:900 206px/1 Arial,Helvetica,sans-serif;letter-spacing:-0.075em">cicala</div>
  </main>
`;
await render("brand/cicala-logo-dark.png", 1019, 378, logo(ink), true);
await render("brand/cicala-logo-reversed.png", 1019, 378, logo(paper), true);
await renderMark("apple-touch-icon.png", 180);
await renderMark("favicon-32.png", 32);
await renderMark("favicon-16.png", 16);
await render(
  "og.png",
  1200,
  630,
  `<main style="height:100%;display:flex;flex-direction:column;align-items:center;justify-content:center;gap:42px;background:${paper};color:${ink}">
    <div style="display:flex;align-items:center;gap:24px">
      ${mark(ink, 152)}
      <div style="font:900 180px/1 Arial,Helvetica,sans-serif;letter-spacing:-0.075em">cicala</div>
    </div>
    <div style="font:700 42px/1.1 Arial,Helvetica,sans-serif">Grab a chair.</div>
  </main>`,
);

await browser.close();
