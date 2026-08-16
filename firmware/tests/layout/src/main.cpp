
#include <zephyr/ztest.h>

#include <string.h>

#include "layout.hpp"
#include "qdb.hpp"

using namespace kveld;

namespace
{

const uint8_t en_bundle[] = {
#include "en_qdb.inc"
};

const uint8_t de_bundle[] = {
#include "de_qdb.inc"
};

constexpr uint8_t kColumns = CONFIG_KVELD_PANEL_COLUMNS;

Glyph glyphs[kMaxGlyphs];
Layout layout;

bool line_reads(const Layout &l, uint8_t index, const char *expected)
{
    if (index >= l.count) {
        return false;
    }

    const Line &line = l.lines[index];
    uint8_t i = 0;

    for (; i < line.cells; i++) {
        if (expected[i] == '\0' || glyphs[line.offset + i].base != expected[i]) {
            return false;
        }
    }

    return expected[i] == '\0';
}

} // namespace

ZTEST_SUITE(kveld_layout, NULL, NULL, NULL, NULL, NULL);

ZTEST(kveld_layout, test_utf8_decodes_one_and_two_byte_sequences)
{
    uint32_t cp = 0;

    zassert_equal(utf8_decode("A", 1, cp), 1);
    zassert_equal(cp, 'A');

    zassert_equal(utf8_decode("\xC3\xA9", 2, cp), 2, "e acute is two bytes");
    zassert_equal(cp, 0x00E9);

    zassert_equal(utf8_decode("\xC3\x9F", 2, cp), 2);
    zassert_equal(cp, 0x00DF, "sharp s");
}

ZTEST(kveld_layout, test_utf8_rejects_malformed_input)
{
    uint32_t cp = 0;

    zassert_equal(utf8_decode("", 0, cp), 0);
    zassert_equal(utf8_decode("\xC3", 1, cp), 0, "truncated two-byte sequence");
    zassert_equal(utf8_decode("\xC3\x28", 2, cp), 0, "second byte is not a continuation");
    zassert_equal(utf8_decode("\xA9", 1, cp), 0, "a continuation byte cannot start a character");
    zassert_equal(utf8_decode("\xC0\xAF", 2, cp), 0, "overlong encoding of '/'");
    zassert_equal(utf8_decode("\xE0\x80\xAF", 3, cp), 0, "overlong, three bytes");
}

ZTEST(kveld_layout, test_utf8_length_counts_characters_not_bytes)
{
    zassert_equal(utf8_length("Gr\xC3\xBC\xC3\x9F"
                              "e",
                              7),
                  5);
    zassert_equal(utf8_length("\xC3", 1), 0, "malformed input has no length");
}

ZTEST(kveld_layout, test_accents_become_a_base_letter_and_a_mark)
{
    Glyph out[2];

    zassert_equal(decompose('e', out), 1);
    zassert_equal(out[0].base, 'e');
    zassert_true(out[0].mark == Mark::NONE);

    zassert_equal(decompose(0x00E9, out), 1); // é
    zassert_equal(out[0].base, 'e');
    zassert_true(out[0].mark == Mark::ACUTE);

    zassert_equal(decompose(0x00FC, out), 1); // ü
    zassert_equal(out[0].base, 'u');
    zassert_true(out[0].mark == Mark::DIAERESIS);

    zassert_equal(decompose(0x00F1, out), 1); // ñ
    zassert_equal(out[0].base, 'n');
    zassert_true(out[0].mark == Mark::TILDE);

    zassert_equal(decompose(0x00E7, out), 1); // ç
    zassert_equal(out[0].base, 'c');
    zassert_true(out[0].mark == Mark::CEDILLA);

    zassert_equal(decompose(0x00E0, out), 1); // à
    zassert_true(out[0].mark == Mark::GRAVE);

    zassert_equal(decompose(0x00EE, out), 1); // î
    zassert_true(out[0].mark == Mark::CIRCUMFLEX);

    zassert_equal(decompose(0x00C9, out), 1); // É — capitals decompose too
    zassert_equal(out[0].base, 'E');
    zassert_true(out[0].mark == Mark::ACUTE);
}

ZTEST(kveld_layout, test_ligatures_fall_back_to_two_letters)
{
    Glyph out[2];

    zassert_equal(decompose(0x00DF, out), 2); // ß
    zassert_equal(out[0].base, 's');
    zassert_equal(out[1].base, 's');

    zassert_equal(decompose(0x0153, out), 2); // œ
    zassert_equal(out[0].base, 'o');
    zassert_equal(out[1].base, 'e');

    zassert_equal(decompose(0x00E6, out), 2); // æ
    zassert_equal(out[0].base, 'a');
    zassert_equal(out[1].base, 'e');
}

