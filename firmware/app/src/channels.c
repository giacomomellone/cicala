/*
 * Channel definitions. See include/channels.h for what they carry and
 * docs/firmware_architecture.md for the full table.
 */

#include "channels.h"

ZBUS_CHAN_DEFINE(chan_category, struct tk_category_msg, NULL, NULL, ZBUS_OBSERVERS_EMPTY,
                 ZBUS_MSG_INIT(.timestamp_ms = 0, .duration_ms = 0));

ZBUS_CHAN_DEFINE(chan_next, struct tk_next_msg, NULL, NULL, ZBUS_OBSERVERS_EMPTY,
                 ZBUS_MSG_INIT(.timestamp_ms = 0, .duration_ms = 0));

ZBUS_CHAN_DEFINE(chan_question, struct tk_question_msg, NULL, NULL, ZBUS_OBSERVERS_EMPTY,
                 ZBUS_MSG_INIT(.seq = 0, .deck = 0, .kind = TK_CARD_QUESTION, .len = 0,
                               .text = {0}));

ZBUS_CHAN_DEFINE(chan_service, struct tk_service_msg, NULL, NULL, ZBUS_OBSERVERS_EMPTY,
                 ZBUS_MSG_INIT(.len = 0, .text = {0}));

ZBUS_CHAN_DEFINE(chan_corpus, struct tk_corpus_msg, NULL, NULL, ZBUS_OBSERVERS_EMPTY,
                 ZBUS_MSG_INIT(.language = {0}));

ZBUS_CHAN_DEFINE(chan_render, struct tk_render_msg, NULL, NULL, ZBUS_OBSERVERS_EMPTY,
                 ZBUS_MSG_INIT(.seq = 0, .result = 0, .was_full = false));

ZBUS_CHAN_DEFINE(chan_power, struct tk_power_msg, NULL, NULL, ZBUS_OBSERVERS_EMPTY,
                 ZBUS_MSG_INIT(.mv = 0, .state = TK_POWER_UNKNOWN, .usb = false));

/*
 * Index order is the order Category advances through, and matches
 * questions/schema.json x-tischkarte.decks. This table is the one place a log
 * line gets its name from.
 */
static const char *const deck_names[TK_DECK_COUNT] = {
    "new_people", "close", "family", "work", "here", "wild",
};

/*
 * What a deck is called on the panel. English regardless of the corpus
 * language, which is not an oversight: website/src/i18n.ts keeps the English
 * deck labels in its German translation too, deliberately, for the first
 * physical prototype. These are the same strings.
 */
static const char *const deck_labels[TK_DECK_COUNT] = {
    "new people", "close", "family", "work", "here", "wild",
};

const char *tk_deck_name(uint8_t deck)
{
    if (deck >= TK_DECK_COUNT) {
        return "?";
    }

    return deck_names[deck];
}

const char *tk_deck_label(uint8_t deck)
{
    if (deck >= TK_DECK_COUNT) {
        return "?";
    }

    return deck_labels[deck];
}

/* Reads as the sentence the display log puts it in: "partial refresh of a
 * deck name seq 4 took 622 ms". */
static const char *const card_names[] = {
    "question",
    "deck name",
    "service card",
};

const char *tk_card_name(uint8_t kind)
{
    if (kind >= ARRAY_SIZE(card_names)) {
        return "?";
    }

    return card_names[kind];
}

/* Same order as enum tk_power_state, which is the same order as
 * tk::PowerState. Reads as the sentence the power log puts it in:
 * "3812 mV, charging". */
static const char *const power_names[] = {
    "unknown", "on the cell", "low", "critical", "charging", "charged",
};

const char *tk_power_name(uint8_t state)
{
    if (state >= ARRAY_SIZE(power_names)) {
        return "?";
    }

    return power_names[state];
}
