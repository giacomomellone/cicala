// Editorial hints for the suggestion form, advisory only.
//
// A hint names what a question is doing and stops. It never proposes wording:
// only people write questions here. It never blocks submission either, because
// the wordlists below also match questions maintainers have accepted.

import type { Lang } from "../i18n";
import { collapse } from "./rules";

export type StyleHint = "ranking" | "stacked" | "long" | null;

/* The length the style guides aim for, well inside the 140-character limit. */
const TARGET_LEN = 95;

/* Superlative frames, narrow on purpose: a bare /best|most/ matches a quarter
   of the accepted corpus. Unlike the mechanical rules, these are per language. */
const RANKING: Record<Lang, RegExp> = {
  en: /\b(the (best|worst)|your favou?rite)\b/i,
  de: /\b(beste[rsn]?|schlimmste[rsn]?)\b/i,
};

/* A superlative followed by ", and …" asks for a story rather than a verdict,
   which is the two-part shape both style guides recommend. */
const FOLLOW_UP = /,\s(and|und)\s/i;

/* The first editorial problem with a question, or null. One at a time: a
   contributor who is already fixing the mechanics gets no second complaint. */
export function styleHint(raw: string, lang: Lang): StyleHint {
  const text = collapse(raw);
  if (text.length === 0) return null;
  if (RANKING[lang].test(text) && !FOLLOW_UP.test(text)) return "ranking";
  if ((text.match(/\?/g) ?? []).length > 1) return "stacked";
  if (text.length > TARGET_LEN) return "long";
  return null;
}
