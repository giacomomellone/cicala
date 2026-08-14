/* The soak run: the firmware presses its own Next button. */

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

/* Cleared by every press, set by the render that should follow it. */
static bool rendered;

/* Presses since this boot. */
static uint32_t presses;

/* Ask for another question. */
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
        /* A warm reboot checks the RTC-retained state path. */
        LOG_INF("rebooting after %u presses to check what survives", presses - 1);
        k_sleep(K_MSEC(50)); // Let deferred logging drain.
        sys_reboot(SYS_REBOOT_WARM);
    }
#endif

    /* Re-arm before publishing, at twice the interval, as a fallback. */
    (void) k_work_reschedule(&press_work, K_MSEC(2 * CONFIG_TK_DEBUG_SOAK_INTERVAL_MS));

    const struct tk_next_msg msg = {
        .timestamp_ms = k_uptime_get(),
        /* Telemetry only; no policy reads it. */
        .duration_ms = 0,
    };

    (void) zbus_chan_pub(&chan_next, &msg, K_MSEC(10));
}

/* Ask for the next question, once the last one is on the glass. */
static void on_render(const struct zbus_channel *chan)
{
    const struct tk_render_msg *msg = zbus_chan_const_msg(chan);

    if (msg->result != 0) {
        LOG_WRN("seq %u failed to render: %d", msg->seq, msg->result);
    } else if (msg->was_full) {
        LOG_INF("full refresh of seq %u; the chain starts here", msg->seq);
    } else {
        /* Read the refresh count after the completed render. */
        LOG_INF("partial #%u of seq %u", tk_panel_partial_count(), msg->seq);
    }

    rendered = true;

    (void) k_work_reschedule(&press_work, K_MSEC(CONFIG_TK_DEBUG_SOAK_INTERVAL_MS));
}

ZBUS_LISTENER_DEFINE(tk_soak_obs, on_render);

/* 3 is the app thread's subscription and 4 is taken in the test suites. */
ZBUS_CHAN_ADD_OBS(chan_render, tk_soak_obs, 5);

static int soak_start(void)
{
    (void) k_work_reschedule(&press_work, K_MSEC(CONFIG_TK_DEBUG_SOAK_INTERVAL_MS));

    return 0;
}

SYS_INIT(soak_start, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
