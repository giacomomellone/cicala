/*
 * The `display` thread.
 *
 * It exists for one reason: a panel refresh blocks for up to two seconds, and
 * `app` has to stay responsive during it — responsive enough to notice a Next
 * press and deliberately drop it. Doing the refresh on the app thread would
 * make that impossible.
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/zbus/zbus.h>

#include "channels.h"
#include "panel.h"

LOG_MODULE_REGISTER(tk_display, LOG_LEVEL_INF);

#define DISPLAY_STACK_SIZE 2560
#define DISPLAY_PRIORITY 6

ZBUS_MSG_SUBSCRIBER_DEFINE(display_sub);
ZBUS_CHAN_ADD_OBS(chan_question, display_sub, 3);

static void display_thread(void *p1, void *p2, void *p3)
{
    ARG_UNUSED(p1);
    ARG_UNUSED(p2);
    ARG_UNUSED(p3);

    if (tk_panel_init() != 0) {
        LOG_ERR("no panel; questions will be logged but not shown");
    }

    while (true) {
        const struct zbus_channel *chan;
        struct tk_question_msg question;

        if (zbus_sub_wait_msg(&display_sub, &chan, &question, K_FOREVER) != 0) {
            continue;
        }

        if (chan != &chan_question) {
            continue;
        }

        const bool was_full = tk_panel_next_is_full();
        const int64_t started = k_uptime_get();
        const int result = tk_panel_render(question.text, question.len);

        LOG_INF("%s refresh of %s seq %u took %lld ms (%d)", was_full ? "full" : "partial",
                question.is_category ? "deck name" : "question", question.seq,
                k_uptime_get() - started, result);

        const struct tk_render_msg done = {
            .seq = question.seq,
            .result = result,
            .was_full = was_full,
        };

        (void) zbus_chan_pub(&chan_render, &done, K_MSEC(100));
    }
}

K_THREAD_DEFINE(tk_display_thread, DISPLAY_STACK_SIZE, display_thread, NULL, NULL, NULL,
                DISPLAY_PRIORITY, 0, 0);
