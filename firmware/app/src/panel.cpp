/* E-paper rendering. */

#include "panel.h"
#include "channels.h"
#include <stdio.h>
#include <string.h>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/display/cfb.h>
#include <zephyr/drivers/display.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include "layout.hpp"
#include "retained_block.hpp"

LOG_MODULE_REGISTER(cicala_panel, LOG_LEVEL_INF);

namespace
{

const struct device *const display = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));

constexpr uint8_t kMargin = CONFIG_CICALA_PANEL_MARGIN_PX;

constexpr uint16_t kPanelWidth = DT_PROP(DT_CHOSEN(zephyr_display), width);
constexpr uint16_t kPanelHeight = DT_PROP(DT_CHOSEN(zephyr_display), height);

constexpr uint16_t kUsableWidth = kPanelWidth - 2 * kMargin;
constexpr uint16_t kUsableHeight = kPanelHeight - 2 * kMargin;

/* The three fonts CFB ships, largest first. */
struct Font {
    uint8_t width;
    uint8_t height;
    uint8_t cap_top;
    uint8_t lower_top;
    uint8_t baseline;
    /* Pixel scale for mark strokes. */
    uint8_t mark_scale;
    /* Anchor the mark shapes are built around. */
    uint8_t mark_mid;
    /* How much emboldening shifts the glyph right. */
    uint8_t bold_offset;
    uint8_t columns;
    uint8_t lines;
    int8_t index;
};

Font fonts[] = {
    {20, 32, 4, 9, 25, 2, 12, 2, 0, 0, -1},
    {15, 24, 3, 7, 19, 2, 9, 1, 0, 0, -1},
    {10, 16, 3, 5, 13, 1, 6, 1, 0, 0, -1},
};

/* Panel column and line limits describe the smallest supported font. */
constexpr uint8_t kColumns = CONFIG_CICALA_PANEL_COLUMNS;
constexpr uint8_t kLines = CONFIG_CICALA_PANEL_LINES;

BUILD_ASSERT(kColumns * 10 <= kUsableWidth, "CICALA_PANEL_COLUMNS overflows the panel width");
BUILD_ASSERT(kLines * 16 <= kUsableHeight, "CICALA_PANEL_LINES overflows the panel height");
BUILD_ASSERT((kColumns + 1) * 10 > kUsableWidth, "CICALA_PANEL_COLUMNS wastes a whole column");
BUILD_ASSERT(kLines <= cicala::kMaxLines, "layout cannot return that many lines");

cicala::Glyph glyphs[cicala::kMaxGlyphs];
cicala::Layout layout;

/* The partial-refresh counter lives in RTC memory across deep sleep. */
uint16_t &partial_since_full = cicala_retained().partial_since_full;

uint8_t last_font_height;
bool ready;

bool is_capital(char c)
{
    return c >= 'A' && c <= 'Z';
}

/* Fill a small rectangle. */
int draw_dot(int x, int y, int w, int h)
{
    for (int row = 0; row < h; row++) {
        const struct cfb_position start = {static_cast<int16_t>(x), static_cast<int16_t>(y + row)};
        const struct cfb_position end = {static_cast<int16_t>(x + w - 1),
                                         static_cast<int16_t>(y + row)};

        const int err = cfb_draw_line(display, &start, &end);

        if (err != 0) {
            return err;
        }
    }

    return 0;
}

/* Rows of ordered dither rising from the bottom edge. */
constexpr uint16_t kFloorRows = kPanelHeight * CONFIG_CICALA_PANEL_FLOOR_PCT / 100;

BUILD_ASSERT(kFloorRows * 2 < kPanelHeight, "the floor dither would reach the middle of the panel");

/* The floor the website draws with a gradient mask. A one-bit panel cannot
   fade a pattern, so it thins one instead: the ramp steps down through the
   same three densities the stylesheet uses, 1/4 then 1/8 then 1/16 of the
   grid. Periods are powers of two and indexed off absolute coordinates, so
   the pattern stays phase-locked to the panel and never shifts between
   refreshes — an unchanged pixel costs a partial refresh nothing. */
