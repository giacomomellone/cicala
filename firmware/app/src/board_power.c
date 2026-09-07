#include <zephyr/drivers/gpio.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>

#define USER_NODE DT_PATH(zephyr_user)

static const struct gpio_dt_spec panel_power =
    GPIO_DT_SPEC_GET(USER_NODE, cicala_panel_power_gpios);
static const struct gpio_dt_spec charge_disable =
    GPIO_DT_SPEC_GET(USER_NODE, cicala_charge_disable_gpios);

BUILD_ASSERT(CONFIG_DISPLAY_INIT_PRIORITY > 70,
             "The panel supply must settle before the display driver starts");

static int board_power_start(void)
{
    if (!gpio_is_ready_dt(&panel_power) || !gpio_is_ready_dt(&charge_disable)) {
        return -ENODEV;
    }

    /* Q2 permits this request only while the independent NTC window is valid. */
    int err = gpio_pin_configure_dt(&charge_disable, GPIO_OUTPUT_INACTIVE);

    if (err != 0) {
        return err;
    }

    err = gpio_pin_configure_dt(&panel_power, GPIO_OUTPUT_ACTIVE);
    if (err != 0) {
        return err;
    }

    /* TPS22917 with 1 nF CT: allow its ramp before SSD1680 reset and commands. */
    k_busy_wait(20000);
    return 0;
}

SYS_INIT(board_power_start, POST_KERNEL, 70);
