/*
 * Selector and Next behaviour, driven through emulated GPIO.
 *
 * This is the real src/input.c against the real gpio-keys driver: the suite
 * moves pins and then waits out the debounce and the settle window, so what it
 * checks is the timing rule itself rather than a mock of it. That costs about
 * a second of wall clock per case and is worth it — the drop-a-detent rule is
 * the one input behaviour a user would notice breaking.
 *
 * Contacts are active low, so a closed contact is a physical 0.
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

/* Debounce plus settle plus enough margin that a slow host is not a failure. */
#define SETTLE_WAIT K_MSEC(CONFIG_TK_SELECTOR_SETTLE_MS + CONFIG_TK_NEXT_DEBOUNCE_MS + 150)
/* Long enough for the driver to report an edge, short enough not to settle. */
#define DEBOUNCE_WAIT K_MSEC(CONFIG_TK_NEXT_DEBOUNCE_MS + 70)

static int selector_publishes;
static int next_publishes;
static struct tk_selector_msg last_selector;
static struct tk_next_msg last_next;

static void observe(const struct zbus_channel *chan)
{
    if (chan == &chan_selector) {
        selector_publishes++;
        last_selector = *(const struct tk_selector_msg *) zbus_chan_const_msg(chan);
    } else if (chan == &chan_next) {
        next_publishes++;
        last_next = *(const struct tk_next_msg *) zbus_chan_const_msg(chan);
    }
}

ZBUS_LISTENER_DEFINE(test_obs, observe);
ZBUS_CHAN_ADD_OBS(chan_selector, test_obs, 4);
ZBUS_CHAN_ADD_OBS(chan_next, test_obs, 4);

static void set_contact(int deck, bool closed)
{
    zassert_ok(gpio_emul_input_set(contacts[deck].port, contacts[deck].pin, closed ? 0 : 1));
}

static void set_next(bool pressed)
{
    zassert_ok(gpio_emul_input_set(next_button.port, next_button.pin, pressed ? 0 : 1));
}

static void reset_counters(void)
{
    selector_publishes = 0;
    next_publishes = 0;
}

static void *suite_setup(void)
{
    for (int i = 0; i < TK_DECK_COUNT; i++) {
        set_contact(i, false);
    }

    set_next(false);
    k_sleep(DEBOUNCE_WAIT);

    zassert_ok(tk_input_init());

    return NULL;
}

/* Every case starts from every contact open and the button up. */
static void before_each(void *fixture)
{
    ARG_UNUSED(fixture);

    for (int i = 0; i < TK_DECK_COUNT; i++) {
        set_contact(i, false);
    }

    set_next(false);
    k_sleep(SETTLE_WAIT);
    reset_counters();
}

ZTEST_SUITE(tk_input, NULL, suite_setup, before_each, NULL, NULL);

ZTEST(tk_input, test_one_contact_selects_its_deck)
{
    set_contact(3, true);
    k_sleep(SETTLE_WAIT);

    zassert_equal(selector_publishes, 1, "expected exactly one publish, got %d",
                  selector_publishes);
    zassert_true(last_selector.valid);
    zassert_equal(last_selector.deck, 3);
}

ZTEST(tk_input, test_crossing_detents_publishes_once)
{
    /*
     * Turning the knob from 0 to 2 passes through 1. Each transient closes a
     * contact for less than the settle window, so only the position the knob
     * stops at may be published.
     */
    set_contact(0, true);
    k_sleep(DEBOUNCE_WAIT);

    set_contact(0, false);
    set_contact(1, true);
    k_sleep(DEBOUNCE_WAIT);

    set_contact(1, false);
    set_contact(2, true);
    k_sleep(SETTLE_WAIT);

    zassert_equal(selector_publishes, 1, "one turn must be one question, got %d publishes",
                  selector_publishes);
    zassert_true(last_selector.valid);
    zassert_equal(last_selector.deck, 2);
}

ZTEST(tk_input, test_no_contact_is_invalid_and_keeps_the_deck)
{
    set_contact(4, true);
    k_sleep(SETTLE_WAIT);
    zassert_equal(last_selector.deck, 4);
    reset_counters();

    set_contact(4, false);
    k_sleep(SETTLE_WAIT);

    zassert_equal(selector_publishes, 1);
    zassert_false(last_selector.valid, "no closed contact is not a deck");
    zassert_equal(last_selector.deck, 4, "an invalid selector must not move the deck");
}

ZTEST(tk_input, test_two_contacts_are_invalid)
{
    set_contact(1, true);
    set_contact(5, true);
    k_sleep(SETTLE_WAIT);

    zassert_false(last_selector.valid, "several closed contacts is not a deck");
}

ZTEST(tk_input, test_a_press_publishes_one_event_with_its_duration)
{
    set_next(true);
    k_sleep(K_MSEC(200));
    set_next(false);
    k_sleep(DEBOUNCE_WAIT);

    zassert_equal(next_publishes, 1, "one press is one event, got %d", next_publishes);
    zassert_true(last_next.timestamp_ms > 0);

    /*
     * Both edges are reported a debounce interval late, so the measured
     * duration tracks the real one; the bounds are wide enough for a loaded
     * host and still exclude a stuck-at-zero or a doubled interval.
     */
    zassert_between_inclusive(last_next.duration_ms, 100, 400, "duration was %u ms",
                              last_next.duration_ms);
}

ZTEST(tk_input, test_a_bouncing_press_publishes_once)
{
    /* Make, break, make again inside the debounce interval. */
    set_next(true);
    set_next(false);
    set_next(true);
    k_sleep(K_MSEC(150));

    set_next(false);
    k_sleep(DEBOUNCE_WAIT);

    zassert_equal(next_publishes, 1, "chatter must not produce extra events, got %d",
                  next_publishes);
}

ZTEST(tk_input, test_a_held_contact_is_read_at_boot)
{
    /*
     * The gpio-keys driver only reports edges and samples each pin once at its
     * own init, so a selector already in position when power arrives produces
     * no event at all. After deep sleep every boot is that case, which is why
     * tk_input_init() reads the pins itself.
     */
    set_contact(5, true);
    k_sleep(SETTLE_WAIT);
    reset_counters();

    /* Force the channel back to "no deck", then re-run the boot path. */
    const struct tk_selector_msg unknown = {.deck = 0, .valid = false};

    zassert_ok(zbus_chan_pub(&chan_selector, &unknown, K_MSEC(100)));
    reset_counters();

    zassert_ok(tk_input_init());

    zassert_equal(selector_publishes, 1, "boot must publish the position it found");
    zassert_true(last_selector.valid);
    zassert_equal(last_selector.deck, 5);
}
