/*
 * Deep sleep, and the wake mask that makes it possible.
 *
 * Compiled only when CONFIG_TK_SLEEP is on, which is what `just fw-sleep`
 * sets. The default image stays awake: a board that reboots on every press is
 * harder to bring up than one that does not.
 *
 * ## Sleep is called, not fallen into
 *
 * The architecture originally had the idle thread choose deep sleep once
 * nothing was runnable. That is not how this SoC works — Zephyr's own
 * devicetree marks the state `status = "disabled"` and says it "must be
 * entered using pm_state_force() or sys_poweroff() calls only". So something
 * has to decide, and that something is the idle timer below: rearmed by every
 * question and every render, it fires when the table has gone quiet.
 *
 * ## The wake mask is read, not assumed
 *
 * Both buttons are open at rest, so both are normally armed for ANY_LOW. The
 * mask is still built from a live reading rather than hardcoded: a button held
 * down at the moment of sleep is already low, and arming it would satisfy the
 * wake condition before sleep was entered. Leaving it out means the device
 * sleeps and the *other* button still wakes it.
 */

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
#include "sleep.h"

LOG_MODULE_REGISTER(tk_sleep, LOG_LEVEL_INF);

/*
 * Kconfig only warns when a .conf assigns a symbol that does not exist, so
 * without this an unpatched workspace builds a sleeping image whose panel goes
 * white on every wake — and the warning scrolls past in the middle of a build.
 * See firmware/patches.yml.
 */
#ifndef CONFIG_SSD16XX_PRESERVE_IMAGE_ON_INIT
#error "sleeping image needs the ssd16xx patch — run `just fw-patch`"
#endif

/*
 * Every input that should bring the device back. Same nodes input.c reads, so
 * the pins cannot drift apart; VBUS detect joins this list when it is wired,
 * with the opposite polarity, since plugging in drives it high.
 */
static const struct gpio_dt_spec wake_pins[] = {
    GPIO_DT_SPEC_GET(DT_ALIAS(tk_category), gpios),
    GPIO_DT_SPEC_GET(DT_ALIAS(tk_next), gpios),
};

/*
 * The panel's control lines.
 *
 * Deep sleep isolates every GPIO, which leaves these floating for as long as
 * the device is asleep. RESET is active low and the panel keeps its own power,
 * so a floating line that drifts below the input threshold resets the
 * controller — and with it the RAM that CONFIG_SSD16XX_PRESERVE_IMAGE_ON_INIT
 * exists to keep. CS floating is the same argument one step removed: selected
 * by accident, noise on the clock is a command.
 *
 * All three are RTC-capable (GPIO 8, 10 and 18), which is what makes holding
 * them possible at all.
 */
static const struct gpio_dt_spec panel_pins[] = {
    GPIO_DT_SPEC_GET(DT_NODELABEL(tk_mipi_dbi), reset_gpios),
    GPIO_DT_SPEC_GET(DT_NODELABEL(tk_mipi_dbi), dc_gpios),
    GPIO_DT_SPEC_GET_BY_IDX(DT_NODELABEL(spi2), cs_gpios, 0),
};

#if IS_ENABLED(CONFIG_TK_PANEL_DEEP_SLEEP)

#define TK_PANEL_NODE DT_CHOSEN(zephyr_display)

/*
 * From ssd16xx_regs.h, which is private to the driver and cannot be included
 * from here. Two constants rather than a fourth patch to export them.
 *
 * Mode 1 rather than mode 2: mode 2 drops the RAM, which is the image on the
 * glass and the whole reason the panel is worth keeping powered at all.
 */
#define TK_SSD16XX_CMD_SLEEP_MODE 0x10
#define TK_SSD16XX_SLEEP_MODE_DSM1 0x01

static const struct device *const panel_bus = DEVICE_DT_GET(DT_PARENT(TK_PANEL_NODE));

/*
 * The same bus configuration ssd16xx builds for itself, from the same node.
 * It has to match: a different word size or CS policy addresses a controller
 * that is not the one the driver has been talking to.
 */
static const struct mipi_dbi_config panel_dbi = {
    .mode = MIPI_DBI_MODE_SPI_4WIRE,
    .config = MIPI_DBI_SPI_CONFIG_DT(
        TK_PANEL_NODE, SPI_OP_MODE_MASTER | SPI_WORD_SET(8) | SPI_HOLD_ON_CS | SPI_LOCK_ON, 0),
};

static const struct gpio_dt_spec panel_busy = GPIO_DT_SPEC_GET(TK_PANEL_NODE, busy_gpios);

