/*
 * E-paper rendering.
 *
 * C++ rather than C, unlike the rest of the Zephyr-facing glue, because it
 * calls lib/layout directly. The reason the other files stay C — zbus and HAL
 * macros expanding to out-of-order designated initializers — does not apply to
 * the display and CFB APIs, which are ordinary functions.
 *
 * ## Full versus partial refresh
 *
 * ssd16xx picks the waveform from the blanking state, but the two calls
 * bracket the write rather than acting as a mode flag — see the long comment
 * in tk_panel_render(), which is where getting this wrong showed up as a
 * refresh that returned in 20 ms and changed nothing.
 *
 * A cold boot always refreshes fully, because after deep sleep the panel holds
 * an image this firmware did not draw and has no record of. Partial refreshes
 * then run until CONFIG_TK_FULL_REFRESH_INTERVAL of them have accumulated.
 * That interval is a guess until someone watches ghosting on a real panel —
 * docs/firmware_architecture.md lists it as an open item.
 *
 * ## Font size
 *
 * CFB ships three sizes and the largest one a question fits in is the one it
 * gets, so a short question fills the panel instead of floating in white. The
 * table in `fonts` carries the ink extents each size was measured to have,
 * because the marks are placed against them.
 *
 * ## Marks
 *
 * The fonts have ASCII and nothing else, so accents are drawn here: lib/layout
 * hands over a base character plus a mark, the base comes from the font and
 * the mark from the small routines below.
 */

#include "panel.h"

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/display/cfb.h>
#include <zephyr/drivers/display.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include "layout.hpp"
#include "retained_block.hpp"

LOG_MODULE_REGISTER(tk_panel, LOG_LEVEL_INF);

namespace
{

const struct device *const display = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));

constexpr uint8_t kMargin = CONFIG_TK_PANEL_MARGIN_PX;

constexpr uint16_t kPanelWidth = DT_PROP(DT_CHOSEN(zephyr_display), width);
constexpr uint16_t kPanelHeight = DT_PROP(DT_CHOSEN(zephyr_display), height);

constexpr uint16_t kUsableWidth = kPanelWidth - 2 * kMargin;
constexpr uint16_t kUsableHeight = kPanelHeight - 2 * kMargin;

/*
 * The three fonts CFB ships, largest first. A short question gets big type and
 * a long one gets small type, which is what fills the panel instead of leaving
 * a band of white under every short question.
 *
 * `cap_top` and `lower_top` are the first row of ink for a capital and for a
 * lowercase letter, and `baseline` the first row below it. All three were
 * measured out of the font tables in cfb_fonts.c rather than guessed, because
 * the marks are positioned against them:
 *
 *   10x16   lowercase rows  5..12   capitals rows 3..12
 *   15x24   lowercase rows  7..18   capitals rows 3..18
 *   20x32   lowercase rows  9..24   capitals rows 4..24
 *
 * `index` is filled in at init: CFB addresses fonts by position in an iterable
 * section, and nothing promises that order, so it is looked up by size.
 */
