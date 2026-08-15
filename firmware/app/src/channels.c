/* Channel definitions. */

#include "channels.h"

ZBUS_CHAN_DEFINE(chan_category, struct kveld_category_msg, NULL, NULL, ZBUS_OBSERVERS_EMPTY,
                 ZBUS_MSG_INIT(.timestamp_ms = 0, .duration_ms = 0));

ZBUS_CHAN_DEFINE(chan_next, struct kveld_next_msg, NULL, NULL, ZBUS_OBSERVERS_EMPTY,
                 ZBUS_MSG_INIT(.timestamp_ms = 0, .duration_ms = 0));

ZBUS_CHAN_DEFINE(chan_question, struct kveld_question_msg, NULL, NULL, ZBUS_OBSERVERS_EMPTY,
                 ZBUS_MSG_INIT(.seq = 0, .deck = 0, .kind = KVELD_CARD_QUESTION, .len = 0,
                               .text = {0}));

ZBUS_CHAN_DEFINE(chan_service, struct kveld_service_msg, NULL, NULL, ZBUS_OBSERVERS_EMPTY,
                 ZBUS_MSG_INIT(.len = 0, .text = {0}));

ZBUS_CHAN_DEFINE(chan_corpus, struct kveld_corpus_msg, NULL, NULL, ZBUS_OBSERVERS_EMPTY,
                 ZBUS_MSG_INIT(.language = {0}));

ZBUS_CHAN_DEFINE(chan_render, struct kveld_render_msg, NULL, NULL, ZBUS_OBSERVERS_EMPTY,
                 ZBUS_MSG_INIT(.seq = 0, .result = 0, .was_full = false));

ZBUS_CHAN_DEFINE(chan_power, struct kveld_power_msg, NULL, NULL, ZBUS_OBSERVERS_EMPTY,
                 ZBUS_MSG_INIT(.mv = 0, .state = KVELD_POWER_UNKNOWN, .usb = false));

/* Category order from questions/schema.json. */
static const char *const deck_names[KVELD_DECK_COUNT] = {
    "new_people", "close", "family", "work", "here", "wild",
};

/* What a deck is called on the panel. */
static const char *const deck_labels[KVELD_DECK_COUNT] = {
    "new people", "close", "family", "work", "here", "wild",
};

const char *kveld_deck_name(uint8_t deck)
{
    if (deck >= KVELD_DECK_COUNT) {
        return "?";
    }

    return deck_names[deck];
}

const char *kveld_deck_label(uint8_t deck)
{
    if (deck >= KVELD_DECK_COUNT) {
        return "?";
    }

    return deck_labels[deck];
}

/* Labels used in display refresh logs. */
static const char *const card_names[] = {
    "question",
    "deck name",
    "service card",
};

const char *kveld_card_name(uint8_t kind)
{
    if (kind >= ARRAY_SIZE(card_names)) {
        return "?";
    }

    return card_names[kind];
}

/* Same order as enum kveld_power_state, which is the same order as kveld::PowerState. */
static const char *const power_names[] = {
    "unknown", "on the cell", "low", "critical", "charging", "charged",
};

const char *kveld_power_name(uint8_t state)
{
    if (state >= ARRAY_SIZE(power_names)) {
        return "?";
    }

    return power_names[state];
}
