/* Deep sleep, and the wake mask that makes it possible. */

#include <driver/rtc_io.h>
#include <esp_sleep.h>

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/mipi_dbi.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/logging/log_ctrl.h>
#include <zephyr/sys/poweroff.h>
#include <zephyr/sys/util.h>
#include <zephyr/zbus/zbus.h>

#include "app_logic.h"
#include "channels.h"
#include "net.h"
#include "power.h"
#include "sleep.h"
#include "status.h"

LOG_MODULE_REGISTER(cicala_sleep, LOG_LEVEL_INF);

/* Sleeping builds require the patched SSD16xx initialization policy. */
#ifndef CONFIG_SSD16XX_PRESERVE_IMAGE_ON_INIT
#error "sleeping image needs the ssd16xx patch — run `just fw-patch`"
#endif

static const struct gpio_dt_spec wake_pins[] = {
    GPIO_DT_SPEC_GET(DT_ALIAS(cicala_category), gpios),
    GPIO_DT_SPEC_GET(DT_ALIAS(cicala_next), gpios),
};

#if IS_ENABLED(CONFIG_CICALA_POWER_WAKE_ON_USB)

/* Use the same VBUS devicetree property as power.c. */
static const struct gpio_dt_spec vbus_pin =
    GPIO_DT_SPEC_GET(DT_PATH(zephyr_user), cicala_vbus_gpios);

#endif

static const struct gpio_dt_spec panel_pins[] = {
    GPIO_DT_SPEC_GET(DT_NODELABEL(cicala_mipi_dbi), reset_gpios),
    GPIO_DT_SPEC_GET(DT_NODELABEL(cicala_mipi_dbi), dc_gpios),
    GPIO_DT_SPEC_GET_BY_IDX(DT_NODELABEL(spi2), cs_gpios, 0),
#if IS_ENABLED(CONFIG_CICALA_REV_A)
    GPIO_DT_SPEC_GET(DT_PATH(zephyr_user), cicala_panel_mosi_gpios),
    GPIO_DT_SPEC_GET(DT_PATH(zephyr_user), cicala_panel_clock_gpios),
    GPIO_DT_SPEC_GET(DT_PATH(zephyr_user), cicala_battery_enable_gpios),
    /* Power is last: park every driven signal before removing the rail. */
    GPIO_DT_SPEC_GET(DT_PATH(zephyr_user), cicala_panel_power_gpios),
#endif
};

#if IS_ENABLED(CONFIG_CICALA_PANEL_DEEP_SLEEP)

#define CICALA_PANEL_NODE DT_CHOSEN(zephyr_display)

/* From ssd16xx_regs.h, which is private to the driver and cannot be included from here. */
#define CICALA_SSD16XX_CMD_SLEEP_MODE 0x10
#define CICALA_SSD16XX_SLEEP_MODE_DSM1 0x01

static const struct device *const panel_bus = DEVICE_DT_GET(DT_PARENT(CICALA_PANEL_NODE));

/* The same bus configuration ssd16xx builds for itself, from the same node. */
static const struct mipi_dbi_config panel_dbi = {
    .mode = MIPI_DBI_MODE_SPI_4WIRE,
    .config = MIPI_DBI_SPI_CONFIG_DT(
        CICALA_PANEL_NODE, SPI_OP_MODE_MASTER | SPI_WORD_SET(8) | SPI_HOLD_ON_CS | SPI_LOCK_ON, 0),
};

static const struct gpio_dt_spec panel_busy = GPIO_DT_SPEC_GET(CICALA_PANEL_NODE, busy_gpios);

static void panel_controller_sleep(void)
{
    const uint8_t mode = CICALA_SSD16XX_SLEEP_MODE_DSM1;

    for (int waited = 0; gpio_pin_get_dt(&panel_busy) > 0; waited++) {
        if (waited >= CONFIG_CICALA_REFRESH_TIMEOUT_MS) {
            LOG_ERR("panel still busy; leaving the controller awake");
            return;
        }

        k_msleep(1);
    }

    const int err =
        mipi_dbi_command_write(panel_bus, &panel_dbi, CICALA_SSD16XX_CMD_SLEEP_MODE, &mode, 1);

    (void) mipi_dbi_release(panel_bus, &panel_dbi);

    if (err != 0) {
        LOG_ERR("could not sleep the panel controller: %d", err);
        return;
    }

    LOG_INF("panel controller in deep sleep mode 1");
}

#else

static void panel_controller_sleep(void) {}

#endif /* CONFIG_CICALA_PANEL_DEEP_SLEEP */

static void sleep_now(struct k_work *work);
static K_WORK_DELAYABLE_DEFINE(idle_work, sleep_now);

/* Latched at PRE_KERNEL_1, read by the app thread. */
static enum cicala_wake_source wake_button = CICALA_WAKE_NONE;

enum cicala_wake_source cicala_wake_button(void)
{
    return wake_button;
}

/* Read the wake mask before anything else can disturb it. */
static int latch_wake_button(void)
{
    if (!(esp_sleep_get_wakeup_causes() & BIT(ESP_SLEEP_WAKEUP_EXT1))) {
        return 0;
    }

    const uint64_t mask = esp_sleep_get_ext1_wakeup_status();

    /* Next takes priority when both wake bits are set. */
    if (mask & BIT64(wake_pins[1].pin)) {
        wake_button = CICALA_WAKE_NEXT;
    } else if (mask & BIT64(wake_pins[0].pin)) {
        wake_button = CICALA_WAKE_CATEGORY;
    } else {
        LOG_WRN("woken by EXT1 on an unexpected mask %llx", mask);
    }

    return 0;
}

SYS_INIT(latch_wake_button, PRE_KERNEL_1, 0);