ZTEST(kveld_layout, test_unrepresentable_characters_report_themselves)
{
    Glyph out[2];

    zassert_equal(decompose(0x4E2D, out), 0, "a CJK ideograph has no fallback here");

    zassert_true(wrap("\xE4\xB8\xAD", 3, kColumns, glyphs, kMaxGlyphs, layout));
    zassert_true(layout.substituted, "the caller has to be able to tell");
    zassert_equal(layout.count, 1);
    zassert_equal(glyphs[layout.lines[0].offset].base, '?');
}

ZTEST(kveld_layout, test_a_short_question_is_one_line)
{
    const char *text = "What made you laugh?";

    zassert_true(wrap(text, 20, kColumns, glyphs, kMaxGlyphs, layout));
    zassert_equal(layout.count, 1);
    zassert_equal(layout.width, 20);
    zassert_false(layout.truncated);
    zassert_false(layout.substituted);
    zassert_true(line_reads(layout, 0, "What made you laugh?"));
}

ZTEST(kveld_layout, test_wrapping_breaks_on_spaces_and_drops_them)
{
    const char *text = "one two three four";

    zassert_true(wrap(text, 18, 10, glyphs, kMaxGlyphs, layout));
    zassert_equal(layout.count, 2);
    zassert_true(line_reads(layout, 0, "one two"), "no trailing space on a wrapped line");
    zassert_true(line_reads(layout, 1, "three four"));
}

ZTEST(kveld_layout, test_explicit_line_breaks_start_new_lines)
{
    const char *text = "join network\npass secret\nthen open";

    zassert_true(wrap(text, strlen(text), 20, glyphs, kMaxGlyphs, layout));
    zassert_equal(layout.count, 3);
    zassert_true(line_reads(layout, 0, "join network"));
    zassert_true(line_reads(layout, 1, "pass secret"));
    zassert_true(line_reads(layout, 2, "then open"));
}

ZTEST(kveld_layout, test_a_word_longer_than_the_line_is_broken)
{
    const char *text = "antidisestablishmentarianism";

    zassert_true(wrap(text, 28, 10, glyphs, kMaxGlyphs, layout));
    zassert_true(layout.count > 1);

    for (uint8_t i = 0; i < layout.count; i++) {
        zassert_true(layout.lines[i].cells <= 10, "line %u overflows the panel", i);
    }
}

ZTEST(kveld_layout, test_decomposition_happens_before_wrapping)
{
    const char *text = "Stra\xC3\x9F"
                       "e";

    zassert_true(wrap(text, 7, kColumns, glyphs, kMaxGlyphs, layout));
    zassert_equal(layout.count, 1);
    zassert_equal(layout.width, 7, "six characters, seven cells");
    zassert_true(line_reads(layout, 0, "Strasse"));
}

ZTEST(kveld_layout, test_an_accent_costs_one_cell_not_two)
{
    const char *text = "Gr\xC3\xBC\xC3\x9F"
                       "e";

    zassert_true(wrap(text, 7, kColumns, glyphs, kMaxGlyphs, layout));
    zassert_equal(layout.width, 6);
    zassert_true(line_reads(layout, 0, "Grusse"), "bases only; the marks ride above");

    const Line &line = layout.lines[0];

    zassert_true(glyphs[line.offset + 2].mark == Mark::DIAERESIS, "the u keeps its umlaut");
}

ZTEST(kveld_layout, test_text_needing_too_many_lines_is_flagged)
{
    static char text[512];
    uint16_t len = 0;

    for (int word = 0; word < 24; word++) {
        text[len++] = 'w';
        text[len++] = 'o';
        text[len++] = 'r';
        text[len++] = 'd';
        text[len++] = ' ';
    }

    zassert_true(wrap(text, len, 10, glyphs, kMaxGlyphs, layout));
    zassert_true(layout.truncated, "more text than lines has to be reported");
    zassert_equal(layout.count, kMaxLines);
}

ZTEST(kveld_layout, test_a_full_output_buffer_is_refused)
{
    const char *text = "far too long for the space given";

    zassert_false(wrap(text, 32, kColumns, glyphs, 8, layout));
}

ZTEST(kveld_layout, test_every_shipped_question_fits_the_panel)
{
    Qdb qdb;

    const uint8_t *const bundles[] = {en_bundle, de_bundle};
    const size_t sizes[] = {sizeof(en_bundle), sizeof(de_bundle)};

    for (size_t b = 0; b < ARRAY_SIZE(bundles); b++) {
        zassert_true(qdb.open(bundles[b], sizes[b]));

        for (uint16_t i = 0; i < qdb.count(); i++) {
            Question q;

            zassert_true(qdb.at(i, q));
            zassert_true(wrap(q.text, q.len, kColumns, glyphs, kMaxGlyphs, layout),
                         "question %u did not lay out at all", i);
            zassert_false(layout.truncated, "question %u needs more than %d lines", i, kMaxLines);
            zassert_false(layout.substituted,
                          "question %u contains a character the panel cannot draw", i);
            zassert_true(layout.count > 0);

            for (uint8_t l = 0; l < layout.count; l++) {
                zassert_true(layout.lines[l].cells <= kColumns, "question %u line %u is too wide",
                             i, l);
            }
        }
    }
}
