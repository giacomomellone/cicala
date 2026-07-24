// Client-side question rules (spec §7.7) — a faithful mirror of
// tools/validate.py: 10–140 characters, single line, ends with a "?"
// optionally followed by one closing quote/bracket.

export const TEXT_MIN = 10;
export const TEXT_MAX = 140;

// terminator + optional closing character, mirroring schema.json's
// defaultTerminators + closingCharacters
export const ENDS_OK = /\?["'”’»«)\]]?$/;

export type TextIssue = "short" | "long" | "mark" | null;

/** Returns the first problem with a question text, or null if it's valid. */
export function questionTextIssue(raw: string): TextIssue {
  const n = raw.length;
  if (n < TEXT_MIN) return "short";
  if (n > TEXT_MAX) return "long";
  if (/\n/.test(raw)) return "mark";
  const collapsed = raw.replace(/\s+/g, " ").trim();
  if (!ENDS_OK.test(collapsed)) return "mark";
  return null;
}