int draw_floor(void)
{
    if (kFloorRows == 0) {
        return 0;
    }

    const uint16_t step = MAX(kFloorRows / 3, 1);

    for (int16_t y = kPanelHeight - kFloorRows; y < static_cast<int16_t>(kPanelHeight); y++) {
        const uint16_t above_edge = static_cast<uint16_t>(kPanelHeight - 1 - y);

        uint8_t x_period = 4;
        uint8_t y_period = 4;

        if (above_edge < step) {
            x_period = 2;
            y_period = 2;
        } else if (above_edge < 2 * step) {
            x_period = 2;
            y_period = 4;
        }

        if (y % y_period != 0) {
            continue;
        }

        for (int16_t x = 0; x < static_cast<int16_t>(kPanelWidth); x += x_period) {
            const int err = draw_dot(x, y, 1, 1);

            if (err != 0) {
                return err;
            }
        }
    }

    return 0;
}

/* Draw the diacritic for one cell. */
int draw_mark(const Font &font, cicala::Mark mark, int16_t x, int16_t y, char base)
{
    if (mark == cicala::Mark::NONE) {
        return 0;
    }

    /* Shapes are tuned once at the small font's scale and multiplied up. */
    const int16_t sx = font.mark_scale;
    const int16_t ink_top = is_capital(base) ? font.cap_top : font.lower_top;

    int16_t sy = sx;

    // Shrink until two rows of mark plus one of gap fit above the ink.
    while (sy > 1 && 2 * sy > ink_top - 1) {
        sy--;
    }

    const int16_t top = y + MAX(0, ink_top - 1 - 2 * sy);

    // The middle of the letter's ink, not of the cell.
    const int16_t mid =
        x + font.mark_mid - (IS_ENABLED(CONFIG_CICALA_PANEL_BOLD) ? 0 : (font.bold_offset + 1) / 2);
    const int16_t baseline = y + font.baseline;

    switch (mark) {
    case cicala::Mark::ACUTE:
        // Rising to the right, spanning mid-2s ..
        (void) draw_dot(mid - 2 * sx, top + sy, 2 * sx, sy);
        return draw_dot(mid, top, 2 * sx, sy);

    case cicala::Mark::GRAVE:
        // Falling to the right, the same span mirrored.
        (void) draw_dot(mid - 2 * sx, top, 2 * sx, sy);
        return draw_dot(mid, top + sy, 2 * sx, sy);

    case cicala::Mark::CIRCUMFLEX:
        // A caret: peak over the middle, wings a row below.
        (void) draw_dot(mid - sx, top, 2 * sx, sy);
        (void) draw_dot(mid - 2 * sx, top + sy, sx, sy);
        return draw_dot(mid + sx, top + sy, sx, sy);

    case cicala::Mark::DIAERESIS:
        // Two square dots either side of the middle, 2s clear pixels apart.
        (void) draw_dot(mid - 3 * sx, top, 2 * sx, 2 * sy);
        return draw_dot(mid + sx, top, 2 * sx, 2 * sy);

    case cicala::Mark::TILDE:
        // Low, high, low, spanning mid-3s ..
        (void) draw_dot(mid - 3 * sx, top + sy, 2 * sx, sy);
        (void) draw_dot(mid - sx, top, 2 * sx, sy);
        return draw_dot(mid + sx, top + sy, 2 * sx, sy);

    case cicala::Mark::CEDILLA:
        // Draw the cedilla below the baseline in the descender rows.
        (void) draw_dot(mid, baseline, sx, 2 * sy);
        return draw_dot(mid - sx, baseline + 2 * sy, 2 * sx, sy);

    case cicala::Mark::NONE:
    default:
        return 0;
    }
}

int draw_glyph(const Font &font, const cicala::Glyph &glyph, int16_t x, int16_t y)
{
    // Draw per cell so diacritics use the same position as their base glyph.
    const char text[2] = {glyph.base, '\0'};

    int err = cfb_draw_text(display, text, x, y);

    if (err != 0) {
        return err;
    }

    if (IS_ENABLED(CONFIG_CICALA_PANEL_BOLD)) {
        /* Faux bold: the same glyph again, shifted right. */
        err = cfb_draw_text(display, text, static_cast<int16_t>(x + font.bold_offset), y);

        if (err != 0) {
            return err;
        }
    }

    /* The mark is drawn once whatever the weight. */
    return draw_mark(font, glyph.mark, x, y, glyph.base);
}

