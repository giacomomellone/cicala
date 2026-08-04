/*
 * The whole application, wired the way the device wires it: a real gpio-keys
 * driver on emulated pins, the real input component, the real state machine
 * and question store, and the real panel code against a dummy display.
 *
 * Everything else in firmware/tests checks one piece in isolation. This checks
 * the seams between them — that a contact closing eventually puts a question
 * on the panel, and that pressing Next gets a different one.
 */

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/gpio/gpio_emul.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

#include "channels.h"
#include "input.h"

#define DECK_SPEC(nodelabel) GPIO_DT_SPEC_GET(DT_NODELABEL(nodelabel), gpios)

static const struct gpio_dt_spec contacts[TK_DECK_COUNT] = {
    DECK_SPEC(tk_deck_0), DECK_SPEC(tk_deck_1), DECK_SPEC(tk_deck_2),
    DECK_SPEC(tk_deck_3), DECK_SPEC(tk_deck_4), DECK_SPEC(tk_deck_5),
};

static const struct gpio_dt_spec next_button = DECK_SPEC(tk_next);

/* Long enough for the debounce, the settle window, the app thread to draw and
 * the display thread to finish — the dummy panel returns immediately, so this
 * is dominated by the 600 ms settle. */
#define SETTLE_WAIT K_MSEC(CONFIG_TK_SELECTOR_SETTLE_MS + 400)
#define PRESS_WAIT K_MSEC(400)

static struct tk_question_msg last_question;
static int questions;
static int renders;
static int last_render_result;

static void observe(const struct zbus_channel *chan)
{
    if (chan == &chan_question) {
        last_question = *(const struct tk_question_msg *) zbus_chan_const_msg(chan);
        questions++;
    } else if (chan == &chan_render) {
        const struct tk_render_msg *msg = zbus_chan_const_msg(chan);

        last_render_result = msg->result;
        renders++;
    }
}

ZBUS_LISTENER_DEFINE(test_obs, observe);
ZBUS_CHAN_ADD_OBS(chan_question, test_obs, 4);
ZBUS_CHAN_ADD_OBS(chan_render, test_obs, 4);

static void set_contact(int deck, bool closed)
{
    zassert_ok(gpio_emul_input_set(contacts[deck].port, contacts[deck].pin, closed ? 0 : 1));
}

static void set_next(bool pressed)
{
    zassert_ok(gpio_emul_input_set(next_button.port, next_button.pin, pressed ? 0 : 1));
}

static void *suite_setup(void)
{
    /*
     * Emulated pins read low by default, which for an active-low contact means
     * every deck closed at once — an invalid selector. Open them all before
     * the input layer reads them, so the application starts from the state a
     * real device with the knob between detents would be in.
     */
    for (int i = 0; i < TK_DECK_COUNT; i++) {
        set_contact(i, false);
    }

    set_next(false);
    k_sleep(K_MSEC(100));

    zassert_ok(tk_input_init());
    k_sleep(SETTLE_WAIT);

    return NULL;
}

ZTEST_SUITE(tk_integration, NULL, suite_setup, NULL, NULL, NULL);

ZTEST(tk_integration, test_an_invalid_selector_draws_nothing)
{
    questions = 0;

    k_sleep(SETTLE_WAIT);

    zassert_equal(questions, 0, "a device with no valid deck must not pick one");
}

ZTEST(tk_integration, test_turning_the_selector_puts_the_deck_name_on_the_panel)
{
    questions = 0;
    renders = 0;

    set_contact(2, true);
    k_sleep(SETTLE_WAIT);

    zassert_equal(questions, 1, "one turn is one card");
    zassert_equal(last_question.deck, 2);
    zassert_true(last_question.is_category, "turning the knob names the deck");
    zassert_true(last_question.len > 0);
    zassert_equal(last_question.text[0], 'f', "deck 2 reads \"family\"");
    zassert_equal(renders, 1, "the display should have been asked to draw it");
    zassert_equal(last_render_result, 0, "the render should have succeeded");

    /* And it stays there. */
    k_sleep(SETTLE_WAIT);
    zassert_equal(questions, 1, "the name waits for a press rather than timing out");

    set_contact(2, false);
    k_sleep(SETTLE_WAIT);
}

ZTEST(tk_integration, test_next_turns_the_deck_name_into_a_question)
{
    set_contact(2, true);
    k_sleep(SETTLE_WAIT);
    zassert_true(last_question.is_category);

    questions = 0;

    set_next(true);
    k_sleep(K_MSEC(80));
    set_next(false);
    k_sleep(PRESS_WAIT);

    zassert_equal(questions, 1, "one press is one question");
    zassert_false(last_question.is_category, "and a press asks for a real question");
    zassert_equal(last_question.deck, 2, "from the deck that was named");
    zassert_true(last_question.len > 0);

    set_contact(2, false);
    k_sleep(SETTLE_WAIT);
}

ZTEST(tk_integration, test_next_draws_a_different_question)
{
    set_contact(1, true);
    k_sleep(SETTLE_WAIT);

    /* First press turns the deck name into a question; compare from there. */
    set_next(true);
    k_sleep(K_MSEC(80));
    set_next(false);
    k_sleep(PRESS_WAIT);
    zassert_false(last_question.is_category);

    questions = 0;

    const uint32_t before_seq = last_question.seq;
    char before_text[CONFIG_TK_MAX_QUESTION_BYTES];
    const uint16_t before_len = last_question.len;

    memcpy(before_text, last_question.text, before_len);

    set_next(true);
    k_sleep(K_MSEC(80));
    set_next(false);
    k_sleep(PRESS_WAIT);

    zassert_equal(questions, 1, "one press is one question");
    zassert_true(last_question.seq > before_seq, "the sequence has to advance");
    zassert_equal(last_question.deck, 1, "Next stays on the selected deck");

    const bool same =
        last_question.len == before_len && memcmp(last_question.text, before_text, before_len) == 0;

    zassert_false(same, "the bag must not hand back the question already on screen");

    set_contact(1, false);
    k_sleep(SETTLE_WAIT);
}

ZTEST(tk_integration, test_crossing_decks_names_only_the_deck_it_lands_on)
{
    questions = 0;

    /* Sweeping the knob from Work through Here to Wild. */
    set_contact(3, true);
    k_sleep(K_MSEC(100));
    set_contact(3, false);
    set_contact(4, true);
    k_sleep(K_MSEC(100));
    set_contact(4, false);
    set_contact(5, true);
    k_sleep(SETTLE_WAIT);

    zassert_equal(questions, 1, "one sweep is one card, not three");
    zassert_true(last_question.is_category, "and it names where the knob stopped");
    zassert_equal(last_question.deck, 5);

    set_contact(5, false);
    k_sleep(SETTLE_WAIT);
}

ZTEST(tk_integration, test_the_deck_returns_only_its_own_questions)
{
    /*
     * Wild is the only deck carrying the tone-flagged questions, so drawing it
     * repeatedly is the cheapest end-to-end check that the deck mask survives
     * the whole path from bundle to panel.
     */
    set_contact(5, true);
    k_sleep(SETTLE_WAIT);

    for (int i = 0; i < 5; i++) {
        questions = 0;

        set_next(true);
        k_sleep(K_MSEC(80));
        set_next(false);
        k_sleep(PRESS_WAIT);

        zassert_equal(questions, 1, "press %d produced %d questions", i, questions);
        zassert_equal(last_question.deck, 5);
        zassert_equal(last_render_result, 0);
    }

    set_contact(5, false);
    k_sleep(SETTLE_WAIT);
}
