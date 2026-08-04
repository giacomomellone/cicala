/*
 * Text layout: UTF-8 decoding, accent decomposition, and word wrap.
 *
 * No Zephyr headers, so the suite folds every shipped question on the host.
 *
 * ## Why accents are decomposed rather than drawn
 *
 * Zephyr's bundled CFB fonts cover ASCII 32..126 and nothing else, while 74 of
 * the 99 German questions need ß, ä, ö or ü — and French, Spanish and Italian
 * bring another few dozen accented letters with them. Bundling a font with
 * Latin-1 coverage would solve it, at the cost of a font file and its licence.
 *
 * Decomposition avoids both. `é` is an `e` with an acute above it, and the
 * panel already has an `e`; it draws the base glyph from the existing font and
 * the mark as a few pixels above it. Measured from cfb_font_1016 there is room:
 * lowercase glyphs occupy rows 5..12 of the 16-row cell, leaving five blank
 * rows above, and capitals occupy rows 3..12, leaving three. Cedilla goes into
 * the three rows below.
 *
 * So this module turns text into a sequence of Glyphs — a base character the
 * font can draw, plus an optional mark — and the panel renders both parts.
 * Characters that decompose into nothing sensible (ß, œ, æ) fall back to their
 * standard two-letter spellings, which is what German and French do themselves
 * when an accent is unavailable.
 */

#pragma once

#include <stddef.h>
#include <stdint.h>

namespace tk
{

#ifdef CONFIG_TK_PANEL_LINES
constexpr uint8_t kMaxLines = CONFIG_TK_PANEL_LINES;
#else
constexpr uint8_t kMaxLines = 7;
#endif

#ifdef CONFIG_TK_MAX_QUESTION_BYTES
/* One byte of input can become at most one glyph, and a two-letter fallback
 * comes from a two-byte sequence, so the input byte count bounds this. */
constexpr uint16_t kMaxGlyphs = CONFIG_TK_MAX_QUESTION_BYTES;
#else
constexpr uint16_t kMaxGlyphs = 128;
#endif

/** A diacritic the panel draws itself, since the font has no glyph for it. */
enum class Mark : uint8_t {
    NONE = 0,
    ACUTE,      ///< á é í ó ú ý
    GRAVE,      ///< à è ì ò ù
    CIRCUMFLEX, ///< â ê î ô û
    DIAERESIS,  ///< ä ë ï ö ü ÿ
    TILDE,      ///< ã ñ õ
    CEDILLA,    ///< ç — the only one that goes below the baseline
};

/** One character cell: a glyph the font has, plus what to draw over it. */
struct Glyph {
    /** ASCII 32..126. Always something cfb_font_1016 can draw. */
    char base;
    Mark mark;
};

struct Line {
    /** Index into the glyph output array. */
    uint16_t offset;
    /** Cells, which for this font is also the count of glyphs. */
    uint8_t cells;
};

struct Layout {
    Line lines[kMaxLines];
    uint8_t count;
    /** Widest line, in cells. */
    uint8_t width;
    /** The text needed more lines than were available. */
    bool truncated;
    /** Some codepoint had no glyph, no decomposition and no fallback. */
    bool substituted;
};

/**
 * Decode one UTF-8 sequence.
 *
 * @return bytes consumed, or 0 for a malformed, truncated or overlong
 *         sequence.
 */
uint8_t utf8_decode(const char *s, uint16_t len, uint32_t &cp);

/** Codepoints in `text`, or 0 if it is not valid UTF-8. */
uint16_t utf8_length(const char *text, uint16_t len);

/**
 * Turn one codepoint into glyphs the panel can draw.
 *
 * Writes one glyph for anything the font has or that decomposes into base
 * plus mark, and two for the ligature spellings (ß → ss, œ → oe, æ → ae).
 *
 * @param out  at least 2 glyphs of space
 * @return glyphs written, or 0 when the codepoint has no representation.
 */
uint8_t decompose(uint32_t cp, Glyph *out);

/**
 * Fold `text` into lines no wider than `columns`.
 *
 * Wrapping is greedy on spaces, which reads best for one short question and
 * costs nothing. A word too long for a line is broken rather than allowed to
 * overflow — no shipped question needs it, but a device that silently clips
 * text is worse than one that hyphenates badly.
 *
 * @param out  receives the glyphs; Line offsets index into it
 * @return false when `out` is too small or the text is not valid UTF-8
 */
bool wrap(const char *text, uint16_t len, uint8_t columns, Glyph *out, uint16_t out_size,
          Layout &result);

} // namespace tk
