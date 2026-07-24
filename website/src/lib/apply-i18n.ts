// Pages are statically rendered in English; when the active language differs,
// this pass swaps every [data-i18n] element's text from src/i18n.ts. Both
// language objects together are ~6 KB — far cheaper than per-language routes
// (path-prefix i18n is explicitly out for v1, spec §7.3).

import { strings, t, type Lang, type StringKey } from "../i18n";

export function isLang(x: string): x is Lang {
  return x in strings;
}

export function applyI18n(lang: string): void {
  if (!isLang(lang)) return;
  document.querySelectorAll<HTMLElement>("[data-i18n]").forEach((el) => {
    el.textContent = t(lang, el.dataset.i18n as StringKey);
  });
  document.querySelectorAll<HTMLElement>("[data-i18n-placeholder]").forEach((el) => {
    el.setAttribute("placeholder", t(lang, el.dataset.i18nPlaceholder as StringKey));
  });
  document.querySelectorAll<HTMLElement>("[data-i18n-aria]").forEach((el) => {
    el.setAttribute("aria-label", t(lang, el.dataset.i18nAria as StringKey));
  });
  // <html lang> reflects the displayed content's language (spec §7.10);
  // on play the payload language and the UI language are always the same.
  document.documentElement.lang = lang;
}

export function tr(lang: string, key: StringKey): string {
  return t(isLang(lang) ? lang : "en", key);
}
