/** zbus channels: the contract between the workqueue, `app`, and everything that renders. */

#pragma once

#include <stdbool.h>
#include <stdint.h>

#include <zephyr/zbus/zbus.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Decks, in the order Category advances through them. */
#define TK_DECK_COUNT 6

/** Event channel: one Category press happened. */
struct tk_category_msg {
    /** Uptime at the moment the press started. */
    int64_t timestamp_ms;
    /** Press-to-release duration in milliseconds; telemetry only. */
    uint32_t duration_ms;
};

/** Event channel: one Next press happened. */
struct tk_next_msg {
    /** Uptime at the moment the press started. */
    int64_t timestamp_ms;
    /** Press-to-release duration in milliseconds; telemetry only. */
    uint32_t duration_ms;
};

/** What a card on the panel is. */
enum tk_card {
    /** Something drawn from the corpus. */
    TK_CARD_QUESTION = 0,
    /** The name of the deck Category just moved to. */
    TK_CARD_CATEGORY,
    /** The setup portal, saying which network to join and how it went. */
    TK_CARD_SERVICE,
};

/** State channel. Text is copied so corpus replacement cannot invalidate it. */
struct tk_question_msg {
    /** Increments per draw, so a stale render can be told from a current one. */
    uint32_t seq;
    uint8_t deck;
    /** One of `enum tk_card`, stored as a byte. */
    uint8_t kind;
    uint16_t len;
    char text[CONFIG_TK_MAX_QUESTION_BYTES];
};

/** Event channel. App converts this to a sequenced `chan_question` card. */
struct tk_service_msg {
    uint16_t len;
    char text[CONFIG_TK_MAX_QUESTION_BYTES];
};

/** Event channel. The new corpus applies on the next draw without a refresh. */
struct tk_corpus_msg {
    /** Two-letter code, NUL-terminated. */
    char language[4];
};

/** Event channel: the display finished with the question it was given. */
struct tk_render_msg {
    /** The `seq` of the question this refers to. */
    uint32_t seq;
    /** 0, or a negative errno from the panel. */
    int result;
    /** True when this was a full refresh rather than a partial one. */
    bool was_full;
};

/** Where power stands. */
enum tk_power_state {
    /** No plausible reading. Refreshes remain allowed. */
    TK_POWER_UNKNOWN = 0,
    /** On the cell, above the refresh floor. */
    TK_POWER_NORMAL,
    /** On the cell, under CONFIG_TK_REFRESH_MIN_MV. Refreshes are refused. */
    TK_POWER_LOW,
    /** On the cell, nearly flat. Refreshes are refused. */
    TK_POWER_CRITICAL,
    /** External power is in. */
    TK_POWER_CHARGING,
    /** External power is in and the cell reads above the full estimate. */
    TK_POWER_CHARGED,
};

/** State channel for status consumers. Sleep queries power directly. */
struct tk_power_msg {
    /** At the pack, with the divider undone. */
    uint16_t mv;
    /** One of `enum tk_power_state`, stored as a byte. */
    uint8_t state;
    /** VBUS. */
    bool usb;
};

ZBUS_CHAN_DECLARE(chan_category);
ZBUS_CHAN_DECLARE(chan_next);
ZBUS_CHAN_DECLARE(chan_question);
ZBUS_CHAN_DECLARE(chan_service);
ZBUS_CHAN_DECLARE(chan_corpus);
ZBUS_CHAN_DECLARE(chan_render);
ZBUS_CHAN_DECLARE(chan_power);

/** Deck id for logs, or `?` when out of range. */
const char *tk_deck_name(uint8_t deck);

/** Card name for logs, or `?` when out of range. */
const char *tk_card_name(uint8_t kind);

/** Power-state name for logs, or `?` when out of range. */
const char *tk_power_name(uint8_t state);

/** English deck label shown on the panel for every corpus language. */
const char *tk_deck_label(uint8_t deck);

#ifdef __cplusplus
}
#endif
