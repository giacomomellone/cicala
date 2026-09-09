/* The `app` thread: the only consumer of input, and the only publisher of questions. */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/zbus/zbus.h>

#include "app_logic.h"
#include "channels.h"

LOG_MODULE_DECLARE(cicala_app, LOG_LEVEL_INF);

/* 3072 against a high-water mark of 1728 measured on the board, so 44 % spare. */
#define APP_STACK_SIZE 3072
#define APP_PRIORITY 5

/* Of the four channels this thread subscribes to, chan_service carries the most. */
BUILD_ASSERT(sizeof(struct cicala_service_msg) <=
                 CONFIG_ZBUS_MSG_SUBSCRIBER_NET_BUF_STATIC_DATA_SIZE,
             "chan_service no longer fits the static subscriber buffer; "
             "raise CONFIG_ZBUS_MSG_SUBSCRIBER_NET_BUF_STATIC_DATA_SIZE");

ZBUS_MSG_SUBSCRIBER_DEFINE(app_sub);
ZBUS_CHAN_ADD_OBS(chan_filters, app_sub, 3);
ZBUS_CHAN_ADD_OBS(chan_next, app_sub, 3);
ZBUS_CHAN_ADD_OBS(chan_service, app_sub, 3);
ZBUS_CHAN_ADD_OBS(chan_corpus, app_sub, 3);
ZBUS_CHAN_ADD_OBS(chan_render, app_sub, 3);

static void app_thread(void *p1, void *p2, void *p3)
{
    ARG_UNUSED(p1);
    ARG_UNUSED(p2);
    ARG_UNUSED(p3);

    if (cicala_app_init() != 0) {
        return;
    }

    /* Apply the retained state or the captured wake press. */
    cicala_app_run();

    while (true) {
        const struct zbus_channel *chan;
        union {
            struct cicala_filters_msg category;
            struct cicala_next_msg next;
            struct cicala_service_msg service;
            struct cicala_corpus_msg corpus;
            struct cicala_render_msg render;
        } msg;

        /* K_FOREVER lets Zephyr enter idle sleep. */
        const k_timeout_t wait = cicala_app_needs_timeout() ? K_MSEC(250) : K_FOREVER;
        const int err = zbus_sub_wait_msg(&app_sub, &chan, &msg, wait);

        if (err == -ENOMSG) {
            cicala_app_run();
            continue;
        }

        if (err != 0) {
            LOG_ERR("zbus wait failed: %d", err);
            continue;
        }

        if (chan == &chan_filters) {
            /* Tabletop presses during a refresh are dropped. */
            cicala_app_post_filters();
        } else if (chan == &chan_next) {
            if (cicala_app_is_busy()) {
                LOG_INF("press ignored: the panel is still refreshing");
            }

            cicala_app_post_next();
        } else if (chan == &chan_service) {
            /* Service cards remain available during a refresh. */
            cicala_app_post_service(msg.service.text, msg.service.len);
        } else if (chan == &chan_corpus) {
            /* No redraw: the new corpus applies on the next requested draw. */
            LOG_INF("corpus replaced: %s", msg.corpus.language);
            cicala_app_reload_corpus();
        } else if (chan == &chan_render) {
            cicala_app_post_render(msg.render.result == 0, msg.render.seq);
        }

        cicala_app_run();
    }
}

K_THREAD_DEFINE(cicala_app_thread, APP_STACK_SIZE, app_thread, NULL, NULL, NULL, APP_PRIORITY, 0,
                0);
