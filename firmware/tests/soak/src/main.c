/*
 * The soak driver, running the application it drives.
 *
 * `just fw-soak` is meant to be flashed and left alone for hours, so the
 * failure that costs the most is the one where the chain stops after the first
 * press and nobody notices until they come back to a panel showing one
 * question. That is what this checks: set the selector once, then touch
 * nothing and count what arrives.
 *
 * Its own suite rather than a case in tests/integration, because
 * CONFIG_TK_DEBUG_SOAK presses Next continuously and would walk over every
 * other case in the file.
 */

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/gpio/gpio_emul.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

#include "channels.h"
#include "input.h"

static const struct gpio_dt_spec next_button = GPIO_DT_SPEC_GET(DT_ALIAS(tk_next), gpios);

/* Ten presses' worth, plus a margin for the dummy panel and the logging
 * behind it. */
#define RUN_WAIT K_MSEC(10 * CONFIG_TK_DEBUG_SOAK_INTERVAL_MS + 1000)

#define EXPECTED_RENDERS 6

static uint32_t questions;
static uint32_t renders;
static uint32_t full_renders;
static uint32_t failed_renders;
static uint32_t last_seq;
static bool seq_repeated;

static void observe(const struct zbus_channel *chan)
{
    if (chan == &chan_question) {
        const struct tk_question_msg *msg = zbus_chan_const_msg(chan);

        if (msg->seq <= last_seq) {
            seq_repeated = true;
        }

        last_seq = msg->seq;
        questions++;
    } else if (chan == &chan_render) {
        const struct tk_render_msg *msg = zbus_chan_const_msg(chan);

        renders++;

        if (msg->was_full) {
            full_renders++;
        }

        if (msg->result != 0) {
            failed_renders++;
        }
    }
}

ZBUS_LISTENER_DEFINE(test_obs, observe);
ZBUS_CHAN_ADD_OBS(chan_question, test_obs, 4);
ZBUS_CHAN_ADD_OBS(chan_render, test_obs, 4);

static void *suite_setup(void)
{
    /* Emulated pins read low by default, which for an active-low button means
     * held down. Release it before the input layer looks. */
    zassert_ok(gpio_emul_input_set(next_button.port, next_button.pin, 1));
    k_sleep(K_MSEC(100));

    zassert_ok(tk_input_init());

    return NULL;
}

ZTEST_SUITE(tk_soak, NULL, suite_setup, NULL, NULL, NULL);

ZTEST(tk_soak, test_the_chain_runs_hands_off)
{
    /* Nothing is touched at all: the device boots onto its remembered deck,
     * names it, and the soak driver takes over from there. */
    k_sleep(RUN_WAIT);

    zassert_true(renders >= EXPECTED_RENDERS,
                 "the chain stopped after %u refreshes; the soak run needs it to keep pressing",
                 renders);
    /* Not equality: the sleep can end with the last question mid-refresh. */
    zassert_true(questions >= renders, "a render arrived for a question nobody published");
    zassert_false(seq_repeated, "each press must draw a new card, not redraw the last one");
    zassert_equal(failed_renders, 0, "no refresh should have failed");

    /*
     * The measurement the whole image exists for: ghosting cannot be watched
     * accumulating if a full refresh keeps clearing it. Only the first one —
     * the panel holds an image this firmware did not draw — may be full.
     */
    zassert_equal(full_renders, 1,
                  "soak.conf should have suppressed every full refresh but the "
                  "first, and %u came through",
                  full_renders);
}