static uint64_t open_pin_mask(void)
{
    uint64_t mask = 0;

    for (size_t i = 0; i < ARRAY_SIZE(wake_pins); i++) {
        const int closed = gpio_pin_get_dt(&wake_pins[i]);

        if (closed == 0) {
            mask |= BIT64(wake_pins[i].pin);
        } else if (closed < 0) {
            LOG_WRN("could not read wake pin %u: %d", wake_pins[i].pin, closed);
        }
    }

    return mask;
}

/* Keep the pads that have to be readable through sleep. */
static void hold_wake_pins(uint64_t mask)
{
    for (size_t i = 0; i < ARRAY_SIZE(wake_pins); i++) {
        const gpio_num_t pin = (gpio_num_t) wake_pins[i].pin;

        if ((mask & BIT64(pin)) == 0) {
            continue;
        }

        (void) rtc_gpio_pullup_en(pin);
        (void) rtc_gpio_pulldown_dis(pin);
        (void) rtc_gpio_hold_en(pin);
    }
}

/* Park the panel's control lines and hold them there. */
static void hold_panel_pins(void)
{
    for (size_t i = 0; i < ARRAY_SIZE(panel_pins); i++) {
        const gpio_num_t pin = (gpio_num_t) panel_pins[i].pin;

        if (IS_ENABLED(CONFIG_CICALA_REV_A)) {
            /* Physical zero also applies to active-low CS and RESET. */
            (void) gpio_pin_configure(panel_pins[i].port, panel_pins[i].pin, GPIO_OUTPUT_LOW);
        } else {
            (void) gpio_pin_configure_dt(&panel_pins[i], GPIO_OUTPUT_INACTIVE);
        }
        (void) rtc_gpio_hold_en(pin);
    }
}

static void sleep_now(struct k_work *work)
{
    ARG_UNUSED(work);

    if (!cicala_app_is_settled()) {
        (void) k_work_reschedule(&idle_work, K_MSEC(CONFIG_CICALA_SLEEP_IDLE_MS));
        return;
    }

    if (cicala_net_is_active()) {
        (void) k_work_reschedule(&idle_work, K_MSEC(CONFIG_CICALA_SLEEP_IDLE_MS));
        return;
    }

    if (cicala_power_external()) {
        (void) k_work_reschedule(&idle_work, K_MSEC(CONFIG_CICALA_SLEEP_IDLE_MS));
        return;
    }

    const uint64_t mask = open_pin_mask();

    if (mask == 0) {
        /* Arming an already-low input would wake immediately. */
        LOG_WRN("both buttons read closed; staying awake rather than sleeping forever");
        return;
    }

    const int err = esp_sleep_enable_ext1_wakeup(mask, ESP_EXT1_WAKEUP_ANY_LOW);

    if (err != 0) {
        LOG_ERR("could not arm EXT1 on mask %llx: %d", mask, err);
        return;
    }

#if IS_ENABLED(CONFIG_CICALA_POWER_WAKE_ON_USB)
    /* VBUS, on its own trigger and its own polarity. */
    const int vbus_err = esp_sleep_enable_ext0_wakeup((gpio_num_t) vbus_pin.pin, 1);

    if (vbus_err != 0) {
        LOG_ERR("could not arm EXT0 on GPIO%u: %d", vbus_pin.pin, vbus_err);
        return;
    }
#endif

    hold_wake_pins(mask);

    panel_controller_sleep();

    cicala_power_prepare_sleep();
    cicala_status_off();
    hold_panel_pins();

    /* ESP32-S3 soft-off preserves RTC slow memory. */

    if (IS_ENABLED(CONFIG_CICALA_POWER_WAKE_ON_USB)) {
        LOG_INF("sleeping, wake on any of GPIO mask %llx, or on VBUS", mask);
    } else {
        LOG_INF("sleeping, wake on any of GPIO mask %llx", mask);
    }

    log_panic();

    sys_poweroff();
}

static void on_activity(const struct zbus_channel *chan)
{
    ARG_UNUSED(chan);

    (void) k_work_reschedule(&idle_work, K_MSEC(CONFIG_CICALA_SLEEP_IDLE_MS));
}

ZBUS_LISTENER_DEFINE(cicala_sleep_obs, on_activity);
ZBUS_CHAN_ADD_OBS(chan_render, cicala_sleep_obs, 6);
ZBUS_CHAN_ADD_OBS(chan_next, cicala_sleep_obs, 6);
ZBUS_CHAN_ADD_OBS(chan_filters, cicala_sleep_obs, 6);

/* Let go of the pads held through the last sleep. */
static int sleep_release_holds(void)
{
    for (size_t i = 0; i < ARRAY_SIZE(wake_pins); i++) {
        (void) rtc_gpio_hold_dis((gpio_num_t) wake_pins[i].pin);
    }

    for (size_t i = 0; i < ARRAY_SIZE(panel_pins); i++) {
        (void) rtc_gpio_hold_dis((gpio_num_t) panel_pins[i].pin);
    }

    return 0;
}

SYS_INIT(sleep_release_holds, PRE_KERNEL_2, 0);

/* Start the idle timer even if nothing else ever happens. */
static int sleep_start(void)
{
    /* Log the configured panel sleep and wake policy once per boot. */
    if (IS_ENABLED(CONFIG_CICALA_PANEL_DEEP_SLEEP)) {
        LOG_INF("panel policy: controller sleeps, revived by the reset at init");
    } else {
        LOG_INF("panel policy: controller left awake through sleep");
    }

    (void) k_work_reschedule(&idle_work, K_MSEC(CONFIG_CICALA_SLEEP_IDLE_MS));

    return 0;
}

SYS_INIT(sleep_start, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