/* Pick the largest font the text fits in. */
const Font *choose_font(const char *text, uint16_t len, uint16_t height)
{
    const Font *smallest = &fonts[ARRAY_SIZE(fonts) - 1];

    if (!IS_ENABLED(CONFIG_CICALA_PANEL_AUTOSIZE)) {
        return cicala::wrap(text, len, smallest->columns, glyphs, cicala::kMaxGlyphs, layout)
                   ? smallest
                   : nullptr;
    }

    for (size_t i = 0; i < ARRAY_SIZE(fonts); i++) {
        const Font &font = fonts[i];

        if (font.index < 0 || font.columns == 0 || font.lines == 0) {
            continue;
        }

        if (!cicala::wrap(text, len, font.columns, glyphs, cicala::kMaxGlyphs, layout)) {
            return nullptr;
        }

        if (!layout.truncated && layout.count <= height / font.height) {
            return &font;
        }
    }

    // Return the smallest layout so the caller can report overflow.
    return cicala::wrap(text, len, smallest->columns, glyphs, cicala::kMaxGlyphs, layout) ? smallest
                                                                                          : nullptr;
}

} // namespace

int cicala_panel_init(void)
{
    // Reuse the framebuffer and reset the refresh policy.
    if (!ready) {
        if (!device_is_ready(display)) {
            LOG_ERR("display not ready");
            return -ENODEV;
        }

        int err = display_set_pixel_format(display, PIXEL_FORMAT_MONO10);

        if (err != 0 && err != -ENOSYS) {
            LOG_ERR("pixel format: %d", err);
            return err;
        }

        err = cfb_framebuffer_init(display);

        if (err != 0) {
            LOG_ERR("framebuffer init: %d", err);
            return err;
        }

        /* Match each font we know about to a CFB index by its size. */
        const int available = cfb_get_numof_fonts(display);

        for (size_t i = 0; i < ARRAY_SIZE(fonts); i++) {
            Font &font = fonts[i];

            font.index = -1;

            for (int idx = 0; idx < available; idx++) {
                uint8_t w = 0;
                uint8_t h = 0;

                if (cfb_get_font_size(display, idx, &w, &h) != 0) {
                    continue;
                }

                if (w == font.width && h == font.height) {
                    font.index = static_cast<int8_t>(idx);
                    font.columns = static_cast<uint8_t>(kUsableWidth / w);
                    font.lines = static_cast<uint8_t>(kUsableHeight / h);

                    if (font.lines > cicala::kMaxLines) {
                        font.lines = cicala::kMaxLines;
                    }

                    break;
                }
            }

            LOG_INF("font %ux%u: %s (%u columns, %u lines)", font.width, font.height,
                    font.index < 0 ? "not built in" : "available", font.columns, font.lines);
        }

        if (fonts[ARRAY_SIZE(fonts) - 1].index < 0) {
            LOG_ERR("the 10x16 font is missing; nothing can be drawn");
            return -ENOENT;
        }

        ready = true;
    }

    /* Rev A removes panel power during sleep, so controller RAM is lost even
     * when the MCU's retained state and the visible image survive. */
    if (IS_ENABLED(CONFIG_CICALA_REV_A) || !cicala_retained_survived()) {
        partial_since_full = CONFIG_CICALA_FULL_REFRESH_INTERVAL;
        cicala_retained_seal();
    }

    return 0;
}

uint16_t cicala_panel_partial_count(void)
{
    return partial_since_full;
}

bool cicala_panel_next_is_full(void)
{
    return partial_since_full >= CONFIG_CICALA_FULL_REFRESH_INTERVAL;
}

uint8_t cicala_panel_last_font_height(void)
{
    return last_font_height;
}