struct Font {
    uint8_t width;
    uint8_t height;
    uint8_t cap_top;
    uint8_t lower_top;
    uint8_t baseline;
    /** Pixel scale for mark strokes. Deriving it from the width does not work:
     *  15/10 truncates to 1 and the marks come out at the small font's size. */
    uint8_t mark_scale;
    /**
     * Anchor the mark shapes are built around.
     *
     * Not the middle of the cell: every lowercase glyph in these fonts inks a
     * narrower band than the cell it sits in, and emboldening shifts that band
     * right. Measured ink centres for lowercase, before and after the bold
     * pass:
     *
     *   10x16   cols 3..8    centre 5.5   bolded 3..9    centre 6.0
     *   15x24   cols 3..13   centre 8.0   bolded 3..14   centre 8.5
     *   20x32   cols 4..17   centre 10.5  bolded 4..19   centre 11.5
     *
     * Every shape below spans an even number of pixels either side of this
     * anchor, so its visual centre lands on `mark_mid - 0.5`. The values are
     * therefore the ink centre rounded *up*: 6, 9, 12. The two larger fonts
     * come out exact; 10x16 is half a pixel left, which is as close as whole
     * pixels allow.
     */
    uint8_t mark_mid;
    /** How much emboldening shifts the glyph right. */
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

/* The smallest font is the one the corpus is guaranteed to fit in, so it is
 * the one CONFIG_TK_PANEL_COLUMNS and _LINES describe and the one the layout
 * suite checks every question against. */
constexpr uint8_t kColumns = CONFIG_TK_PANEL_COLUMNS;
constexpr uint8_t kLines = CONFIG_TK_PANEL_LINES;

BUILD_ASSERT(kColumns * 10 <= kUsableWidth, "TK_PANEL_COLUMNS overflows the panel width");
BUILD_ASSERT(kLines * 16 <= kUsableHeight, "TK_PANEL_LINES overflows the panel height");
BUILD_ASSERT((kColumns + 1) * 10 > kUsableWidth, "TK_PANEL_COLUMNS wastes a whole column");
BUILD_ASSERT(kLines <= tk::kMaxLines, "layout cannot return that many lines");

tk::Glyph glyphs[tk::kMaxGlyphs];
tk::Layout layout;

/*
 * In RTC memory rather than here, because deep sleep makes every press a
 * reboot: a counter in .bss would reset on each one, every wake would take the
 * full refresh a cold boot needs, and CONFIG_TK_FULL_REFRESH_INTERVAL would
 * describe nothing. The measured cost of getting this wrong is 2315 ms against
 * 622 ms, on every question.
 */
uint16_t &partial_since_full = tk_retained().partial_since_full;

uint8_t last_font_height;
bool ready;

bool is_capital(char c)
{
    return c >= 'A' && c <= 'Z';
}

/**
 * Fill a small rectangle.
 *
 * One line per row rather than cfb_draw_rect, which draws four lines — an
 * outline, not a fill. That distinction is invisible while the shapes are two
 * pixels square, because then the outline *is* the whole square, and it shows
 * up the moment a mark is scaled: the dots of an umlaut at 15x24 come out as
 * hollow boxes.
 *
 * Takes ints so the scaled arithmetic in draw_mark() does not need a cast per
 * argument; the values are all small and bounded by the cell.
 */
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

/**
 * Draw the diacritic for one cell.
 *
 * `x`,`y` are the cell's top-left. Marks sit just above the base glyph's ink,
 * which is two rows higher for capitals than for lowercase, so the whole mark
 * shifts up rather than colliding with the letter.
 *
 * Every shape below is symmetric about `mid`. That is the whole trick: an
 * earlier version drew the acute from `mid` rightwards and the grave from
 * `mid` leftwards, so each mark had its own centre and they all looked hung
 * off to one side of the letter. Spans are given in the comments so the
 * symmetry is checkable by reading rather than by photographing the panel.
 */
int draw_mark(const Font &font, tk::Mark mark, int16_t x, int16_t y, char base)
{
    if (mark == tk::Mark::NONE) {
        return 0;
    }

    /*
     * Shapes are tuned once at the small font's scale and multiplied up.
     * Horizontal and vertical scales are separate because the room above a
     * letter does not grow with the font: every one of these fonts starts its
     * capitals within three or four rows of the top of the cell, so a mark
     * over a capital has to stay short however large the type is.
     */
    const int16_t sx = font.mark_scale;
    const int16_t ink_top = is_capital(base) ? font.cap_top : font.lower_top;

    int16_t sy = sx;

    // Shrink until two rows of mark plus one of gap fit above the ink.
    while (sy > 1 && 2 * sy > ink_top - 1) {
        sy--;
    }

    const int16_t top = y + MAX(0, ink_top - 1 - 2 * sy);

    // The middle of the letter's ink, not of the cell. Without emboldening the
    // ink sits back to the left, so the anchor follows it.
    const int16_t mid =
        x + font.mark_mid - (IS_ENABLED(CONFIG_TK_PANEL_BOLD) ? 0 : (font.bold_offset + 1) / 2);
    const int16_t baseline = y + font.baseline;

    switch (mark) {
    case tk::Mark::ACUTE:
        // Rising to the right, spanning mid-2s .. mid+2s-1.
        (void) draw_dot(mid - 2 * sx, top + sy, 2 * sx, sy);
        return draw_dot(mid, top, 2 * sx, sy);

    case tk::Mark::GRAVE:
        // Falling to the right, the same span mirrored.
        (void) draw_dot(mid - 2 * sx, top, 2 * sx, sy);
        return draw_dot(mid, top + sy, 2 * sx, sy);

    case tk::Mark::CIRCUMFLEX:
        // A caret: peak over the middle, wings a row below.
        (void) draw_dot(mid - sx, top, 2 * sx, sy);
        (void) draw_dot(mid - 2 * sx, top + sy, sx, sy);
        return draw_dot(mid + sx, top + sy, sx, sy);

    case tk::Mark::DIAERESIS:
        // Two square dots either side of the middle, 2s clear pixels apart.
        (void) draw_dot(mid - 3 * sx, top, 2 * sx, 2 * sy);
        return draw_dot(mid + sx, top, 2 * sx, 2 * sy);

    case tk::Mark::TILDE:
        // Low, high, low, spanning mid-3s .. mid+3s-1.
        (void) draw_dot(mid - 3 * sx, top + sy, 2 * sx, sy);
        (void) draw_dot(mid - sx, top, 2 * sx, sy);
        return draw_dot(mid + sx, top + sy, 2 * sx, sy);

    case tk::Mark::CEDILLA:
        // Below the baseline, in the rows descenders use: a tick under the
        // letter hooking left, which is the direction a cedilla turns.
        (void) draw_dot(mid, baseline, sx, 2 * sy);
        return draw_dot(mid - sx, baseline + 2 * sy, 2 * sx, sy);

    case tk::Mark::NONE:
    default:
        return 0;
    }
}

int draw_glyph(const Font &font, const tk::Glyph &glyph, int16_t x, int16_t y)
{
    // One character at a time rather than one string per line: the marks need
    // per-cell positions anyway, and the font is monospace so the advance is
    // just the cell width.
    const char text[2] = {glyph.base, '\0'};

    int err = cfb_draw_text(display, text, x, y);

    if (err != 0) {
        return err;
    }

    if (IS_ENABLED(CONFIG_TK_PANEL_BOLD)) {
        /*
         * Faux bold: the same glyph again, shifted right. These fonts draw
         * thin strokes, which on e-paper at arm's length reads faint — this
         * thickens them for the cost of a second pass and no second font in
         * flash. The shift scales with the font, since a single pixel barely
         * shows against 20x32. Every glyph leaves at least a column of the
         * cell empty, so the shift cannot touch the next character.
         */
        err = cfb_draw_text(display, text, static_cast<int16_t>(x + font.bold_offset), y);

        if (err != 0) {
            return err;
        }
    }

    /*
     * The mark is drawn once whatever the weight. Emboldening it overdraws
     * shapes that are only two or three pixels to begin with: the two dots of
     * a diaeresis close up into a bar, and acute, grave and circumflex all
     * turn into the same blob.
     */
    return draw_mark(font, glyph.mark, x, y, glyph.base);
}

/**
 * Pick the largest font the text fits in.
 *
 * Largest first, so the answer is the first one that works. Falls back to the
 * smallest, which is the size every shipped question is checked against by the
 * layout suite — so the fallback is a guarantee rather than a hope.
 */
const Font *choose_font(const char *text, uint16_t len)
{
    const Font *smallest = &fonts[ARRAY_SIZE(fonts) - 1];

    if (!IS_ENABLED(CONFIG_TK_PANEL_AUTOSIZE)) {
        return tk::wrap(text, len, smallest->columns, glyphs, tk::kMaxGlyphs, layout) ? smallest
                                                                                      : nullptr;
    }

    for (size_t i = 0; i < ARRAY_SIZE(fonts); i++) {
        const Font &font = fonts[i];

        if (font.index < 0 || font.columns == 0 || font.lines == 0) {
            continue;
        }

        if (!tk::wrap(text, len, font.columns, glyphs, tk::kMaxGlyphs, layout)) {
            return nullptr;
        }

        if (!layout.truncated && layout.count <= font.lines) {
            return &font;
        }
    }

    // Nothing fitted, so lay it out at the smallest size and let the caller
    // decide what to do about the overflow.
    return tk::wrap(text, len, smallest->columns, glyphs, tk::kMaxGlyphs, layout) ? smallest
                                                                                  : nullptr;
}

} // namespace

