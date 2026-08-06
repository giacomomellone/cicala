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
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/poweroff.h>
#include <zephyr/sys/util.h>
#include <zephyr/zbus/zbus.h>

#include "app_logic.h"
#include "channels.h"
#include "net.h"

LOG_MODULE_REGISTER(tk_sleep, LOG_LEVEL_INF);

/*
 * Every input that should bring the device back. Same nodes input.c reads, so
 * the pins cannot drift apart; VBUS detect joins this list when it is wired,
 * with the opposite polarity, since plugging in drives it high.
 */
static const struct gpio_dt_spec wake_pins[] = {
    GPIO_DT_SPEC_GET(DT_ALIAS(tk_category), gpios),
    GPIO_DT_SPEC_GET(DT_ALIAS(tk_next), gpios),
};

static void sleep_now(struct k_work *work);
static K_WORK_DELAYABLE_DEFINE(idle_work, sleep_now);

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

    /*
     * RTC slow memory needs no arrangement here: the ESP32-S3 does not define
     * SOC_PM_SUPPORT_RTC_SLOW_MEM_PD, so the domain cannot be powered down and
     * the retained block survives by construction. What it does need is for
     * something to have sealed it, which is why sleeping is only allowed from
     * a settled state — a device that slept before its first render would seal
     * nothing, wake to an unstamped block, and report a cold boot forever.
     */

    LOG_INF("sleeping, wake on any of GPIO mask %llx", mask);

    /* The log is deferred, so give the backend a moment before the core stops
     * existing. Everything worth keeping is already in RTC memory. */
    k_sleep(K_MSEC(50));

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
 * them.
 */
static int sleep_release_holds(void)
{
    for (size_t i = 0; i < ARRAY_SIZE(wake_pins); i++) {
        (void) rtc_gpio_hold_dis((gpio_num_t) wake_pins[i].pin);
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
    (void) k_work_reschedule(&idle_work, K_MSEC(CONFIG_TK_SLEEP_IDLE_MS));

    return 0;
}

SYS_INIT(sleep_start, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
