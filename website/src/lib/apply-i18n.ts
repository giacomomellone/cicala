// Replace statically rendered English strings for the active language.

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
  document.querySelectorAll<HTMLImageElement>("[data-i18n-alt]").forEach((el) => {
    el.alt = t(lang, el.dataset.i18nAlt as StringKey);
  });
  // Keep <html lang> aligned with the displayed content.
  document.documentElement.lang = lang;
}

export function tr(lang: string, key: StringKey): string {
  return t(isLang(lang) ? lang : "en", key);
}
