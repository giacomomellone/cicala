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

ZBUS_CHAN_DECLARE(chan_selector);
ZBUS_CHAN_DECLARE(chan_next);

/** Deck name for logs. Returns "?" outside 0..5. */
const char *tk_deck_name(uint8_t deck);

#ifdef __cplusplus
}
#endif
