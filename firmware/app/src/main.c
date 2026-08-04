/*
 * Boot.
 *
 * Everything that runs afterwards is a thread: `app` decides (src/app.c),
 * `display` renders (src/display.c), and the system workqueue debounces the
 * selector and Next (src/input.c). main() only starts the input layer and
 * reports what came up — the threads are already running by the time it does,
 * because K_THREAD_DEFINE starts them at boot.
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
 * to the pin named by the `zephyr,user` node. It follows Next presses, which
 * is the one input with no other visible effect until the panel is wired.
 */
static const struct gpio_dt_spec blink = GPIO_DT_SPEC_GET(DT_PATH(zephyr_user), blink_gpios);

/*
 * Follows what actually reached the panel, not what was pressed. Toggling on
 * chan_next instead looks right and is a lie: a press made during a refresh is
 * deliberately dropped, and an LED that winks at it tells the table the button
 * worked when nothing is going to happen.
 */
static void on_drawn(const struct zbus_channel *chan)
{
    ARG_UNUSED(chan);

    if (device_is_ready(blink.port)) {
        (void) gpio_pin_toggle_dt(&blink);
    }
}

ZBUS_LISTENER_DEFINE(main_blink, on_drawn);
ZBUS_CHAN_ADD_OBS(chan_question, main_blink, 4);

int main(void)
{
    LOG_INF("tischkarte on %s", CONFIG_BOARD_TARGET);

    if (gpio_is_ready_dt(&blink)) {
        (void) gpio_pin_configure_dt(&blink, GPIO_OUTPUT_INACTIVE);
    } else {
        LOG_WRN("GPIO port %s is not ready; running without the LED", blink.port->name);
    }

    const int ret = tk_input_init();

    if (ret != 0) {
        LOG_ERR("input init failed: %d", ret);
        return ret;
    }

    return 0;
}
