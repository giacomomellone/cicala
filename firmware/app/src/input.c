/*
 * Selector and Next.
 *
 * Zephyr's input subsystem does the per-contact debounce: the `gpio-keys`
 * nodes in the board overlay are driven by the in-tree driver, which owns the
 * interrupt and the debounce-interval-ms delayed work. What is left — and what
 * is actually ours — is two rules:
 *
 *   1. The selector is one-hot. Exactly one closed contact is a deck; zero or
 *      several is not a deck and must not change anything.
 *   2. A position has to hold still for TK_SELECTOR_SETTLE_MS before it counts,
 *      so turning the knob across three detents draws one question, not three.
 *
 * Rule 2 is a k_work_reschedule on every edge: the timer restarts while the
 * knob is moving and only expires once it stops. There is no input thread —
 * see the CONFIG_INPUT_MODE_SYNCHRONOUS note below.
 */

#include "input.h"

#include <zephyr/drivers/gpio.h>
#include <zephyr/dt-bindings/input/input-event-codes.h>
#include <zephyr/input/input.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/math_extras.h>
#include <zephyr/sys/util.h>

#include "channels.h"

LOG_MODULE_REGISTER(tk_input, LOG_LEVEL_INF);

/*
 * Deck 0..5 report INPUT_BTN_0..INPUT_BTN_5, which are 0x100..0x105 and so
 * subtract to an index. INPUT_KEY_0..INPUT_KEY_5 look like the obvious choice
 * and are not: they are keyboard scancodes (KEY_1 is 2, KEY_0 is 11), so they
 * are neither contiguous nor in numeric order.
 */
#define DECK_CODE_FIRST INPUT_BTN_0
#define NEXT_CODE INPUT_KEY_ENTER

#define DECK_SPEC(nodelabel) GPIO_DT_SPEC_GET(DT_NODELABEL(nodelabel), gpios)

/*
 * Indexed by deck, not by devicetree child order: the nodelabels are named for
 * their position so the mapping survives someone reordering the overlay.
 */
static const struct gpio_dt_spec selector_pins[TK_DECK_COUNT] = {
    DECK_SPEC(tk_deck_0), DECK_SPEC(tk_deck_1), DECK_SPEC(tk_deck_2),
    DECK_SPEC(tk_deck_3), DECK_SPEC(tk_deck_4), DECK_SPEC(tk_deck_5),
};

/*
 * One bit per closed contact. Written by the input callback and read by the
 * settle handler, both of which run on the system workqueue: the input
 * subsystem is configured for synchronous mode, so a callback executes in the
 * context that reported the event, and gpio-keys reports from its own delayed
 * work. Same context, so no lock. Switching to CONFIG_INPUT_MODE_THREAD would
 * break that assumption.
 */
static uint8_t contact_mask;

/** Uptime when Next went down; negative while the button is up. */
static int64_t next_press_ms = -1;

static void selector_settled(struct k_work *work);
static K_WORK_DELAYABLE_DEFINE(settle_work, selector_settled);

/** Publish only on an actual change, so a quiet table produces no traffic. */
static void publish_selector(uint8_t deck, bool valid)
{
    struct tk_selector_msg current;

    if (zbus_chan_read(&chan_selector, &current, K_MSEC(10)) == 0) {
        if (current.valid == valid && (!valid || current.deck == deck)) {
            return;
        }
    }

    const struct tk_selector_msg msg = {
        .deck = deck,
        .valid = valid,
    };

    (void) zbus_chan_pub(&chan_selector, &msg, K_MSEC(10));
}

/**
 * Decide what the mask means, once it has stopped changing.
 *
 * An invalid mask keeps the last deck: `valid` goes false and `deck` is left
 * alone, so a reader that ignores the flag still shows something sensible
 * rather than snapping to new_people.
 */
static void selector_settled(struct k_work *work)
{
    ARG_UNUSED(work);

    if (POPCOUNT(contact_mask) != 1) {
        struct tk_selector_msg current;
        const uint8_t deck =
            zbus_chan_read(&chan_selector, &current, K_MSEC(10)) == 0 ? current.deck : 0;

        publish_selector(deck, false);
        return;
    }

    publish_selector((uint8_t) u32_count_trailing_zeros(contact_mask), true);
}

static void handle_next(int32_t value)
{
    if (value != 0) {
        next_press_ms = k_uptime_get();
        return;
    }

    if (next_press_ms < 0) {
        /* Released without a press we saw — held down across boot. */
        return;
    }

    /*
     * Published on release rather than on press, which is what makes the
     * duration honest. No policy reads it (docs/firmware_architecture.md):
     * long press is Next, same as short.
     */
    const struct tk_next_msg msg = {
        .timestamp_ms = next_press_ms,
        .duration_ms = (uint32_t) (k_uptime_get() - next_press_ms),
    };

    next_press_ms = -1;

    (void) zbus_chan_pub(&chan_next, &msg, K_MSEC(10));
}

static void tk_input_cb(struct input_event *evt, void *user_data)
{
    ARG_UNUSED(user_data);

    if (evt->type != INPUT_EV_KEY) {
        return;
    }

    if (evt->code == NEXT_CODE) {
        handle_next(evt->value);
        return;
    }

    const uint16_t deck = evt->code - DECK_CODE_FIRST;

    if (deck >= TK_DECK_COUNT) {
        return;
    }

    WRITE_BIT(contact_mask, deck, evt->value != 0);

    /* Restart the window: the knob is still moving. */
    (void) k_work_reschedule(&settle_work, K_MSEC(CONFIG_TK_SELECTOR_SETTLE_MS));
}

INPUT_CALLBACK_DEFINE(NULL, tk_input_cb, NULL);

int tk_input_init(void)
{
    contact_mask = 0;

    for (size_t i = 0; i < TK_DECK_COUNT; i++) {
        if (!gpio_is_ready_dt(&selector_pins[i])) {
            LOG_ERR("selector contact %u is not ready", (unsigned int) i);
            return -ENODEV;
        }

        const int closed = gpio_pin_get_dt(&selector_pins[i]);

        if (closed < 0) {
            LOG_ERR("selector contact %u read failed: %d", (unsigned int) i, closed);
            return closed;
        }

        WRITE_BIT(contact_mask, i, closed != 0);
    }

    /*
     * No settle window here: a contact that is already closed at boot has by
     * definition been stable longer than any window, and the wake path has
     * under a second in total to reach a drawn question.
     */
    selector_settled(NULL);

    return 0;
}
