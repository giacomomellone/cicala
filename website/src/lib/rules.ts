// Client-side question rules (spec §7.7) — a faithful mirror of
// tools/validate.py: 10–140 characters, single line, ends with a "?"
// optionally followed by one closing quote/bracket.

export const TEXT_MIN = 10;
export const TEXT_MAX = 140;

// terminator + optional closing character, mirroring schema.json's
// defaultTerminators + closingCharacters
export const ENDS_OK = /\?["'”’»«)\]]?$/;

export type TextIssue = "short" | "long" | "multiline" | "mark" | null;

/** What the database would store for this input: runs of whitespace collapse
 * to one space and the ends are trimmed, the same as promote_issue.py does
 * before it writes the entry. Every rule is measured against this, so padding
 * a text to 141 characters with spaces is not "too long". */
export function collapse(raw: string): string {
  return raw.replace(/\s+/g, " ").trim();
}

/** Returns the first problem with a question text, or null if it's valid. */
export function questionTextIssue(raw: string): TextIssue {
  const text = collapse(raw);
  if (text.length < TEXT_MIN) return "short";
  if (text.length > TEXT_MAX) return "long";
  if (/\n/.test(raw)) return "multiline";
  if (!ENDS_OK.test(text)) return "mark";
  return null;
}

/** The form of a question the database compares on — validate.py's
 * normalize_text: NFC, lowercased, whitespace collapsed. Two texts that
 * normalize the same are the same question. */
export function normalizeQuestion(raw: string): string {
  return collapse(raw.normalize("NFC").toLowerCase());
}

/** Index a language's corpus by normalized text, for the duplicate check. */
export function buildTextIndex(
  questions: readonly { id: string; text: string }[],
): Map<string, string> {
  return new Map(questions.map((q) => [normalizeQuestion(q.text), q.id]));
}

/** The id of the question this text duplicates, or null. Languages are
 * independent corpora, so the index must be the active language's. */
export function duplicateOf(
  raw: string,
  index: Map<string, string>,
): string | null {
  return index.get(normalizeQuestion(raw)) ?? null;
}
