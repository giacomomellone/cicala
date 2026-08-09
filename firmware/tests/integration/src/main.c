/*
 * The whole application, wired the way the device wires it: a real gpio-keys
 * driver on emulated pins, the real input component, the real state machine
 * and question store, and the real panel code against a dummy display.
 *
 * Everything else in firmware/tests checks one piece in isolation. This checks
 * the seams between them — that pressing Category names a deck, that Next then
 * draws from it, and that the deck advances and wraps.
 */

#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/adc/adc_emul.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/gpio/gpio_emul.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

#include "channels.h"
#include "input.h"

static const struct gpio_dt_spec category = GPIO_DT_SPEC_GET(DT_ALIAS(tk_category), gpios);
static const struct gpio_dt_spec next_button = GPIO_DT_SPEC_GET(DT_ALIAS(tk_next), gpios);

static const struct gpio_dt_spec led_red = GPIO_DT_SPEC_GET(DT_ALIAS(tk_led_red), gpios);
static const struct gpio_dt_spec led_green = GPIO_DT_SPEC_GET(DT_ALIAS(tk_led_green), gpios);

static const struct adc_dt_spec cell = ADC_DT_SPEC_GET(DT_PATH(zephyr_user));

/** A comfortable cell, and one under CONFIG_TK_REFRESH_MIN_MV. */
#define HEALTHY_MV 3900
#define FLAT_MV 3000

/*
 * The emulated ADC reads the divider tap, not the pack, exactly as the board
 * does — so the suite divides on the way in and power.c multiplies on the way
 * out. Bypassing that would leave the one piece of arithmetic between a
 * voltage and a decision untested.
 */
#define TAP_MV(pack) ((pack) * CONFIG_TK_POWER_DIVIDER_DEN / CONFIG_TK_POWER_DIVIDER_NUM)

/**
 * Charge the emulated cell before the application starts.
 *
 * Priority 0 rather than CONFIG_APPLICATION_INIT_PRIORITY, so this runs ahead
 * of power.c's own hook at the same level. Without it the boot reading is the
 * emulated default of zero, the device decides the cell is flat, and every
 * other test in this file fails on a panel that correctly refused to refresh.
 */
static int charge_the_cell(void)
{
    (void) adc_emul_const_value_set(cell.dev, cell.channel_id, TAP_MV(HEALTHY_MV));

    return 0;
}

SYS_INIT(charge_the_cell, APPLICATION, 0);

/** Set the cell and wait for the sampler to notice. */
static void set_cell_mv(uint16_t pack_mv)
{
    zassert_ok(adc_emul_const_value_set(cell.dev, cell.channel_id, TAP_MV(pack_mv)));
    k_sleep(K_MSEC(CONFIG_TK_POWER_SAMPLE_MS * 3));
}

/* Debounce, the app thread, and the display thread finishing. The dummy panel
 * returns immediately, so this is mostly the debounce. */
#define PRESS_WAIT K_MSEC(CONFIG_TK_BUTTON_DEBOUNCE_MS + 300)

static struct tk_question_msg last_question;

/*
 * The first card of the run, kept separately. ztest does not promise an order
 * for the cases in a suite, so a test that asked about boot behaviour through
 * `last_question` would be asserting on whatever ran before it.
 */
static struct tk_question_msg first_question;
static bool have_first;

static int questions;
static int renders;
static int last_render_result;

static void observe(const struct zbus_channel *chan)
{
    if (chan == &chan_question) {
        last_question = *(const struct tk_question_msg *) zbus_chan_const_msg(chan);
        questions++;

        if (!have_first) {
            first_question = last_question;
            have_first = true;
        }
    } else if (chan == &chan_render) {
        const struct tk_render_msg *msg = zbus_chan_const_msg(chan);

        last_render_result = msg->result;
        renders++;
    }
}

ZBUS_LISTENER_DEFINE(test_obs, observe);
ZBUS_CHAN_ADD_OBS(chan_question, test_obs, 4);
ZBUS_CHAN_ADD_OBS(chan_render, test_obs, 4);

