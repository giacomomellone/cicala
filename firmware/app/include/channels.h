/** zbus channels: the contract between the workqueue, `app`, and everything that renders. */

#pragma once

#include <stdbool.h>
#include <stdint.h>

#include <zephyr/zbus/zbus.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Decks in the bundle mask, in schema order. */
#define CICALA_DECK_COUNT 6

/** The device cycles five of the six decks; Work stays on the website. */
#define CICALA_DECK_CYCLE_COUNT 5

/** Event channel: one Category press happened. */
struct cicala_category_msg {
    /** Uptime at the moment the press started. */
    int64_t timestamp_ms;
    /** Press-to-release duration in milliseconds; telemetry only. */
    uint32_t duration_ms;
};

/** Event channel: one Next press happened. */
struct cicala_next_msg {
    /** Uptime at the moment the press started. */
    int64_t timestamp_ms;
    /** Press-to-release duration in milliseconds; telemetry only. */
    uint32_t duration_ms;
};

/** What a card on the panel is. */
enum cicala_card {
    /** Something drawn from the corpus. */
    CICALA_CARD_QUESTION = 0,
    /** The name of the deck Category just moved to. */
    CICALA_CARD_CATEGORY,
    /** The setup portal, saying which network to join and how it went. */
    CICALA_CARD_SERVICE,
};

/** State channel. Text is copied so corpus replacement cannot invalidate it. */
struct cicala_question_msg {
    /** Increments per draw, so a stale render can be told from a current one. */
    uint32_t seq;
    uint8_t deck;
    /** One of `enum cicala_card`, stored as a byte. */
    uint8_t kind;
    uint16_t len;
    char text[CONFIG_CICALA_MAX_QUESTION_BYTES];
};

/** Event channel. App converts this to a sequenced `chan_question` card. */
struct cicala_service_msg {
    uint16_t len;
    char text[CONFIG_CICALA_MAX_QUESTION_BYTES];
};

/** Event channel. The new corpus applies on the next draw without a refresh. */
struct cicala_corpus_msg {
    /** Two-letter code, NUL-terminated. */
    char language[4];
};

/** Event channel: the display finished with the question it was given. */
struct cicala_render_msg {
    /** The `seq` of the question this refers to. */
    uint32_t seq;
    /** 0, or a negative errno from the panel. */
    int result;
    /** True when this was a full refresh rather than a partial one. */
    bool was_full;
};

/** Where power stands. */
enum cicala_power_state {
    /** No plausible reading. Refreshes remain allowed. */
    CICALA_POWER_UNKNOWN = 0,
    /** On the cell, above the refresh floor. */
    CICALA_POWER_NORMAL,
    /** On the cell, under CONFIG_CICALA_REFRESH_MIN_MV. Refreshes are refused. */
    CICALA_POWER_LOW,
    /** On the cell, nearly flat. Refreshes are refused. */
    CICALA_POWER_CRITICAL,
    /** External power is in. */
    CICALA_POWER_CHARGING,
    /** External power is in and the cell reads above the full estimate. */
    CICALA_POWER_CHARGED,
    /** USB present; charger idle/disabled, or its status cannot be read. */
    CICALA_POWER_EXTERNAL_IDLE,
    /** The charger reports a fault; VBUS can still power the system. */
    CICALA_POWER_CHARGE_FAULT,
};

/** State channel for status consumers. Sleep queries power directly. */
struct cicala_power_msg {
    /** At the pack, with the divider undone. */
    uint16_t mv;
    /** One of `enum cicala_power_state`, stored as a byte. */
    uint8_t state;
    /** VBUS. */
    bool usb;
    /** One of enum cicala_charger_status; independent of pack-voltage estimate. */
    uint8_t charger;
};

ZBUS_CHAN_DECLARE(chan_category);
ZBUS_CHAN_DECLARE(chan_next);
ZBUS_CHAN_DECLARE(chan_question);
ZBUS_CHAN_DECLARE(chan_service);
ZBUS_CHAN_DECLARE(chan_corpus);
ZBUS_CHAN_DECLARE(chan_render);
ZBUS_CHAN_DECLARE(chan_power);

/** Deck id for logs, or `?` when out of range. */
const char *cicala_deck_name(uint8_t deck);

/** Card name for logs, or `?` when out of range. */
const char *cicala_card_name(uint8_t kind);

/** Power-state name for logs, or `?` when out of range. */
const char *cicala_power_name(uint8_t state);

/** English deck label shown on the panel for every corpus language. */
const char *cicala_deck_label(uint8_t deck);

/** True when the device offers this deck in the Category cycle. */
bool cicala_deck_on_device(uint8_t deck);

/** The deck one Category press after `deck`; a deck outside the cycle restarts it. */
uint8_t cicala_deck_cycle_next(uint8_t deck);

#ifdef __cplusplus
}
#endif
