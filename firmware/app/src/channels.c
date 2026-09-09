/* Channel definitions. */

#include "channels.h"

ZBUS_CHAN_DEFINE(chan_filters, struct cicala_filters_msg, NULL, NULL, ZBUS_OBSERVERS_EMPTY,
                 ZBUS_MSG_INIT(.timestamp_ms = 0, .duration_ms = 0));

ZBUS_CHAN_DEFINE(chan_next, struct cicala_next_msg, NULL, NULL, ZBUS_OBSERVERS_EMPTY,
                 ZBUS_MSG_INIT(.timestamp_ms = 0, .duration_ms = 0));

ZBUS_CHAN_DEFINE(chan_question, struct cicala_question_msg, NULL, NULL, ZBUS_OBSERVERS_EMPTY,
                 ZBUS_MSG_INIT(.seq = 0, .permissions = 0, .cursor = 0,
                               .kind = CICALA_CARD_QUESTION, .len = 0, .text = {0}));

ZBUS_CHAN_DEFINE(chan_service, struct cicala_service_msg, NULL, NULL, ZBUS_OBSERVERS_EMPTY,
                 ZBUS_MSG_INIT(.len = 0, .text = {0}));

ZBUS_CHAN_DEFINE(chan_corpus, struct cicala_corpus_msg, NULL, NULL, ZBUS_OBSERVERS_EMPTY,
                 ZBUS_MSG_INIT(.language = {0}));

ZBUS_CHAN_DEFINE(chan_render, struct cicala_render_msg, NULL, NULL, ZBUS_OBSERVERS_EMPTY,
                 ZBUS_MSG_INIT(.seq = 0, .result = 0, .was_full = false));

ZBUS_CHAN_DEFINE(chan_power, struct cicala_power_msg, NULL, NULL, ZBUS_OBSERVERS_EMPTY,
                 ZBUS_MSG_INIT(.mv = 0, .state = CICALA_POWER_UNKNOWN, .usb = false));

/* Labels used in display refresh logs. */
static const char *const card_names[] = {
    "question",
    "filters",
    "service card",
    "empty",
};

const char *cicala_card_name(uint8_t kind)
{
    if (kind >= ARRAY_SIZE(card_names)) {
        return "?";
    }

    return card_names[kind];
}

/* Same order as enum cicala_power_state, which is the same order as cicala::PowerState. */
static const char *const power_names[] = {
    "unknown",  "on the cell", "low",           "critical",
    "charging", "charged",     "external idle", "charger fault",
};

const char *cicala_power_name(uint8_t state)
{
    if (state >= ARRAY_SIZE(power_names)) {
        return "?";
    }

    return power_names[state];
}
