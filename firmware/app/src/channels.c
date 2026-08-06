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

ZBUS_CHAN_DEFINE(chan_render, struct tk_render_msg, NULL, NULL, ZBUS_OBSERVERS_EMPTY,
                 ZBUS_MSG_INIT(.seq = 0, .result = 0, .was_full = false));

/*
 * chan_question is the biggest message on any channel, and the static
 * subscriber buffer is sized against it in prj.conf. Adding a field wide enough
 * to bring padding with it overflows that buffer, and a publish then fails at
 * runtime on a board rather than here. `kind` is a byte for this reason.
 */
BUILD_ASSERT(sizeof(struct tk_question_msg) <= CONFIG_ZBUS_MSG_SUBSCRIBER_NET_BUF_STATIC_DATA_SIZE,
             "chan_question no longer fits the static subscriber buffer; "
             "raise CONFIG_ZBUS_MSG_SUBSCRIBER_NET_BUF_STATIC_DATA_SIZE in prj.conf");

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
