/* Channel definitions. */

#include "channels.h"

ZBUS_CHAN_DEFINE(chan_category, struct cicala_category_msg, NULL, NULL, ZBUS_OBSERVERS_EMPTY,
                 ZBUS_MSG_INIT(.timestamp_ms = 0, .duration_ms = 0));

ZBUS_CHAN_DEFINE(chan_next, struct cicala_next_msg, NULL, NULL, ZBUS_OBSERVERS_EMPTY,
                 ZBUS_MSG_INIT(.timestamp_ms = 0, .duration_ms = 0));

ZBUS_CHAN_DEFINE(chan_question, struct cicala_question_msg, NULL, NULL, ZBUS_OBSERVERS_EMPTY,
                 ZBUS_MSG_INIT(.seq = 0, .deck = 0, .kind = CICALA_CARD_QUESTION, .len = 0,
                               .text = {0}));

ZBUS_CHAN_DEFINE(chan_service, struct cicala_service_msg, NULL, NULL, ZBUS_OBSERVERS_EMPTY,
                 ZBUS_MSG_INIT(.len = 0, .text = {0}));

ZBUS_CHAN_DEFINE(chan_corpus, struct cicala_corpus_msg, NULL, NULL, ZBUS_OBSERVERS_EMPTY,
                 ZBUS_MSG_INIT(.language = {0}));

ZBUS_CHAN_DEFINE(chan_render, struct cicala_render_msg, NULL, NULL, ZBUS_OBSERVERS_EMPTY,
                 ZBUS_MSG_INIT(.seq = 0, .result = 0, .was_full = false));

ZBUS_CHAN_DEFINE(chan_power, struct cicala_power_msg, NULL, NULL, ZBUS_OBSERVERS_EMPTY,
                 ZBUS_MSG_INIT(.mv = 0, .state = CICALA_POWER_UNKNOWN, .usb = false));

/* Category order from questions/schema.json. */
static const char *const deck_names[CICALA_DECK_COUNT] = {
    "new_people", "close", "family", "work", "here", "wild",
};

/* What a deck is called on the panel. */
static const char *const deck_labels[CICALA_DECK_COUNT] = {
    "new people", "close", "family", "work", "here", "wild",
};

const char *cicala_deck_name(uint8_t deck)
{
    if (deck >= CICALA_DECK_COUNT) {
        return "?";
    }

    return deck_names[deck];
}

const char *cicala_deck_label(uint8_t deck)
{
    if (deck >= CICALA_DECK_COUNT) {
        return "?";
    }

    return deck_labels[deck];
}

/* Category order on the device: schema order minus work. The mask bits keep
   their schema positions; only the offered cycle shrinks. */
static const uint8_t deck_cycle[CICALA_DECK_CYCLE_COUNT] = {0, 1, 2, 4, 5};

bool cicala_deck_on_device(uint8_t deck)
{
    for (uint8_t i = 0; i < CICALA_DECK_CYCLE_COUNT; i++) {
        if (deck_cycle[i] == deck) {
            return true;
        }
    }

    return false;
}

uint8_t cicala_deck_cycle_next(uint8_t deck)
{
    for (uint8_t i = 0; i < CICALA_DECK_CYCLE_COUNT; i++) {
        if (deck_cycle[i] == deck) {
            return deck_cycle[(i + 1) % CICALA_DECK_CYCLE_COUNT];
        }
    }

    return deck_cycle[0];
}

/* Labels used in display refresh logs. */
static const char *const card_names[] = {
    "question",
    "deck name",
    "service card",
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
