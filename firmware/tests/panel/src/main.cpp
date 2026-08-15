/* Panel policy against a real-size display that discards pixels. */

#include <zephyr/ztest.h>

#include "panel.h"
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

void *suite_setup(void)
{
    zassert_ok(kveld_panel_init(), "the dummy display should always come up");

    return NULL;
}

} // namespace

ZTEST_SUITE(kveld_panel, NULL, suite_setup, NULL, NULL, NULL);

ZTEST(kveld_panel, test_a_question_renders)
{
    zassert_ok(kveld_panel_render("What made you laugh today?", 25));
}

ZTEST(kveld_panel, test_the_first_refresh_after_boot_is_full)
{
    zassert_ok(kveld_panel_init());
    zassert_true(kveld_panel_next_is_full(), "a cold boot must refresh fully");

    zassert_ok(kveld_panel_render("first", 5));
    zassert_equal(kveld_panel_partial_count(), 0, "a full refresh resets the count");
    zassert_false(kveld_panel_next_is_full());
}

ZTEST(kveld_panel, test_full_refreshes_come_round_on_the_interval)
{
    zassert_ok(kveld_panel_init());
    zassert_ok(kveld_panel_render("first", 5));

    for (int i = 1; i <= CONFIG_KVELD_FULL_REFRESH_INTERVAL; i++) {
        zassert_false(kveld_panel_next_is_full(), "refresh %d should still be partial", i);
        zassert_ok(kveld_panel_render("again", 5));
        zassert_equal(kveld_panel_partial_count(), i);
    }

    zassert_true(kveld_panel_next_is_full(), "ghosting has to be cleared eventually");

    zassert_ok(kveld_panel_render("clearing", 8));
    zassert_equal(kveld_panel_partial_count(), 0);
}

ZTEST(kveld_panel, test_accented_text_renders)
{
    // á à â ä ã ç é ñ ü ß
    const char *text = "\xC3\xA1 \xC3\xA0 \xC3\xA2 \xC3\xA4 \xC3\xA3 "
                       "\xC3\xA7 \xC3\xA9 \xC3\xB1 \xC3\xBC \xC3\x9F";

    zassert_ok(kveld_panel_render(text, 29), "marks are drawn, not skipped");
}

ZTEST(kveld_panel, test_capitals_with_marks_render)
{
    const char *text = "\xC3\x80\xC3\x81\xC3\x84\xC3\x87\xC3\x89\xC3\x91\xC3\x96\xC3\x9C";

    zassert_ok(kveld_panel_render(text, 16));
}

ZTEST(kveld_panel, test_malformed_text_is_refused)
{
    zassert_equal(kveld_panel_render("\xC3", 1), -EINVAL, "a truncated sequence is not a question");
}

ZTEST(kveld_panel, test_a_question_larger_than_the_buffer_is_refused)
{
    static char text[512];

    for (int i = 0; i < 400; i++) {
        text[i] = (i % 5 == 4) ? ' ' : 'x';
    }

    zassert_equal(kveld_panel_render(text, 400), -EINVAL);
}

ZTEST(kveld_panel, test_a_question_at_the_buffer_limit_still_renders)
{
    static char text[CONFIG_KVELD_MAX_QUESTION_BYTES];

    for (int i = 0; i < CONFIG_KVELD_MAX_QUESTION_BYTES; i += 2) {
        text[i] = '\xC3'; // ß, which decomposes to "ss"
        text[i + 1] = '\x9F';
    }

    zassert_ok(kveld_panel_render(text, CONFIG_KVELD_MAX_QUESTION_BYTES),
               "the worst-case expansion has to fit the buffer it was sized for");
}

ZTEST(kveld_panel, test_a_short_question_gets_bigger_type)
{
    zassert_ok(kveld_panel_render("Why?", 4));

    const uint8_t big = kveld_panel_last_font_height();

    zassert_true(big > 16, "a four-character question should not be set at 10x16, got %u", big);

    const char *lengthy = "Which object within sight would be hardest to explain "
                          "to someone from the past?";

    zassert_ok(kveld_panel_render(lengthy, 78));
    zassert_equal(kveld_panel_last_font_height(), 16,
                  "a long question falls back to the small font");
}

ZTEST(kveld_panel, test_font_choice_never_overflows_the_panel)
{
    static char text[CONFIG_KVELD_MAX_QUESTION_BYTES];

    for (uint16_t len = 1; len < CONFIG_KVELD_MAX_QUESTION_BYTES; len++) {
        text[len - 1] = (len % 6 == 0) ? ' ' : 'm';

        zassert_ok(kveld_panel_render(text, len), "length %u failed to render", len);

        const uint8_t h = kveld_panel_last_font_height();

        zassert_true(h == 16 || h == 24 || h == 32, "unexpected font height %u at length %u", h,
                     len);
    }
}

ZTEST(kveld_panel, test_leading_never_pushes_text_off_the_panel)
{
    static char text[CONFIG_KVELD_MAX_QUESTION_BYTES];

    for (uint16_t words = 1; words < 20; words++) {
        uint16_t len = 0;

        for (uint16_t w = 0; w < words && len + 5 < CONFIG_KVELD_MAX_QUESTION_BYTES; w++) {
            text[len++] = 'w';
            text[len++] = 'o';
            text[len++] = 'r';
            text[len++] = 'd';
            text[len++] = ' ';
        }

        zassert_ok(kveld_panel_render(text, len), "%u words failed to render", words);
    }
}

ZTEST(kveld_panel, test_every_shipped_question_renders)
{
    Qdb qdb;

    const uint8_t *const bundles[] = {en_bundle, de_bundle};
    const size_t sizes[] = {sizeof(en_bundle), sizeof(de_bundle)};

    for (size_t b = 0; b < ARRAY_SIZE(bundles); b++) {
        zassert_true(qdb.open(bundles[b], sizes[b]));

        for (uint16_t i = 0; i < qdb.count(); i++) {
            Question q;

            zassert_true(qdb.at(i, q));
            zassert_ok(kveld_panel_render(q.text, q.len), "question %u failed to render", i);
        }
    }
}
