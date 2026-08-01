/*
 * Bring-up blinky. Not the product.
 *
 * This exists to prove one thing end to end: build, flash, devicetree, GPIO.
 * It is replaced by the state machine in docs/firmware_architecture.md once
 * the input and display components land.
 *
 * The pin comes from the `zephyr,user` node in the board overlay rather than a
 * number in this file, so moving the LED is an overlay edit and the same
 * source builds for the devkit and for a host platform.
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(tk_main, LOG_LEVEL_INF);

#define BLINK_INTERVAL K_MSEC(500)

static const struct gpio_dt_spec blink = GPIO_DT_SPEC_GET(DT_PATH(zephyr_user), blink_gpios);

int main(void)
{
    LOG_INF("tischkarte bring-up on %s", CONFIG_BOARD_TARGET);

    if (!gpio_is_ready_dt(&blink)) {
        LOG_ERR("GPIO port %s is not ready", blink.port->name);
        return -ENODEV;
    }

    int ret = gpio_pin_configure_dt(&blink, GPIO_OUTPUT_ACTIVE);
    if (ret != 0) {
        LOG_ERR("failed to configure pin %d: %d", blink.pin, ret);
        return ret;
    }

    LOG_INF("blinking on pin %d every %d ms", blink.pin, 500);

    while (true) {
        gpio_pin_toggle_dt(&blink);
        k_sleep(BLINK_INTERVAL);
    }

    return 0;
}
