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

/**
 * Event channel: the setup portal has something to say.
 *
 * Published by `net`, read by `app`. It does not go straight to the display,
 * because `app_logic` owns the sequence number every card is stamped with and
 * the guard that drops a late render matches against it — a second publisher on
 * chan_question would break that guard rather than merely race it.
 *
 * `app` stays the only thread that decides anything, and the only publisher of
 * chan_question.
 */
struct tk_service_msg {
    uint16_t len;
    char text[CONFIG_TK_MAX_QUESTION_BYTES];
};

/**
 * Event channel: the corpus has been replaced.
 *
 * Published by `net` when the setup portal changes the language, and by `sync`
 * when a downloaded bundle is swapped in. Read by `app`, which reopens the
 * store and rebinds the bag.
 *
 * **This must not trigger a redraw.** The new corpus applies on the next
 * *requested* draw. The question on the panel is one somebody is reading, and
 * replacing it because a setting changed takes the table's attention for
 * nothing.
 */
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

/**
 * Where power stands.
 *
 * The C face of `tk::PowerState` in lib/power. Same order, and power_logic.cpp
 * casts between them, so a value added here has to be added there.
 */
enum tk_power_state {
    /** Nothing measured yet, or the reading made no sense. Refreshes allowed. */
    TK_POWER_UNKNOWN = 0,
    /** On the cell, above the refresh floor. */
    TK_POWER_NORMAL,
    /** On the cell, under CONFIG_TK_REFRESH_MIN_MV. Refreshes refused. */
    TK_POWER_LOW,
    /** On the cell, nearly flat. Refreshes refused, and said so loudly. */
    TK_POWER_CRITICAL,
    /** External power is in. */
    TK_POWER_CHARGING,
    /** External power is in and the cell reads full. An estimate — see lib/power. */
    TK_POWER_CHARGED,
};

/**
 * State channel: the cell, and whether anything is feeding it.
 *
 * Published by the system workqueue from `power`, read by `app` and `net`.
 *
 * Deliberately not read by `sleep`. Adding it to that file's listener would
 * rearm the idle timer on every sample, and TK_POWER_SAMPLE_MS is five times
 * TK_SLEEP_IDLE_MS — survivable today, and a device that never sleeps again
 * the moment either number moves. `sleep.c` calls tk_power_external() instead.
 */
struct tk_power_msg {
    /** At the pack, with the divider undone. 0 before the first reading. */
    uint16_t mv;
    /**
     * One of `enum tk_power_state`, stored as a byte.
     *
     * Not the enum type itself, for the reason tk_question_msg::kind is not.
     */
    uint8_t state;
    /** VBUS. Says nothing about whether current is flowing into the cell. */
    bool usb;
};

ZBUS_CHAN_DECLARE(chan_category);
ZBUS_CHAN_DECLARE(chan_next);
ZBUS_CHAN_DECLARE(chan_question);
ZBUS_CHAN_DECLARE(chan_service);
ZBUS_CHAN_DECLARE(chan_corpus);
ZBUS_CHAN_DECLARE(chan_render);
ZBUS_CHAN_DECLARE(chan_power);

/** Deck id for logs, as it appears in questions/schema.json. "?" outside 0..5. */
const char *tk_deck_name(uint8_t deck);

/** What a card is, for logs. "?" for a value outside `enum tk_card`. */
const char *tk_card_name(uint8_t kind);

/** Where power stands, for logs. "?" outside `enum tk_power_state`. */
const char *tk_power_name(uint8_t state);

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
