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
const paper = "#dedad2";
const ink = "#100f0e";

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

/* The identity is typographic, so square formats are the wordmark's own
   lowercase c rather than a symbol. It is drawn on a canvas because only
   TextMetrics reports where the ink actually is: a c has no ascender and no
   descender, so centring its line box leaves it small and sitting low. */
async function renderGlyph(path, size) {
  const context = await browser.newContext({
    viewport: { width: size, height: size },
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
      html, body { width: 100%; height: 100%; margin: 0; overflow: hidden; }
      canvas { display: block; }
    </style>
    <canvas width="${size}" height="${size}"></canvas>
  `);
  await page.evaluate(
    ({ size, paper, ink }) =>
      document.fonts.ready.then(() => {
        const ctx = document.querySelector("canvas").getContext("2d");
        const measure = (px) => {
          ctx.font = `400 ${px}px Literata`;
          const m = ctx.measureText("c");
          return {
            w: m.actualBoundingBoxLeft + m.actualBoundingBoxRight,
            h: m.actualBoundingBoxAscent + m.actualBoundingBoxDescent,
            left: m.actualBoundingBoxLeft,
            descent: m.actualBoundingBoxDescent,
          };
        };

        /* Fill 62% of the square, keeping the clear space the guide asks for. */
        const probe = measure(100);
        const fontPx = Math.round((size * 0.62 * 100) / Math.max(probe.w, probe.h));
        const m = measure(fontPx);

        ctx.fillStyle = paper;
        ctx.fillRect(0, 0, size, size);
        ctx.fillStyle = ink;
        ctx.fillText("c", size / 2 - m.w / 2 + m.left, size / 2 + m.h / 2 - m.descent);
      }),
    { size, paper, ink },
  );
  await page.screenshot({ path: join(publicDir, path) });
  await context.close();
}

const lockup = (colour) => `
  <main style="height:100%;display:flex;align-items:center;justify-content:center;color:${colour}">
    <div style="font:400 206px/1 Literata,serif;letter-spacing:-0.035em">cicala</div>
  </main>
`;

await renderGlyph("brand/cicala-avatar-1024.png", 1024);
await render("brand/cicala-wordmark-dark.png", 1019, 378, lockup(ink), true);
await render("brand/cicala-wordmark-reversed.png", 1019, 378, lockup(paper), true);
await renderGlyph("apple-touch-icon.png", 180);
await renderGlyph("favicon-32.png", 32);
await renderGlyph("favicon-16.png", 16);
await render(
  "og.png",
  1200,
  630,
  `<main style="height:100%;display:flex;flex-direction:column;align-items:center;justify-content:center;gap:42px;background:${paper};color:${ink}">
    <div style="font:400 152px/1 Literata,serif;letter-spacing:-0.035em">cicala</div>
    <div style="font:400 42px/1.25 Literata,serif">let's talk.</div>
  </main>`,
);

await browser.close();