/**
 * Send the controller into deep sleep mode 1.
 *
 * Only ever reached from a settled state, so the panel should be idle already;
 * the BUSY poll is there because a command issued mid-update is ignored, and
 * being ignored here looks exactly like working until the current is measured.
 *
 * Waking it again is the hardware reset ssd16xx performs at init, which
 * CONFIG_SSD16XX_PRESERVE_IMAGE_HW_RESET exists to put back. Without that
 * symbol the controller would sleep and never be spoken to again.
 */
static void panel_controller_sleep(void)
{
    const uint8_t mode = TK_SSD16XX_SLEEP_MODE_DSM1;

    for (int waited = 0; gpio_pin_get_dt(&panel_busy) > 0; waited++) {
        if (waited >= CONFIG_TK_REFRESH_TIMEOUT_MS) {
            LOG_ERR("panel still busy; leaving the controller awake");
            return;
        }

        k_msleep(1);
    }

    const int err =
        mipi_dbi_command_write(panel_bus, &panel_dbi, TK_SSD16XX_CMD_SLEEP_MODE, &mode, 1);

    (void) mipi_dbi_release(panel_bus, &panel_dbi);

    if (err != 0) {
        LOG_ERR("could not sleep the panel controller: %d", err);
        return;
    }

    LOG_INF("panel controller in deep sleep mode 1");
}

#else

static void panel_controller_sleep(void) {}

#endif /* CONFIG_TK_PANEL_DEEP_SLEEP */

static void sleep_now(struct k_work *work);
static K_WORK_DELAYABLE_DEFINE(idle_work, sleep_now);

/** Latched at PRE_KERNEL_1, read by the app thread. See sleep.h. */
static enum tk_wake_source wake_button = TK_WAKE_NONE;

enum tk_wake_source tk_wake_button(void)
{
    return wake_button;
}

/**
 * Read the wake mask before anything else can disturb it.
 *
 * PRE_KERNEL_1 so it runs ahead of sleep_release_holds() below: the hold is a
 * property of the same RTC domain the wake status lives in, and reading first
 * costs nothing while assuming they are independent would have to be checked
 * against the silicon.
 *
 * A boot that was not a wake leaves this at TK_WAKE_NONE, which is what a
 * power-on and the reset button both are.
 */
static int latch_wake_button(void)
{
    if (esp_sleep_get_wakeup_cause() != ESP_SLEEP_WAKEUP_EXT1) {
        return 0;
    }

    const uint64_t mask = esp_sleep_get_ext1_wakeup_status();

    /* Next first: with both bits set it is the one that answers. */
    if (mask & BIT64(wake_pins[1].pin)) {
        wake_button = TK_WAKE_NEXT;
    } else if (mask & BIT64(wake_pins[0].pin)) {
        wake_button = TK_WAKE_CATEGORY;
    } else {
        /* EXT1 fired with no pin of ours in the mask. Nothing else is armed,
         * so this should be unreachable; say so rather than silently treating
         * it as a cold boot. */
        LOG_WRN("woken by EXT1 on an unexpected mask %llx", mask);
    }

    return 0;
}

SYS_INIT(latch_wake_button, PRE_KERNEL_1, 0);

/**
 * Which pins can be armed for ANY_LOW right now.
 *
 * `gpio_pin_get_dt` reports the logical level, so an active-low contact reads
 * 1 when it is closed. Those are the ones left out.
 */
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

/**
 * Keep the pads that have to be readable through sleep.
 *
 * The GPIO peripheral's internal pull-ups die with the digital domain, and a
 * wake input armed for ANY_LOW that is left floating pulls itself low: the
 * device wakes the instant it sleeps, over and over. The RTC pad has its own
 * pull-up, and holding the configuration is what carries it past the moment
 * the rest of the chip stops.
 */
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

/**
 * Park the panel's control lines and hold them there.
 *
 * Driven to their inactive level first — RESET and CS released, D/C at command
 * — because the hold latches whatever the pad is doing at the moment it is
 * applied, not some safe default.
 */
static void hold_panel_pins(void)
{
    for (size_t i = 0; i < ARRAY_SIZE(panel_pins); i++) {
        const gpio_num_t pin = (gpio_num_t) panel_pins[i].pin;

        (void) gpio_pin_configure_dt(&panel_pins[i], GPIO_OUTPUT_INACTIVE);
        (void) rtc_gpio_hold_en(pin);
    }
}

