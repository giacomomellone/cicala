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

/** Number of selector contacts, one per deck. */
#define TK_DECK_COUNT 6

/**
 * State channel: which deck the selector is on right now.
 *
 * The selector *is* the deck — there is no stored copy of this anywhere, by
 * design (docs/firmware_architecture.md, "Where state lives"). Zero or several
 * closed contacts is not a deck, it is `valid = false`, and a reader must keep
 * showing whatever it was showing.
 */
struct tk_selector_msg {
    /** 0..5, matching questions/schema.json x-tischkarte.decks. */
    uint8_t deck;
    /** False when zero or several contacts are closed. `deck` is then stale. */
    bool valid;
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

ZBUS_CHAN_DECLARE(chan_selector);
ZBUS_CHAN_DECLARE(chan_next);
ZBUS_CHAN_DECLARE(chan_question);
ZBUS_CHAN_DECLARE(chan_render);

/** Deck name for logs. Returns "?" outside 0..5. */
const char *tk_deck_name(uint8_t deck);

#ifdef __cplusplus
}
#endif