int tk_panel_init(void)
{
    // Idempotent: the framebuffer is set up once, but the refresh counter is
    // reset on every call. cfb_framebuffer_init() allocates, so calling it
    // twice would leak, and a caller asking to start again is asking about the
    // panel's state rather than the framebuffer's.
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

        /*
         * Match each font we know about to a CFB index by its size. CFB
         * numbers fonts by their position in an iterable section and nothing
         * promises that order, so asking by size is the only stable way.
         */
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

                    if (font.lines > tk::kMaxLines) {
                        font.lines = tk::kMaxLines;
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

    /*
     * On a cold boot the panel holds whatever it was showing when power went
     * away, including across a flash, and this firmware has no record of it —
     * so the next render has to be a full one. On a wake it does have a
     * record, and forcing a full refresh there would spend 2315 ms undoing the
     * partial-refresh policy this counter exists to implement.
     */
    if (!tk_retained_survived()) {
        partial_since_full = CONFIG_TK_FULL_REFRESH_INTERVAL;
    }

    return 0;
}

uint16_t tk_panel_partial_count(void)
{
    return partial_since_full;
}

bool tk_panel_next_is_full(void)
{
    return partial_since_full >= CONFIG_TK_FULL_REFRESH_INTERVAL;
}

uint8_t tk_panel_last_font_height(void)
{
    return last_font_height;
}

int tk_panel_render(const char *text, uint16_t len)
{
    if (!ready) {
        return -ENODEV;
    }

    const Font *font = choose_font(text, len);

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
        // Drawn anyway: a clipped question still reads, and refusing would
        // leave the previous one up with no explanation. The suite fails on
        // this for every shipped question, so reaching it means the corpus
        // grew a question nobody checked.
        LOG_WRN("question needs more than %u lines", kLines);
    }

    const bool full = tk_panel_next_is_full();

    /*
     * Blanking is how ssd16xx is told which waveform to use, but it is not a
     * flag you set and forget — the two calls bracket the write:
     *
     *   blanking_on()   selects the full profile and suppresses the update
     *   write           loads the controller's RAM, and updates the panel
     *                   only while blanking is off
     *   blanking_off()  performs the update that was suppressed
     *
     * So a full refresh is a sandwich and a partial one is just a write with
     * blanking already off. Calling only the first half — which is what this
     * did at first — loads the image and never shows it: the refresh takes
     * about 20 ms, nothing changes on the glass, and the update is deferred
     * onto whichever later call turns blanking off, which then runs long and
     * displays the previous image.
     */
    err = full ? display_blanking_on(display) : display_blanking_off(display);

    if (err != 0 && err != -ENOSYS) {
        LOG_ERR("blanking: %d", err);
        return err;
    }

    err = cfb_framebuffer_clear(display, false);

    if (err != 0) {
        return err;
    }

    /*
     * Centred in pixels rather than in cells. Cell-quantised centring can only
     * ever be right to within half a character, which on a 23-column line is a
     * visible five-pixel lean.
     */
    /*
     * A question almost never fills its font's last line, and setting the
     * lines solid leaves the difference as a band of white above and below.
     * Sharing it between the lines uses the panel instead.
     *
     * This is not the same as bigger type, and it is not a substitute for it —
     * only a font between 15x24 and 20x32 would give that, which is an open
     * item. CONFIG_TK_PANEL_MAX_LEADING_PCT=0 sets the text solid again for
     * comparison.
     *
     * The leading can never exceed the slack it divides, so the block cannot
     * grow past the glass.
     */
    const uint16_t solid = static_cast<uint16_t>(layout.count * font->height);
    const uint8_t gaps = layout.count > 1 ? static_cast<uint8_t>(layout.count - 1) : 0;

    uint16_t leading = 0;

    if (gaps > 0 && kUsableHeight > solid) {
        const uint16_t cap =
            static_cast<uint16_t>(font->height * CONFIG_TK_PANEL_MAX_LEADING_PCT / 100);

        leading = static_cast<uint16_t>((kUsableHeight - solid) / gaps);

        if (leading > cap) {
            leading = cap;
        }
    }

    const uint16_t block_height = static_cast<uint16_t>(solid + leading * gaps);
    const int16_t top = static_cast<int16_t>(kMargin + (kUsableHeight - block_height) / 2);

    for (uint8_t i = 0; i < layout.count; i++) {
        const tk::Line &line = layout.lines[i];
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

    err = cfb_framebuffer_finalize(display);

    if (err != 0) {
        LOG_ERR("finalize: %d", err);
        return err;
    }

    if (full) {
        // The other half of the sandwich: this is the call that actually
        // drives the panel, and it blocks for the length of a full update.
        err = display_blanking_off(display);

        if (err != 0 && err != -ENOSYS) {
            LOG_ERR("blanking off: %d", err);
            return err;
        }
    }

    partial_since_full = full ? 0 : static_cast<uint16_t>(partial_since_full + 1);

    return 0;
}
