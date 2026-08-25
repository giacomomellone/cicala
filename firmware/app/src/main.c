#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/hwinfo.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/zbus/zbus.h>

#include "channels.h"
#include "input.h"
#include "update_notice.h"

LOG_MODULE_REGISTER(cicala_main, LOG_LEVEL_INF);

#ifdef CONFIG_SETTINGS

#include <zephyr/settings/settings.h>

/* Read the `cicala` subtree back out of NVS. */
static int load_settings(void)
{
    int err = settings_subsys_init();

    if (err != 0) {
        LOG_ERR("settings init failed: %d", err);

        return 0;
    }

    err = settings_load();

    if (err != 0) {
        LOG_ERR("settings load failed: %d", err);
    }

    /* Missing settings leaves defaults intact. */
    return 0;
}

SYS_INIT(load_settings, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);

#endif /* CONFIG_SETTINGS */

static void log_reset_cause(void)
{
    uint32_t cause = 0;

    if (hwinfo_get_reset_cause(&cause) != 0) {
        LOG_INF("reset: cause not available on this target");
        return;
    }

    /* Clear accumulated reset causes after reading them. */
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

/* Hold the DevKitC's onboard WS2812 data line low. */
static void hold_onboard_led_quiet(void)
{
    /* _OR: only the devkit overlay has this LED. */
    static const struct gpio_dt_spec ws2812 =
        GPIO_DT_SPEC_GET_OR(DT_PATH(zephyr_user), ws2812_gpios, {0});

    if (ws2812.port != NULL && gpio_is_ready_dt(&ws2812)) {
        (void) gpio_pin_configure_dt(&ws2812, GPIO_OUTPUT_INACTIVE);
    }
}

int main(void)
{
    LOG_INF("cicala on %s", CONFIG_BOARD_TARGET);

    log_reset_cause();
    hold_onboard_led_quiet();

    const int ret = cicala_input_init();

    if (ret != 0) {
        LOG_ERR("input init failed: %d", ret);
        return ret;
    }

    /* Publishing a notice requires the app thread to be running. */
    cicala_update_notice_check();

    return 0;
}
