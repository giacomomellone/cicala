import { chromium } from "@playwright/test";
import { copyFile, mkdir, readFile, writeFile } from "node:fs/promises";
import { dirname, join } from "node:path";
import { fileURLToPath } from "node:url";

const websiteDir = join(dirname(fileURLToPath(import.meta.url)), "..");
const publicDir = join(websiteDir, "public");
const iconDir = join(websiteDir, "src/assets/brand");
const cicada = await readFile(join(publicDir, "brand/cicala-mark.svg"), "utf8");
const paper = "#f46b45";
const ink = "#341e24";

const browser = await chromium.launch();

async function render(path, width, height, body, transparent = false, directory = publicDir) {
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
  await page.screenshot({ path: join(directory, path), omitBackground: transparent });
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
// Import these into the layout so Astro gives every revision a new filename.
// The tab icon uses the same orange circle as the website, with extra size
// for the mark so its eyes remain legible at 16 px.
await mkdir(iconDir, { recursive: true });
const tabIcon = `<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 100 100" width="100%" height="100%">
  <circle cx="50" cy="50" r="50" fill="${paper}"/>
  ${cicada.replace("<svg ", '<svg x="10" y="10" width="80" height="80" ').replaceAll("currentColor", ink)}
</svg>`;
await writeFile(join(iconDir, "favicon.svg"), tabIcon);
for (const size of [16, 32]) {
  const filename = `favicon-${size}.png`;
  await render(filename, size, size, tabIcon, true, iconDir);
  // Keep conventional fallback URLs current for previously opened tabs.
  await copyFile(join(iconDir, filename), join(publicDir, filename));
}
await copyFile(join(publicDir, "apple-touch-icon.png"), join(iconDir, "apple-touch-icon.png"));
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
