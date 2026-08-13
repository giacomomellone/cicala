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

/*
 * 3072 against a high-water mark of 1728 measured on the board, so 44 % spare.
 * The deepest path is the boot render, where this thread walks app_logic into
 * the question store and the layout before handing a card to `display`, and the
 * margin is there for a longer question or one more frame of nesting. This is
 * the thread that owns every press, so it is the wrong one to run close.
 */
#define APP_STACK_SIZE 3072
#define APP_PRIORITY 5

/* Of the four channels this thread subscribes to, chan_service carries the
 * most. See the note in display.c. */
BUILD_ASSERT(sizeof(struct tk_service_msg) <= CONFIG_ZBUS_MSG_SUBSCRIBER_NET_BUF_STATIC_DATA_SIZE,
             "chan_service no longer fits the static subscriber buffer; "
             "raise CONFIG_ZBUS_MSG_SUBSCRIBER_NET_BUF_STATIC_DATA_SIZE");

ZBUS_MSG_SUBSCRIBER_DEFINE(app_sub);
ZBUS_CHAN_ADD_OBS(chan_category, app_sub, 3);
ZBUS_CHAN_ADD_OBS(chan_next, app_sub, 3);
ZBUS_CHAN_ADD_OBS(chan_service, app_sub, 3);
ZBUS_CHAN_ADD_OBS(chan_corpus, app_sub, 3);
ZBUS_CHAN_ADD_OBS(chan_render, app_sub, 3);

static void app_thread(void *p1, void *p2, void *p3)
{
    ARG_UNUSED(p1);
    ARG_UNUSED(p2);
    ARG_UNUSED(p3);

    if (tk_app_init() != 0) {
        return;
    }

    /* Run once before waiting: tk_app_init() has already announced the active
     * deck, which is what lets BOOT leave on the first pass. */
    tk_app_run();

    while (true) {
        const struct zbus_channel *chan;
        union {
            struct tk_category_msg category;
            struct tk_next_msg next;
            struct tk_service_msg service;
            struct tk_corpus_msg corpus;
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

        if (chan == &chan_category) {
            /*
             * Not gated on the refresh, unlike a press. A Category press
             * changes which deck is active whether or not the panel is free,
             * and the machine names it as soon as it is. Dropping it would
             * leave the panel showing a deck the device is no longer on.
             */
            tk_app_post_category();
        } else if (chan == &chan_next) {
            if (tk_app_is_busy()) {
                LOG_INF("press ignored: the panel is still refreshing");
            }

            tk_app_post_next();
        } else if (chan == &chan_service) {
            /*
             * Not gated on the refresh either. The portal speaks a handful of
             * times in a session and somebody is standing there waiting to be
             * told which network to join; the state machine holds it until the
             * panel is free.
             */
            tk_app_post_service(msg.service.text, msg.service.len);
        } else if (chan == &chan_corpus) {
            /* No redraw: the new corpus applies on the next requested draw. */
            LOG_INF("corpus replaced: %s", msg.corpus.language);
            tk_app_reload_corpus();
        } else if (chan == &chan_render) {
            tk_app_post_render(msg.render.result == 0, msg.render.seq);
        }

        tk_app_run();
    }
}

K_THREAD_DEFINE(tk_app_thread, APP_STACK_SIZE, app_thread, NULL, NULL, NULL, APP_PRIORITY, 0, 0);
