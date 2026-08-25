#include "layout.hpp"

namespace cicala
{

namespace
{

/** Return whether cfb_font_1016 contains the codepoint. */
bool has_glyph(uint32_t cp)
{
    return cp >= 32 && cp <= 126;
}

bool is_space(char c)
{
    return c == ' ' || c == '\t';
}

bool is_line_break(char c)
{
    return c == '\n';
}

struct Decomposition {
    uint16_t cp;
    char base;
    Mark mark;
};

// Latin-1 letters supported as a base glyph plus a mark.
constexpr Decomposition kDecompositions[] = {
    {0x00C0, 'A', Mark::GRAVE},      // À
    {0x00C1, 'A', Mark::ACUTE},      // Á
    {0x00C2, 'A', Mark::CIRCUMFLEX}, // Â
    {0x00C3, 'A', Mark::TILDE},      // Ã
    {0x00C4, 'A', Mark::DIAERESIS},  // Ä
    {0x00C7, 'C', Mark::CEDILLA},    // Ç
    {0x00C8, 'E', Mark::GRAVE},      // È
    {0x00C9, 'E', Mark::ACUTE},      // É
    {0x00CA, 'E', Mark::CIRCUMFLEX}, // Ê
    {0x00CB, 'E', Mark::DIAERESIS},  // Ë
    {0x00CC, 'I', Mark::GRAVE},      // Ì
    {0x00CD, 'I', Mark::ACUTE},      // Í
    {0x00CE, 'I', Mark::CIRCUMFLEX}, // Î
    {0x00CF, 'I', Mark::DIAERESIS},  // Ï
    {0x00D1, 'N', Mark::TILDE},      // Ñ
    {0x00D2, 'O', Mark::GRAVE},      // Ò
    {0x00D3, 'O', Mark::ACUTE},      // Ó
    {0x00D4, 'O', Mark::CIRCUMFLEX}, // Ô
    {0x00D5, 'O', Mark::TILDE},      // Õ
    {0x00D6, 'O', Mark::DIAERESIS},  // Ö
    {0x00D9, 'U', Mark::GRAVE},      // Ù
    {0x00DA, 'U', Mark::ACUTE},      // Ú
    {0x00DB, 'U', Mark::CIRCUMFLEX}, // Û
    {0x00DC, 'U', Mark::DIAERESIS},  // Ü
    {0x00DD, 'Y', Mark::ACUTE},      // Ý
    {0x00E0, 'a', Mark::GRAVE},      // à
    {0x00E1, 'a', Mark::ACUTE},      // á
    {0x00E2, 'a', Mark::CIRCUMFLEX}, // â
    {0x00E3, 'a', Mark::TILDE},      // ã
    {0x00E4, 'a', Mark::DIAERESIS},  // ä
    {0x00E7, 'c', Mark::CEDILLA},    // ç
    {0x00E8, 'e', Mark::GRAVE},      // è
    {0x00E9, 'e', Mark::ACUTE},      // é
    {0x00EA, 'e', Mark::CIRCUMFLEX}, // ê
    {0x00EB, 'e', Mark::DIAERESIS},  // ë
    {0x00EC, 'i', Mark::GRAVE},      // ì
    {0x00ED, 'i', Mark::ACUTE},      // í
    {0x00EE, 'i', Mark::CIRCUMFLEX}, // î
    {0x00EF, 'i', Mark::DIAERESIS},  // ï
    {0x00F1, 'n', Mark::TILDE},      // ñ
    {0x00F2, 'o', Mark::GRAVE},      // ò
    {0x00F3, 'o', Mark::ACUTE},      // ó
    {0x00F4, 'o', Mark::CIRCUMFLEX}, // ô
    {0x00F5, 'o', Mark::TILDE},      // õ
    {0x00F6, 'o', Mark::DIAERESIS},  // ö
    {0x00F9, 'u', Mark::GRAVE},      // ù
    {0x00FA, 'u', Mark::ACUTE},      // ú
    {0x00FB, 'u', Mark::CIRCUMFLEX}, // û
    {0x00FC, 'u', Mark::DIAERESIS},  // ü
    {0x00FD, 'y', Mark::ACUTE},      // ý
    {0x00FF, 'y', Mark::DIAERESIS},  // ÿ
};

struct Ligature {
    uint16_t cp;
    char first;
    char second;
};

// Ligatures rendered as two available glyphs.
constexpr Ligature kLigatures[] = {
    {0x00C6, 'A', 'e'}, // Æ
    {0x00DF, 's', 's'}, // ß
    {0x00E6, 'a', 'e'}, // æ
    {0x0152, 'O', 'e'}, // Œ
    {0x0153, 'o', 'e'}, // œ
};

// Unsupported inverted punctuation uses its upright glyph.
struct Substitute {
    uint16_t cp;
    char base;
};

constexpr Substitute kSubstitutes[] = {
    {0x00A1, '!'},                  // ¡
    {0x00BF, '?'},                  // ¿
    {0x2018, '\''}, {0x2019, '\''}, // curly single quotes
    {0x201C, '"'},  {0x201D, '"'},  // curly double quotes
    {0x2013, '-'},  {0x2014, '-'},  // en and em dash
    {0x2026, '.'},                  // ellipsis, narrowed to one dot
    {0x00A0, ' '},                  // non-breaking space
};

} // namespace

uint8_t utf8_decode(const char *s, uint16_t len, uint32_t &cp)
{
    if (len == 0) {
        return 0;
    }

    const uint8_t b0 = static_cast<uint8_t>(s[0]);

    if (b0 < 0x80) {
        cp = b0;
        return 1;
    }

    uint8_t extra;
    uint32_t value;

    if ((b0 & 0xE0) == 0xC0) {
        extra = 1;
        value = b0 & 0x1F;
    } else if ((b0 & 0xF0) == 0xE0) {
        extra = 2;
        value = b0 & 0x0F;
    } else if ((b0 & 0xF8) == 0xF0) {
        extra = 3;
        value = b0 & 0x07;
    } else {
        return 0;
    }

    if (len < extra + 1u) {
        return 0;
    }

    for (uint8_t i = 1; i <= extra; i++) {
        const uint8_t b = static_cast<uint8_t>(s[i]);

        if ((b & 0xC0) != 0x80) {
            return 0;
        }

        value = (value << 6) | (b & 0x3F);
    }

    // Reject overlong UTF-8 encodings.
    static const uint32_t min_value[4] = {0, 0x80, 0x800, 0x10000};

    if (value < min_value[extra]) {
        return 0;
    }

    cp = value;

    return static_cast<uint8_t>(extra + 1);
}

uint16_t utf8_length(const char *text, uint16_t len)
{
    uint16_t cells = 0;
    uint16_t pos = 0;

    while (pos < len) {
        uint32_t cp = 0;
        const uint8_t used = utf8_decode(text + pos, static_cast<uint16_t>(len - pos), cp);

        if (used == 0) {
            return 0;
        }

        pos += used;
        cells++;
    }

    return cells;
}

uint8_t decompose(uint32_t cp, Glyph *out)
{
    if (has_glyph(cp)) {
        out[0] = {static_cast<char>(cp), Mark::NONE};
        return 1;
    }

    for (const Decomposition &d : kDecompositions) {
        if (d.cp == cp) {
            out[0] = {d.base, d.mark};
            return 1;
        }
    }

    for (const Ligature &l : kLigatures) {
        if (l.cp == cp) {
            out[0] = {l.first, Mark::NONE};
            out[1] = {l.second, Mark::NONE};
            return 2;
        }
    }

    for (const Substitute &s : kSubstitutes) {
        if (s.cp == cp) {
            out[0] = {s.base, Mark::NONE};
            return 1;
        }
    }

    return 0;
}

bool wrap(const char *text, uint16_t len, uint8_t columns, Glyph *out, uint16_t out_size,
          Layout &result)
{
    result.count = 0;
    result.width = 0;
    result.truncated = false;
    result.substituted = false;

    if (text == nullptr || out == nullptr || columns == 0) {
        return false;
    }

    // Decomposition can change width, so it must happen before wrapping.
    uint16_t glyphs = 0;
    uint16_t pos = 0;

    while (pos < len) {
        uint32_t cp = 0;
        const uint8_t used = utf8_decode(text + pos, static_cast<uint16_t>(len - pos), cp);

        if (used == 0) {
            return false;
        }

        pos += used;

        Glyph decomposed[2];
        uint8_t n;

        if (cp == '\n') {
            decomposed[0] = {'\n', Mark::NONE};
            n = 1;
        } else {
            n = decompose(cp, decomposed);
        }

        if (n == 0) {
            // Keep unsupported input visible in the rendered text.
            decomposed[0] = {'?', Mark::NONE};
            n = 1;
            result.substituted = true;
        }

        if (glyphs + n > out_size) {
            return false;
        }

        for (uint8_t i = 0; i < n; i++) {
            out[glyphs++] = decomposed[i];
        }
    }

    uint16_t at = 0;

    while (at < glyphs) {
        while (at < glyphs && is_space(out[at].base)) {
            at++;
        }

        if (at < glyphs && is_line_break(out[at].base)) {
            at++;
            continue;
        }

        if (at >= glyphs) {
            break;
        }

        if (result.count >= kMaxLines) {
            result.truncated = true;
            return true;
        }

        Line &line = result.lines[result.count];

        line.offset = at;
        line.cells = 0;

        while (at < glyphs && !is_line_break(out[at].base)) {
            uint16_t word_end = at;

            while (word_end < glyphs && !is_space(out[word_end].base) &&
                   !is_line_break(out[word_end].base)) {
                word_end++;
            }

            const uint16_t word_cells = static_cast<uint16_t>(word_end - at);
            const uint16_t separator = (line.cells == 0) ? 0 : 1;

            if (line.cells + separator + word_cells <= columns) {
                at = word_end;
                line.cells = static_cast<uint8_t>(line.cells + separator + word_cells);

                if (at < glyphs && out[at].base == ' ') {
                    at++;
                }

                if (at < glyphs && is_line_break(out[at].base)) {
                    at++;
                    break;
                }

                continue;
            }

            if (line.cells == 0) {
                // Break a word that is wider than the panel.
                line.cells = columns;
                at = static_cast<uint16_t>(line.offset + columns);
            }

            break;
        }

        if (line.cells > result.width) {
            result.width = line.cells;
        }

        result.count++;
    }

    return true;
}

} // namespace cicala
