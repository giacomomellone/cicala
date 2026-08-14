// Client-side mirror of the question-text validator.

export const TEXT_MIN = 10;
export const TEXT_MAX = 140;

// Match a configured terminator followed by an optional closing character.
export const ENDS_OK = /\?["'”’»«)\]]?$/;

export type TextIssue = "short" | "long" | "multiline" | "mark" | null;

/* Normalize whitespace as promote_issue.py stores it. */
export function collapse(raw: string): string {
  return raw.replace(/\s+/g, " ").trim();
}

/* Returns the first problem with a question text, or null if it's valid. */
export function questionTextIssue(raw: string): TextIssue {
  const text = collapse(raw);
  if (text.length < TEXT_MIN) return "short";
  if (text.length > TEXT_MAX) return "long";
  if (/\n/.test(raw)) return "multiline";
  if (!ENDS_OK.test(text)) return "mark";
  return null;
}

/* Match validate.py normalization: NFC, lowercase, and collapsed whitespace. */
export function normalizeQuestion(raw: string): string {
  return collapse(raw.normalize("NFC").toLowerCase());
}

/* Index a language's corpus by normalized text, for the duplicate check. */
export function buildTextIndex(
  questions: readonly { id: string; text: string }[],
): Map<string, string> {
  return new Map(questions.map((q) => [normalizeQuestion(q.text), q.id]));
}

/* The id of the question this text duplicates, or null. */
export function duplicateOf(raw: string, index: Map<string, string>): string | null {
  return index.get(normalizeQuestion(raw)) ?? null;
}
