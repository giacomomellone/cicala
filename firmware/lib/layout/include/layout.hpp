/* UTF-8 decoding, accent decomposition, and word wrapping for the panel. */

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
// Input byte count bounds the number of decomposed glyphs.
constexpr uint16_t kMaxGlyphs = CONFIG_TK_MAX_QUESTION_BYTES;
#else
constexpr uint16_t kMaxGlyphs = 128;
#endif

/** A diacritic drawn over or under a base glyph. */
enum class Mark : uint8_t {
    NONE = 0,
    ACUTE,      ///< á é í ó ú ý
    GRAVE,      ///< à è ì ò ù
    CIRCUMFLEX, ///< â ê î ô û
    DIAERESIS,  ///< ä ë ï ö ü ÿ
    TILDE,      ///< ã ñ õ
    CEDILLA,    ///< ç — the only one that goes below the baseline
};

/** One available font glyph and its optional diacritic. */
struct Glyph {
    /** ASCII 32..126. */
    char base;
    Mark mark;
};

struct Line {
    /** Index into the glyph output array. */
    uint16_t offset;
    /** Number of glyph cells. */
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

/** Decode one UTF-8 sequence. Return bytes consumed, or zero on error. */
uint8_t utf8_decode(const char *s, uint16_t len, uint32_t &cp);

/** Codepoints in `text`, or 0 if it is not valid UTF-8. */
uint16_t utf8_length(const char *text, uint16_t len);

/** Decompose one codepoint into at most two drawable glyphs. */
uint8_t decompose(uint32_t cp, Glyph *out);

/** Greedily wrap text. Return false for invalid UTF-8 or a small output buffer. */
bool wrap(const char *text, uint16_t len, uint8_t columns, Glyph *out, uint16_t out_size,
          Layout &result);

} // namespace tk
