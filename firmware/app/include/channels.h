/*
 * zbus channels: the contract between the workqueue, `app`, and everything
 * that renders. The full table is in docs/firmware_architecture.md; the two
 * channels here are the input half of it.
 *
 * Channels are defined with an empty observer list and observers attach
 * themselves with ZBUS_CHAN_ADD_OBS from their own file. That keeps
 * channels.c from having to know who is listening, which is what lets the
 * test suites observe the same channels the application does without linking
 * the application's main().
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>

#include <zephyr/zbus/zbus.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Decks, in the order Category advances through them. */
#define TK_DECK_COUNT 6

/**
 * Event channel: one Category press happened.
 *
 * The deck it advances to is not carried here. A button has no position, so
 * the active deck is remembered in RTC memory and owned by app_logic; this
 * channel only says that someone asked for the next one.
 */
struct tk_category_msg {
    /** Uptime at the moment the press started. */
    int64_t timestamp_ms;
    /** Press to release. Telemetry only, as for Next. */
    uint32_t duration_ms;
};

/**
 * Event channel: one Next press happened.
 *
 * `duration_ms` travels but no policy reads it: long press is Next, same as a
 * short one. It exists so a table study can answer "did anyone try to
 * long-press?".
 */
struct tk_next_msg {
    /** Uptime at the moment the press started. */
    int64_t timestamp_ms;
    /** Press to release. Telemetry only. */
    uint32_t duration_ms;
};

/**
 * What a card on the panel is.
 *
 * The panel renders all three the same way — the font chooser gives short text
 * the largest size on its own — but the display log says which is which, and a
 * card that wanted its own styling would branch here.
 */
enum tk_card {
    /** Something drawn from the corpus. */
    TK_CARD_QUESTION = 0,
    /**
     * The name of the deck Category just moved to.
     *
     * It stays up until Next is pressed, which is what makes the deck legible
     * without printing the six names on the case.
     */
    TK_CARD_CATEGORY,
    /**
     * The setup portal, saying which network to join and how it went.
     *
     * Only reachable through the service gesture, and only while `net` is
     * running. Joining a network knocks the phone off the setup access point,
     * so the panel is the only place the result can be reported.
     */
    TK_CARD_SERVICE,
};

/**
 * State channel: the question the panel should be showing.
 *
 * Carries the text by value rather than a pointer into the bundle. The corpus
 * can be replaced by a sync between the publish and the render, and a
 * 128-byte copy is cheaper than the rule that would otherwise be needed about
 * who may free what.
 */
struct tk_question_msg {
    /** Increments per draw, so a stale render can be told from a current one. */
    uint32_t seq;
    uint8_t deck;
    /**
     * One of `enum tk_card`, stored as a byte.
     *
     * Not the enum type itself: that is an int, and the padding it brings
     * would push this message from 136 bytes to 144 — exactly
     * CONFIG_ZBUS_MSG_SUBSCRIBER_NET_BUF_STATIC_DATA_SIZE, which is sized
     * against it. See prj.conf.
     */
    uint8_t kind;
    uint16_t len;
    char text[CONFIG_TK_MAX_QUESTION_BYTES];
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

ZBUS_CHAN_DECLARE(chan_category);
ZBUS_CHAN_DECLARE(chan_next);
ZBUS_CHAN_DECLARE(chan_question);
ZBUS_CHAN_DECLARE(chan_render);

/** Deck id for logs, as it appears in questions/schema.json. "?" outside 0..5. */
const char *tk_deck_name(uint8_t deck);

/** What a card is, for logs. "?" for a value outside `enum tk_card`. */
const char *tk_card_name(uint8_t kind);

/**
 * Deck name as it should be read, for the panel.
 *
 * English whatever the corpus language, matching website/src/i18n.ts, where
 * the German translation deliberately keeps the English deck labels for the
 * first physical prototype.
 */
const char *tk_deck_label(uint8_t deck);

#ifdef __cplusplus
}
#endif