static void press(const struct gpio_dt_spec *button)
{
    zassert_ok(gpio_emul_input_set(button->port, button->pin, 0));
    k_sleep(K_MSEC(80));
    zassert_ok(gpio_emul_input_set(button->port, button->pin, 1));
    k_sleep(PRESS_WAIT);
}

static void *suite_setup(void)
{
    /*
     * Emulated pins read low by default, which for an active-low button means
     * held down. Release both before the input layer looks at them.
     */
    zassert_ok(gpio_emul_input_set(category.port, category.pin, 1));
    zassert_ok(gpio_emul_input_set(next_button.port, next_button.pin, 1));
    k_sleep(PRESS_WAIT);

    zassert_ok(tk_input_init());
    k_sleep(PRESS_WAIT);

    return NULL;
}

ZTEST_SUITE(tk_integration, NULL, suite_setup, NULL, NULL, NULL);

ZTEST(tk_integration, test_boot_names_the_remembered_deck)
{
    /*
     * A button has no position, so the deck comes from RTC memory. On this
     * platform the block is ordinary .bss and always reads as a cold boot,
     * which is the documented New People default.
     */
    zassert_true(have_first, "boot should have put something on the panel");
    zassert_equal(first_question.kind, TK_CARD_CATEGORY, "and on a cold boot that is a deck name");
    zassert_equal(first_question.deck, 0, "New People is the cold default");
}

ZTEST(tk_integration, test_category_advances_and_names_the_deck)
{
    const uint8_t before = last_question.deck;

    questions = 0;
    renders = 0;

    press(&category);

    zassert_equal(questions, 1, "one press is one card");
    zassert_equal(last_question.kind, TK_CARD_CATEGORY, "pressing Category names a deck");
    zassert_equal(last_question.deck, (before + 1) % TK_DECK_COUNT, "and advances by one");
    zassert_equal(renders, 1, "the display should have been asked to draw it");
    zassert_equal(last_render_result, 0);

    /* And it stays there: naming a deck is not asking a question. */
    k_sleep(PRESS_WAIT);
    zassert_equal(questions, 1, "the name waits for a press rather than timing out");
}

ZTEST(tk_integration, test_next_turns_the_deck_name_into_a_question)
{
    press(&category);
    zassert_equal(last_question.kind, TK_CARD_CATEGORY);

    const uint8_t deck = last_question.deck;

    questions = 0;

    press(&next_button);

    zassert_equal(questions, 1, "one press is one question");
    zassert_equal(last_question.kind, TK_CARD_QUESTION, "and a press asks for a real question");
    zassert_equal(last_question.deck, deck, "from the deck that was named");
    zassert_true(last_question.len > 0);
}

ZTEST(tk_integration, test_next_draws_a_different_question)
{
    press(&next_button);
    zassert_equal(last_question.kind, TK_CARD_QUESTION);

    const uint32_t before_seq = last_question.seq;
    const uint16_t before_len = last_question.len;
    char before_text[CONFIG_TK_MAX_QUESTION_BYTES];

    memcpy(before_text, last_question.text, before_len);

    questions = 0;

    press(&next_button);

    zassert_equal(questions, 1, "one press is one question");
    zassert_true(last_question.seq > before_seq, "the sequence has to advance");

    const bool same =
        last_question.len == before_len && memcmp(last_question.text, before_text, before_len) == 0;

    zassert_false(same, "the bag must not hand back the question already on screen");
}

ZTEST(tk_integration, test_the_deck_wraps_from_wild_to_new_people)
{
    /*
     * Six presses return to where they started, whatever that was. The wrap is
     * what makes one button enough to reach every deck.
     */
    press(&category);

    const uint8_t start = last_question.deck;

    for (int i = 0; i < TK_DECK_COUNT; i++) {
        press(&category);
    }

    zassert_equal(last_question.deck, start, "six presses is a full turn");
}

