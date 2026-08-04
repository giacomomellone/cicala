/*
 * Input bring-up. Still not the product.
 *
 * This is the console face of the two input channels: turn the selector and a
 * deck line appears, press Next and a press line appears. It proves the whole
 * chain — devicetree, gpio-keys, the settle window, zbus — with nothing on the
 * SPI bus yet, and it is what `just fw-monitor` shows.
 *
 * The decision maker that replaces it is `app_fsm` in
 * docs/firmware_architecture.md. Its inputs are exactly these two channels, so
 * that change adds a thread and deletes this loop rather than rewiring
 * anything.
 */

#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/zbus/zbus.h>

#include "channels.h"
#include "input.h"

LOG_MODULE_REGISTER(tk_main, LOG_LEVEL_INF);

/*
 * Visual confirm on the breadboard, next to the console line: the DevKitC's
 * own LED is an addressable WS2812 on GPIO48, so this is a discrete LED wired
 * to the pin named by the `zephyr,user` node.
 */
static const struct gpio_dt_spec blink = GPIO_DT_SPEC_GET(DT_PATH(zephyr_user), blink_gpios);

ZBUS_MSG_SUBSCRIBER_DEFINE(main_sub);
ZBUS_CHAN_ADD_OBS(chan_selector, main_sub, 3);
ZBUS_CHAN_ADD_OBS(chan_next, main_sub, 3);

static void report_selector(const struct tk_selector_msg *msg)
{
    if (!msg->valid) {
        LOG_INF("selector invalid - deck unchanged");
        return;
    }

    LOG_INF("deck %u %s", msg->deck, tk_deck_name(msg->deck));
}

static void report_next(const struct tk_next_msg *msg)
{
    LOG_INF("next (held %u ms)", msg->duration_ms);

    if (device_is_ready(blink.port)) {
        (void) gpio_pin_toggle_dt(&blink);
    }
}

int main(void)
{
    LOG_INF("tischkarte input on %s", CONFIG_BOARD_TARGET);

    if (gpio_is_ready_dt(&blink)) {
        (void) gpio_pin_configure_dt(&blink, GPIO_OUTPUT_INACTIVE);
    } else {
        LOG_WRN("GPIO port %s is not ready; running without the LED", blink.port->name);
    }

    int ret = tk_input_init();

    if (ret != 0) {
        LOG_ERR("input init failed: %d", ret);
        return ret;
    }

    /*
     * Blocking with no timeout is the point, not an accident: once the
     * application has power management, a main loop that woke periodically
     * would be the one thing keeping the device out of deep sleep.
     */
    while (true) {
        const struct zbus_channel *chan;
        union {
            struct tk_selector_msg selector;
            struct tk_next_msg next;
        } msg;

        ret = zbus_sub_wait_msg(&main_sub, &chan, &msg, K_FOREVER);

        if (ret != 0) {
            LOG_ERR("zbus wait failed: %d", ret);
            continue;
        }

        if (chan == &chan_selector) {
            report_selector(&msg.selector);
        } else if (chan == &chan_next) {
            report_next(&msg.next);
        }
    }

    return 0;
}