static int render(const char *text, uint16_t len, const cicala_question_msg *card)
{
    if (!ready) {
        return -ENODEV;
    }

    const bool menu = card && card->kind == CICALA_CARD_FILTERS;
    const bool header = card && card->kind != CICALA_CARD_SERVICE && !menu;
    const uint16_t height = kUsableHeight - (header ? 20 : 0);
    const Font *font = choose_font(text, len, height);
    if (menu)
        font = &fonts[ARRAY_SIZE(fonts) - 1];

    if (font == nullptr) {
        LOG_ERR("question does not lay out");
        return -EINVAL;
    }

    int err = cfb_framebuffer_set_font(display, static_cast<uint8_t>(font->index));

    if (err != 0) {
        LOG_ERR("font %ux%u: %d", font->width, font->height, err);
        return err;
    }

    last_font_height = font->height;

    if (layout.truncated) {
        // Render clipped text so the press still produces visible output.
        LOG_WRN("question needs more than %u lines", kLines);
    }

    const bool full = cicala_panel_next_is_full();

    /* SSD16xx full refreshes bracket the RAM write with blanking mode. */
    err = full ? display_blanking_on(display) : display_blanking_off(display);

    if (err != 0 && err != -ENOSYS) {
        LOG_ERR("blanking: %d", err);
        return err;
    }

    err = cfb_framebuffer_clear(display, false);

    if (err != 0) {
        return err;
    }

    /* Before the glyphs, so the question is drawn over the floor. */
    err = draw_floor();

    if (err != 0) {
        LOG_ERR("floor: %d", err);
        return err;
    }

    if (menu) {
        const char *labels[] = {"Dark", "Sexual", "Heavy", "Done"};
        err = cfb_draw_text(display, "Filters", kMargin, 0);
        for (int i = 0; i < 4 && !err; i++) {
            char row[32];
            if (i < 3)
                snprintf(row, sizeof(row), "%c %s: %s", card->cursor == i ? '>' : ' ', labels[i],
                         card->permissions & (1u << i) ? "allowed" : "excluded");
            else
                snprintf(row, sizeof(row), "%c Done", card->cursor == i ? '>' : ' ');
            err = cfb_draw_text(display, row, kMargin, 17 + 16 * i);
        }
        if (!err)
            err = cfb_draw_text(display, "Filters: move", kMargin, kPanelHeight - 32);
        if (!err)
            err = cfb_draw_text(display, "Next: change / done", kMargin, kPanelHeight - 16);
        if (err)
            return err;
    } else {
        /* Centre using pixel dimensions. */
        /* Share unused line height above and below the text. */
        const uint16_t solid = static_cast<uint16_t>(layout.count * font->height);
        const uint8_t gaps = layout.count > 1 ? static_cast<uint8_t>(layout.count - 1) : 0;

        uint16_t leading = 0;

        if (gaps > 0 && height > solid) {
            const uint16_t cap =
                static_cast<uint16_t>(font->height * CONFIG_CICALA_PANEL_MAX_LEADING_PCT / 100);

            leading = static_cast<uint16_t>((height - solid) / gaps);

            if (leading > cap) {
                leading = cap;
            }
        }

        const uint16_t block_height = static_cast<uint16_t>(solid + leading * gaps);
        const int16_t top =
            static_cast<int16_t>(kMargin + (header ? 20 : 0) + (height - block_height) / 2);

        for (uint8_t i = 0; i < layout.count; i++) {
            const cicala::Line &line = layout.lines[i];
            const uint16_t line_width = static_cast<uint16_t>(line.cells * font->width);
            const int16_t left = static_cast<int16_t>(kMargin + (kUsableWidth - line_width) / 2);
            const int16_t y = static_cast<int16_t>(top + i * (font->height + leading));

            for (uint8_t c = 0; c < line.cells; c++) {
                err = draw_glyph(*font, glyphs[line.offset + c],
                                 static_cast<int16_t>(left + c * font->width), y);

                if (err != 0) {
                    LOG_ERR("draw at line %u column %u: %d", i, c, err);
                    return err;
                }
            }
        }

        if (header) {
            char summary[32];
            snprintf(summary, sizeof(summary), "Dark%c Sexual%c Heavy%c",
                     card->permissions & 1 ? '+' : '-', card->permissions & 2 ? '+' : '-',
                     card->permissions & 4 ? '+' : '-');
            err = cfb_framebuffer_set_font(display, fonts[ARRAY_SIZE(fonts) - 1].index);
            if (!err)
                err = cfb_draw_text(display, summary, kMargin, kMargin);
            if (err)
                return err;
        }
    }

    err = cfb_framebuffer_finalize(display);

    if (err != 0) {
        LOG_ERR("finalize: %d", err);
        return err;
    }

    if (full) {
        // Leaving blanking mode drives the full panel update.
        err = display_blanking_off(display);

        if (err != 0 && err != -ENOSYS) {
            LOG_ERR("blanking off: %d", err);
            return err;
        }
    }

    partial_since_full = full ? 0 : static_cast<uint16_t>(partial_since_full + 1);

    return 0;
}

int cicala_panel_render(const char *text, uint16_t len)
{
    return render(text, len, nullptr);
}

int cicala_panel_render_card(const cicala_question_msg *card)
{
    if (card->kind == CICALA_CARD_EMPTY) {
        const char *empty = "No questions match. Filters to adjust.";
        return render(empty, strlen(empty), card);
    }
    // A menu has no question text to lay out.
    return render(card->kind == CICALA_CARD_FILTERS ? "Filters" : card->text,
                  card->kind == CICALA_CARD_FILTERS ? 7 : card->len, card);
}
