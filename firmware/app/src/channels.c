/*
 * Channel definitions. See include/channels.h for what they carry and
 * docs/firmware_architecture.md for the full table.
 */

#include "channels.h"

/*
 * chan_selector is a state channel: its initial value is what a reader sees
 * before the first settle completes, so it has to be an honest "no deck yet"
 * rather than deck 0. BOOT waits for a valid selector precisely because of
 * this, and a device with a knob between detents must not silently draw from
 * new_people.
 */
ZBUS_CHAN_DEFINE(chan_selector, struct tk_selector_msg, NULL, NULL, ZBUS_OBSERVERS_EMPTY,
                 ZBUS_MSG_INIT(.deck = 0, .valid = false));

ZBUS_CHAN_DEFINE(chan_next, struct tk_next_msg, NULL, NULL, ZBUS_OBSERVERS_EMPTY,
                 ZBUS_MSG_INIT(.timestamp_ms = 0, .duration_ms = 0));

ZBUS_CHAN_DEFINE(chan_question, struct tk_question_msg, NULL, NULL, ZBUS_OBSERVERS_EMPTY,
                 ZBUS_MSG_INIT(.seq = 0, .deck = 0, .len = 0, .text = {0}));

ZBUS_CHAN_DEFINE(chan_render, struct tk_render_msg, NULL, NULL, ZBUS_OBSERVERS_EMPTY,
                 ZBUS_MSG_INIT(.seq = 0, .result = 0, .was_full = false));

/*
 * Index order is the absolute selector order and is duplicated in the board
 * overlays' `label` properties and in questions/schema.json. The overlay
 * labels are not readable from C without a devicetree walk per node, so this
 * table is the one place a log line gets its name from.
 */
static const char *const deck_names[TK_DECK_COUNT] = {
    "new_people", "close", "family", "work", "here", "wild",
};

const char *tk_deck_name(uint8_t deck)
{
    if (deck >= TK_DECK_COUNT) {
        return "?";
    }

    return deck_names[deck];
}
