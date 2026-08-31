// The question-writing tips shown one at a time above the suggestion form.
//
// The rules mirror the tips in README.md and are translated like any other UI
// string. The example questions are not: they are quoted from docs/theory.md
// (en) and questions/de/STYLE.md (de). A language appears in `examples` only
// once a fluent maintainer has written its pair, because question text here is
// never machine translated.

import type { Lang, StringKey } from "../i18n";

export interface TipExample {
  weak: string;
  better: string;
}

export interface Tip {
  rule: StringKey;
  examples: Partial<Record<Lang, TipExample>>;
}

export const TIPS: readonly Tip[] = [
  {
    rule: "suggest.tip.build",
    examples: {
      en: {
        weak: "What food do you dislike?",
        better: "Which food do you wish you liked, and what keeps stopping you?",
      },
    },
  },
  {
    rule: "suggest.tip.rankings",
    examples: {
      en: {
        weak: "What's the best book you've ever read?",
        better: "Which book do you still think about at odd moments?",
      },
    },
  },
  {
    rule: "suggest.tip.concrete",
    examples: {
      en: {
        weak: "Do you value honesty?",
        better: "When did you last tell a lie you're still unsure about?",
      },
      de: {
        weak: "Bist du aufgeschlossen?",
        better: "Wann hast du zuletzt deine Meinung zu etwas Wichtigem geändert?",
      },
    },
  },
  {
    rule: "suggest.tip.assumptions",
    examples: {
      en: {
        weak: "Which parent understands you better?",
        better: "Who understood you better than you expected?",
      },
    },
  },
  {
    rule: "suggest.tip.onejob",
    examples: {},
  },
];

/* Wrap an unbounded counter onto the list, in both directions, so the back
   arrow moves from the first tip to the last. */
export function tipIndex(n: number, count: number = TIPS.length): number {
  return ((n % count) + count) % count;
}

export function tipAt(n: number): Tip {
  return TIPS[tipIndex(n)]!;
}

export function tipExample(tip: Tip, lang: Lang): TipExample | null {
  return tip.examples[lang] ?? null;
}
