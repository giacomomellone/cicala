/*
 * The two buttons.
 *
 * Zephyr's input subsystem does the work: the `gpio-keys` node in the board
 * overlay is driven by the in-tree driver, which owns the interrupt and the
 * per-contact debounce. What is left here is turning a debounced key event
 * into the channel the application speaks in, and measuring how long the
 * button was held so a table study can ask whether anyone tried.
 *
 * Category advances the deck; Next asks for a question. Neither has a press
 * duration that means anything.
 *
 * There is no input thread — see the CONFIG_INPUT_MODE_SYNCHRONOUS note below.
 */

#include "input.h"

#include <zephyr/drivers/gpio.h>
#include <zephyr/dt-bindings/input/input-event-codes.h>
#include <zephyr/input/input.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include "channels.h"

LOG_MODULE_REGISTER(tk_input, LOG_LEVEL_INF);

#define CATEGORY_CODE INPUT_KEY_MENU
#define NEXT_CODE INPUT_KEY_ENTER

/** Uptime when each button went down; negative while it is up. */
static int64_t category_press_ms = -1;
static int64_t next_press_ms = -1;

/**
 * Turn a press-and-release into one event.
 *
 * Published on release rather than on press, which is what makes the duration
 * honest. No policy reads it: a long press means the same as a short one, for
 * both buttons.
 *
 * A release with no press behind it is a button that was already held when the
 * device booted — which, once deep sleep exists, is exactly how a wake looks if
 * someone leans on the button. Ignored rather than invented.
 */
static bool press_completed(int32_t value, int64_t *down_at, uint32_t *duration_ms)
{
    if (value != 0) {
        *down_at = k_uptime_get();
        return false;
    }

    if (*down_at < 0) {
        return false;
    }

    *duration_ms = (uint32_t) (k_uptime_get() - *down_at);
    *down_at = -1;

    return true;
}

static void tk_input_cb(struct input_event *evt, void *user_data)
{
    ARG_UNUSED(user_data);

    if (evt->type != INPUT_EV_KEY) {
        return;
    }

    uint32_t duration_ms = 0;

    if (evt->code == CATEGORY_CODE) {
        if (press_completed(evt->value, &category_press_ms, &duration_ms)) {
            const struct tk_category_msg msg = {
                .timestamp_ms = k_uptime_get() - duration_ms,
                .duration_ms = duration_ms,
            };

            (void) zbus_chan_pub(&chan_category, &msg, K_MSEC(10));
        }

        return;
    }

    if (evt->code == NEXT_CODE) {
        if (press_completed(evt->value, &next_press_ms, &duration_ms)) {
            const struct tk_next_msg msg = {
                .timestamp_ms = k_uptime_get() - duration_ms,
                .duration_ms = duration_ms,
            };

            (void) zbus_chan_pub(&chan_next, &msg, K_MSEC(10));
        }
    }
}

INPUT_CALLBACK_DEFINE(NULL, tk_input_cb, NULL);

int tk_input_init(void)
{
    static const struct gpio_dt_spec buttons[] = {
        GPIO_DT_SPEC_GET(DT_ALIAS(tk_category), gpios),
        GPIO_DT_SPEC_GET(DT_ALIAS(tk_next), gpios),
    };

    for (size_t i = 0; i < ARRAY_SIZE(buttons); i++) {
        if (!gpio_is_ready_dt(&buttons[i])) {
            LOG_ERR("button %u is not ready", (unsigned int) i);
            return -ENODEV;
        }
    }

    category_press_ms = -1;
    next_press_ms = -1;

    return 0;
}
