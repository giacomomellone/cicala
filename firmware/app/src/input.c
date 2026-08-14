/* The two buttons. */

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

/* Uptime when each button went down; negative while it is up. */
static int64_t category_press_ms = -1;
static int64_t next_press_ms = -1;

/* Turn a press-and-release into one event. */
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
