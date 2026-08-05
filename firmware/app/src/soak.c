/*
 * The soak run: the firmware presses its own Next button.
 *
 * Compiled only when CONFIG_TK_DEBUG_SOAK is on, which is what `just fw-soak`
 * sets. It exists for one measurement — how many partial refreshes the panel
 * tolerates before ghosting shows, which is the number
 * CONFIG_TK_FULL_REFRESH_INTERVAL holds and currently guesses. Producing that
 * chain by hand means forty presses while watching the glass, so the device
 * does the pressing and the operator only watches.
 *
 * app/soak.conf sets CONFIG_TK_FULL_REFRESH_INTERVAL high enough that no full
 * refresh interrupts the run. The first render after boot is still a full one,
 * because tk_panel_init() seeds the counter with the interval, so the chain
 * starts from a panel this firmware drew rather than from whatever was on the
 * glass.
 *
 * Two console lines per render, and they are complementary: src/display.c
 * reports how long the refresh took, this one reports where it sits in the
 * chain. The first is what sets CONFIG_TK_REFRESH_TIMEOUT_MS, the second what
 * sets the full-refresh interval.
 *
 * Never enabled in a shipped build.
 */

#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/reboot.h>
#include <zephyr/zbus/zbus.h>

#include "channels.h"
#include "panel.h"

LOG_MODULE_REGISTER(tk_soak, LOG_LEVEL_INF);

static void press(struct k_work *work);
static K_WORK_DELAYABLE_DEFINE(press_work, press);

/** Cleared by every press, set by the render that should follow it. */
static bool rendered;

/** Presses since this boot. Deliberately not retained: it counts a run. */
static uint32_t presses;

/**
 * Ask for another question.
 *
 * Log lines here and in on_render() stay ASCII: twister reads the console byte
 * by byte and aborts the run on anything it cannot decode, so an em dash in a
 * format string fails the suite with "unexpected byte" and no other clue.
 */
static void press(struct k_work *work)
{
    ARG_UNUSED(work);

    if (!rendered && presses > 0) {
        /* The fallback below fired: nothing rendered after the last press. */
        LOG_WRN("no refresh followed the last press; pressing again");
    }

    rendered = false;
    presses++;

#ifdef CONFIG_TK_DEBUG_SOAK_REBOOT
    if (presses > CONFIG_TK_DEBUG_SOAK_REBOOT_EVERY) {
        /*
         * Deep sleep is a reboot, so this is the closest thing to a wake that
         * exists before CONFIG_PM does: the state that is supposed to outlive
         * one either comes back or it does not, and the boot log says which.
         *
         * Warm rather than cold. A cold reset takes the RTC domain down with
         * it, which is the one thing this must not do.
         */
        LOG_INF("rebooting after %u presses to check what survives", presses - 1);
        k_sleep(K_MSEC(50)); // let the log drain
        sys_reboot(SYS_REBOOT_WARM);
    }
#endif

    /*
     * Re-arm before publishing, at twice the interval, as a fallback. The
     * chain is otherwise driven by renders, and a press the state machine
     * drops produces no render — so without this one dropped press would end
     * the run silently, hours before anyone looked. The render that normally
     * follows reschedules this to the ordinary interval, so the fallback only
     * ever fires when nothing happened.
     */
    (void) k_work_reschedule(&press_work, K_MSEC(2 * CONFIG_TK_DEBUG_SOAK_INTERVAL_MS));

    const struct tk_next_msg msg = {
        .timestamp_ms = k_uptime_get(),
        /* Telemetry only; no policy reads it. See channels.h. */
        .duration_ms = 0,
    };

    (void) zbus_chan_pub(&chan_next, &msg, K_MSEC(10));
}

/**
 * Ask for the next question, once the last one is on the glass.
 *
 * Driving off render completion rather than off a free-running timer is what
 * keeps the count honest: app_fsm drops a press made during REFRESHING
 * (docs/firmware_architecture.md), so a fixed-period timer would lose ticks
 * whenever a refresh ran long and undercount the chain.
 */
static void on_render(const struct zbus_channel *chan)
{
    const struct tk_render_msg *msg = zbus_chan_const_msg(chan);

    if (msg->result != 0) {
        LOG_WRN("seq %u failed to render: %d", msg->seq, msg->result);
    } else if (msg->was_full) {
        LOG_INF("full refresh of seq %u; the chain starts here", msg->seq);
    } else {
        /* Read after the render, so it is that render's own position in the
         * chain: the number to write down when the glass stops being clean. */
        LOG_INF("partial #%u of seq %u", tk_panel_partial_count(), msg->seq);
    }

    rendered = true;

    (void) k_work_reschedule(&press_work, K_MSEC(CONFIG_TK_DEBUG_SOAK_INTERVAL_MS));
}

ZBUS_LISTENER_DEFINE(tk_soak_obs, on_render);

/* 3 is the app thread's subscription and 4 is taken in the test suites. */
ZBUS_CHAN_ADD_OBS(chan_render, tk_soak_obs, 5);

/**
 * Arm the first press without waiting for a render.
 *
 * The chain is driven by renders, and a boot does not always produce one: with
 * retained state a wake to a question the panel already holds draws nothing at
 * all, which is the whole point of it. Waiting for a render there would leave
 * the run stopped before it started.
 */
static int soak_start(void)
{
    (void) k_work_reschedule(&press_work, K_MSEC(CONFIG_TK_DEBUG_SOAK_INTERVAL_MS));

    return 0;
}

SYS_INIT(soak_start, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