ZTEST(tk_integration, test_a_deck_returns_only_its_own_questions)
{
    /*
     * Wild is the only deck carrying the tone-flagged questions, so drawing it
     * repeatedly is the cheapest end-to-end check that the deck mask survives
     * the whole path from bundle to panel.
     */
    while (last_question.deck != 5 || last_question.kind != TK_CARD_CATEGORY) {
        press(&category);
    }

    for (int i = 0; i < 5; i++) {
        questions = 0;

        press(&next_button);

        zassert_equal(questions, 1, "press %d produced %d questions", i, questions);
        zassert_equal(last_question.deck, 5);
        zassert_equal(last_render_result, 0);
    }
}

/*
 * The refresh gate, which is the one piece of power policy with no host-only
 * home: it lives in app_logic.cpp's Io rather than in a library, because it has
 * to sit between the state machine and the bag. So it is checked here, against
 * the real ADC path and the real button.
 */

ZTEST(tk_integration, test_a_flat_cell_leaves_the_question_on_the_panel)
{
    press(&next_button);

    const uint32_t drawn = last_question.seq;

    set_cell_mv(FLAT_MV);

    questions = 0;
    renders = 0;

    press(&next_button);

    zassert_equal(questions, 0, "nothing should have been published");
    zassert_equal(renders, 0, "and nothing should have reached the panel");
    zassert_equal(last_question.seq, drawn, "the glass still holds the last question");

    set_cell_mv(HEALTHY_MV);
}

ZTEST(tk_integration, test_a_charged_cell_answers_again)
{
    set_cell_mv(FLAT_MV);

    questions = 0;
    press(&next_button);
    zassert_equal(questions, 0);

    set_cell_mv(HEALTHY_MV);

    press(&next_button);

    zassert_equal(questions, 1, "the press after a recovery is answered");
    zassert_equal(last_question.kind, TK_CARD_QUESTION);
    zassert_equal(last_render_result, 0);
}

ZTEST(tk_integration, test_a_flat_cell_does_not_burn_a_question_from_the_bag)
{
    /*
     * The gate runs before bag.draw(), which mutates the retained shuffle
     * state. Gating after it would spend a question from the no-repeat cycle
     * on every press the device was too flat to answer — invisible on the
     * glass, and the reason the order matters enough to test.
     */
    set_cell_mv(FLAT_MV);

    for (int i = 0; i < 5; i++) {
        press(&next_button);
    }

    set_cell_mv(HEALTHY_MV);

    questions = 0;
    press(&next_button);

    zassert_equal(questions, 1, "the bag still had something to give");
    zassert_equal(last_question.kind, TK_CARD_QUESTION);
}

ZTEST(tk_integration, test_a_refused_press_is_answered_on_the_red_led)
{
    set_cell_mv(HEALTHY_MV);

    zassert_equal(gpio_emul_output_get(led_red.port, led_red.pin), 0,
                  "a healthy cell shows nothing at all");
    zassert_equal(gpio_emul_output_get(led_green.port, led_green.pin), 0);

    set_cell_mv(FLAT_MV);

    press(&next_button);

    /*
     * Three blinks, so within the burst the pin has to be seen both lit and
     * dark. Polling rather than sampling once: press() already consumed part
     * of the burst, and asserting on a single instant would be asserting on
     * where in the blink the sleep happened to land.
     */
    bool seen_lit = false;
    bool seen_dark = false;

    for (int i = 0; i < 60 && !(seen_lit && seen_dark); i++) {
        if (gpio_emul_output_get(led_red.port, led_red.pin) > 0) {
            seen_lit = true;
        } else {
            seen_dark = true;
        }

        k_sleep(K_MSEC(CONFIG_TK_STATUS_LED_BLINK_MS / 4));
    }

    zassert_true(seen_lit, "a refused press should say so");
    zassert_true(seen_dark, "and it should blink rather than latch on");

    zassert_equal(gpio_emul_output_get(led_green.port, led_green.pin), 0,
                  "green means charged, and this cell is the opposite");

    set_cell_mv(HEALTHY_MV);
    k_sleep(K_MSEC(CONFIG_TK_STATUS_LED_BLINK_MS * 8));

    zassert_equal(gpio_emul_output_get(led_red.port, led_red.pin), 0,
                  "and the burst ends rather than running forever");
}
