/*
 * Category and Next, driven through emulated GPIO.
 *
 * This is the real src/input.c against the real gpio-keys driver: the suite
 * moves pins and waits out the debounce, so what it checks is the timing rule
 * itself rather than a mock of it.
 *
 * Buttons are active low, so a pressed button is a physical 0.
 */

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/gpio/gpio_emul.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

#include "channels.h"
#include "input.h"

static const struct gpio_dt_spec category = GPIO_DT_SPEC_GET(DT_ALIAS(tk_category), gpios);
static const struct gpio_dt_spec next_button = GPIO_DT_SPEC_GET(DT_ALIAS(tk_next), gpios);

/* Long enough for the driver to debounce and report, with margin for a slow
 * host. */
#define PRESS_WAIT K_MSEC(CONFIG_TK_BUTTON_DEBOUNCE_MS + 150)

static int category_publishes;
static int next_publishes;
static struct tk_category_msg last_category;
static struct tk_next_msg last_next;

static void observe(const struct zbus_channel *chan)
{
    if (chan == &chan_category) {
        category_publishes++;
        last_category = *(const struct tk_category_msg *) zbus_chan_const_msg(chan);
    } else if (chan == &chan_next) {
        next_publishes++;
        last_next = *(const struct tk_next_msg *) zbus_chan_const_msg(chan);
    }
}

ZBUS_LISTENER_DEFINE(test_obs, observe);
ZBUS_CHAN_ADD_OBS(chan_category, test_obs, 4);
ZBUS_CHAN_ADD_OBS(chan_next, test_obs, 4);

static void press(const struct gpio_dt_spec *button, bool down)
{
    zassert_ok(gpio_emul_input_set(button->port, button->pin, down ? 0 : 1));
}

static void tap(const struct gpio_dt_spec *button, int hold_ms)
{
    press(button, true);
    k_sleep(K_MSEC(hold_ms));
    press(button, false);
    k_sleep(PRESS_WAIT);
}

static void *suite_setup(void)
{
    /* Emulated pins read low by default, which for an active-low button means
     * held down from the start. Release both before the input layer looks. */
    press(&category, false);
    press(&next_button, false);
    k_sleep(PRESS_WAIT);

    zassert_ok(tk_input_init());

    return NULL;
}

static void before_each(void *fixture)
{
    ARG_UNUSED(fixture);

    category_publishes = 0;
    next_publishes = 0;
}

ZTEST_SUITE(tk_input, NULL, suite_setup, before_each, NULL, NULL);

ZTEST(tk_input, test_one_category_press_is_one_event)
{
    tap(&category, 60);

    zassert_equal(category_publishes, 1, "one press is one event");
    zassert_equal(next_publishes, 0, "and it must not look like Next");
}

ZTEST(tk_input, test_one_next_press_is_one_event)
{
    tap(&next_button, 60);

    zassert_equal(next_publishes, 1, "one press is one event");
    zassert_equal(category_publishes, 0, "and it must not look like Category");
}

ZTEST(tk_input, test_a_press_is_reported_on_release)
{
    /*
     * Publishing on release rather than on press is what makes the duration
     * honest. No policy reads it — a long press means the same as a short one
     * — but a table study asks whether anyone tried to hold.
     */
    press(&next_button, true);
    k_sleep(PRESS_WAIT);

    zassert_equal(next_publishes, 0, "held down is not yet a press");

    press(&next_button, false);
    k_sleep(PRESS_WAIT);

    zassert_equal(next_publishes, 1);
    zassert_true(last_next.duration_ms >= (uint32_t) CONFIG_TK_BUTTON_DEBOUNCE_MS,
                 "the reported duration should cover the hold, got %u ms", last_next.duration_ms);
}

ZTEST(tk_input, test_a_long_press_is_still_one_event)
{
    tap(&next_button, 900);

    zassert_equal(next_publishes, 1, "long press is Next, same as a short one");
}

ZTEST(tk_input, test_the_buttons_are_independent)
{
    /*
     * Category cannot disturb Next and vice versa. That independence is the
     * reason there are two buttons rather than one with a press vocabulary.
     */
    press(&category, true);
    k_sleep(PRESS_WAIT);

    tap(&next_button, 60);

    press(&category, false);
    k_sleep(PRESS_WAIT);

    zassert_equal(next_publishes, 1, "Next reports while Category is held");
    zassert_equal(category_publishes, 1, "and Category reports when it is let go");
    zassert_true(last_category.duration_ms > 0);
}