static void sleep_now(struct k_work *work)
{
    ARG_UNUSED(work);

    if (!tk_app_is_settled()) {
        /*
         * Mid-decision: a refresh in flight, or a deck named but not yet
         * drawn from. A wake is a fresh boot, so sleeping here does not pause
         * the work, it discards it.
         */
        (void) k_work_reschedule(&idle_work, K_MSEC(CONFIG_TK_SLEEP_IDLE_MS));
        return;
    }

    if (tk_net_is_active()) {
        /*
         * The setup portal is on air. sys_poweroff() would take the access
         * point down mid-session and, since a wake is a fresh boot, lose it —
         * the phone would be looking at a network that no longer exists.
         * docs/firmware_architecture.md calls this the PM lock.
         */
        (void) k_work_reschedule(&idle_work, K_MSEC(CONFIG_TK_SLEEP_IDLE_MS));
        return;
    }

    const uint64_t mask = open_pin_mask();

    if (mask == 0) {
        /*
         * Both buttons read closed — held down, or stuck. Sleeping would be
         * permanent, so stay awake and let the panel keep showing what it has.
         */
        LOG_ERR("both buttons read closed; staying awake rather than sleeping forever");
        return;
    }

    const int err = esp_sleep_enable_ext1_wakeup(mask, ESP_EXT1_WAKEUP_ANY_LOW);

    if (err != 0) {
        LOG_ERR("could not arm EXT1 on mask %llx: %d", mask, err);
        return;
    }

    hold_wake_pins(mask);

    /* Before the pins are parked: this is the last thing that uses the bus,
     * and holding CS would take it away mid-command. */
    panel_controller_sleep();

    hold_panel_pins();

    /*
     * RTC slow memory needs no arrangement here: the ESP32-S3 does not define
     * SOC_PM_SUPPORT_RTC_SLOW_MEM_PD, so the domain cannot be powered down and
     * the retained block survives by construction. What it does need is for
     * something to have sealed it, which is why sleeping is only allowed from
     * a settled state — a device that slept before its first render would seal
     * nothing, wake to an unstamped block, and report a cold boot forever.
     */

    LOG_INF("sleeping, wake on any of GPIO mask %llx", mask);

    /*
     * Flush, rather than wait and hope.
     *
     * Logging is deferred, and the processing thread is woken either by
     * CONFIG_LOG_PROCESS_TRIGGER_THRESHOLD messages piling up — ten — or by
     * its own CONFIG_LOG_PROCESS_THREAD_SLEEP_MS tick, which is a second. The
     * sleep path emits two messages. Two never reaches ten, so nothing wakes
     * the thread, and the sleep this used to do was fifty milliseconds against
     * a thousand: the core stopped existing with both lines still in the
     * buffer, and the console simply ended after the last refresh.
     *
     * log_panic() puts the backends into synchronous mode and drains what is
     * queued, which is what a shutdown path wants — there is no later.
     *
     * Everything worth keeping is already in RTC memory; this is only so the
     * bench can see where the device went.
     */
    log_panic();

    sys_poweroff();
}

/** Any activity puts the idle timer back to the start. */
static void on_activity(const struct zbus_channel *chan)
{
    ARG_UNUSED(chan);

    (void) k_work_reschedule(&idle_work, K_MSEC(CONFIG_TK_SLEEP_IDLE_MS));
}

ZBUS_LISTENER_DEFINE(tk_sleep_obs, on_activity);
ZBUS_CHAN_ADD_OBS(chan_render, tk_sleep_obs, 6);
ZBUS_CHAN_ADD_OBS(chan_next, tk_sleep_obs, 6);
ZBUS_CHAN_ADD_OBS(chan_category, tk_sleep_obs, 6);

/**
 * Let go of the pads held through the last sleep.
 *
 * `rtc_gpio_hold_en()` latches a pad's configuration and keeps it latched
 * across the wake — and across an ordinary reset, since the latch lives in the
 * RTC domain rather than in the peripheral. Left in place it outlives the sleep
 * it was for: the gpio-keys driver reconfigures pins that no longer listen, and
 * neither button reaches the application again.
 *
 * Runs before device init, so the pads are free by the time anything claims
 * them — which for the panel's lines is the point twice over: still held, they
 * would keep RESET released and CS deselected while the SPI driver believed it
 * was driving them, and the first refresh would go nowhere.
 */
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

/**
 * Start the idle timer even if nothing else ever happens.
 *
 * A wake to a question the panel already holds draws nothing, so no render
 * follows it and no channel carries anything. Without this the device would
 * stay awake precisely in the case that is supposed to be cheapest.
 */
static int sleep_start(void)
{
    /*
     * Which panel policy this image was built with, said once per boot — which
     * on a sleeping image is once per press, and is the line that tells a
     * failed revival apart from a working one. A controller that never came
     * back out of its deep sleep still logs a normal refresh: SPI writes carry
     * no acknowledgement, so the driver cannot know the glass did not move.
     * Without this the console reads identically in both cases.
     */
    if (IS_ENABLED(CONFIG_TK_PANEL_DEEP_SLEEP)) {
        LOG_INF("panel policy: controller sleeps, revived by the reset at init");
    } else {
        LOG_INF("panel policy: controller left awake through sleep");
    }

    (void) k_work_reschedule(&idle_work, K_MSEC(CONFIG_TK_SLEEP_IDLE_MS));

    return 0;
}

SYS_INIT(sleep_start, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
