/*
 * The `app` thread: the only consumer of input, and the only publisher of
 * questions.
 *
 * C rather than C++ for the one reason the architecture gives — the zbus
 * observer macros expand to out-of-order designated initializers, which C++17
 * rejects. The decisions themselves live in app_logic.cpp behind app_logic.h.
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/zbus/zbus.h>

#include "app_logic.h"
#include "channels.h"

LOG_MODULE_DECLARE(tk_app, LOG_LEVEL_INF);

#define APP_STACK_SIZE 2048
#define APP_PRIORITY 5

ZBUS_MSG_SUBSCRIBER_DEFINE(app_sub);
ZBUS_CHAN_ADD_OBS(chan_selector, app_sub, 3);
ZBUS_CHAN_ADD_OBS(chan_next, app_sub, 3);
ZBUS_CHAN_ADD_OBS(chan_render, app_sub, 3);

static void app_thread(void *p1, void *p2, void *p3)
{
    ARG_UNUSED(p1);
    ARG_UNUSED(p2);
    ARG_UNUSED(p3);

    if (tk_app_init() != 0) {
        return;
    }

    /* Run once before waiting: the selector was read at boot, and its value is
     * already sitting in chan_selector as state rather than as an event. */
    tk_app_run();

    while (true) {
        const struct zbus_channel *chan;
        union {
            struct tk_selector_msg selector;
            struct tk_next_msg next;
            struct tk_render_msg render;
        } msg;

        /*
         * Blocking with no timeout is the point. Only REFRESHING has one, and
         * only because a dead panel must not wedge the device; every other
         * state waits indefinitely, which is what will let the idle thread
         * pick a sleep state once CONFIG_PM is on.
         */
        const k_timeout_t wait = tk_app_needs_timeout() ? K_MSEC(250) : K_FOREVER;
        const int err = zbus_sub_wait_msg(&app_sub, &chan, &msg, wait);

        if (err == -ENOMSG) {
            /*
             * The wait expired with nothing queued. Not an error — it is how
             * the poll during REFRESHING is supposed to end, and ticking here
             * is what lets that state's timeout fire.
             *
             * -ENOMSG rather than the -EAGAIN one might expect: zbus reports a
             * timed-out k_fifo_get this way, so testing for the wrong one logs
             * an error four times a second for the length of every refresh.
             */
            tk_app_run();
            continue;
        }

        if (err != 0) {
            LOG_ERR("zbus wait failed: %d", err);
            continue;
        }

        if (chan == &chan_selector) {
            tk_app_post_selector(msg.selector.deck, msg.selector.valid);
        } else if (chan == &chan_next) {
            tk_app_post_next();
        } else if (chan == &chan_render) {
            tk_app_post_render(msg.render.result == 0, msg.render.seq);
        }

        tk_app_run();
    }
}

K_THREAD_DEFINE(tk_app_thread, APP_STACK_SIZE, app_thread, NULL, NULL, NULL, APP_PRIORITY, 0, 0);
