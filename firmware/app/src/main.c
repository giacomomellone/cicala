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
#include <zephyr/drivers/hwinfo.h>
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

/**
 * Say why the chip started.
 *
 * Retained state survives some of these and not others, and which is which is
 * a property of the silicon rather than of this firmware: a software restart
 * and a deep-sleep wake keep RTC memory, a power-on reset does not, and the
 * reset pin is the one nobody should have to guess about. Printing the cause
 * next to whether the block survived makes every reset an experiment that
 * reports its own result — including, later, the one that matters, when this
 * line reads `low-power wake`.
 */
static void log_reset_cause(void)
{
    uint32_t cause = 0;

    if (hwinfo_get_reset_cause(&cause) != 0) {
        LOG_INF("reset: cause not available on this target");
        return;
    }

    /* Cleared so the next boot reports its own cause rather than the union of
     * every cause since power-on. */
    (void) hwinfo_clear_reset_cause();

    if (cause == 0) {
        LOG_INF("reset: none reported");
    } else if (cause & RESET_POR) {
        LOG_INF("reset: power-on — RTC memory is not expected to survive this");
    } else if (cause & RESET_LOW_POWER_WAKE) {
        LOG_INF("reset: low-power wake");
    } else if (cause & RESET_SOFTWARE) {
        LOG_INF("reset: software");
    } else if (cause & RESET_PIN) {
        LOG_INF("reset: pin");
    } else if (cause & RESET_BROWNOUT) {
        LOG_WRN("reset: brownout");
    } else if (cause & RESET_WATCHDOG) {
        LOG_WRN("reset: watchdog");
    } else {
        LOG_INF("reset: cause 0x%08x", cause);
    }
}

/**
 * Hold the DevKitC's onboard WS2812 data line low.
 *
 * GPIO48 drives an addressable LED this firmware never uses. Left as a
 * floating input it picks up enough noise to clock a colour into the LED's
 * shift register, which then latches: the light stays on through resets,
 * because the LED holds its own state rather than the pin holding it.
 *
 * Driving the line low stops anything further being clocked in. It cannot turn
 * off a colour already latched — that needs a zero pixel over RMT, and the
 * target PCB has no such LED to justify the driver. Power-cycle to clear one.
 */
static void hold_onboard_led_quiet(void)
{
    /* _OR: only the devkit overlay has this LED. qemu and native_sim do not,
     * and neither will the target PCB. */
    static const struct gpio_dt_spec ws2812 =
        GPIO_DT_SPEC_GET_OR(DT_PATH(zephyr_user), ws2812_gpios, {0});

    if (ws2812.port != NULL && gpio_is_ready_dt(&ws2812)) {
        (void) gpio_pin_configure_dt(&ws2812, GPIO_OUTPUT_INACTIVE);
    }
}

int main(void)
{
    LOG_INF("tischkarte on %s", CONFIG_BOARD_TARGET);

    log_reset_cause();
    hold_onboard_led_quiet();

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
