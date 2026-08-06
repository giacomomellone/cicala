/*
 * C face of the state machine and the question store.
 *
 * `app_logic.cpp` holds the C++17 objects — AppFsm, Qdb, Bag — and `app.c`
 * owns the thread and the zbus subscriber, because the observer macros are
 * the ones that do not compile as C++. This header is the seam.
 */

#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Open the built-in corpus and bind the bag to it.
 *
 * @return 0, or -EINVAL if the embedded bundle does not parse, which would
 *         mean the build embedded something that is not a QDB2 image.
 */
int tk_app_init(void);

/**
 * One Category press happened: advance to the next deck and name it.
 *
 * Wraps from Wild back to New People. The deck is remembered in RTC memory
 * rather than read from a pin, because a button has no position and, once deep
 * sleep exists, every press starts from a fresh boot.
 */
void tk_app_post_category(void);

void tk_app_post_next(void);

/**
 * A render finished.
 *
 * `seq` identifies which question it was for. A result arriving for anything
 * other than the question last drawn is discarded: it belongs to a refresh the
 * state machine already gave up waiting for, and letting it through would end
 * the *next* refresh before the panel had drawn anything.
 */
void tk_app_post_render(bool ok, uint32_t seq);

/**
 * The setup portal wants `text` on the panel.
 *
 * Copied here rather than referenced, and routed through the state machine
 * rather than published straight to the display: app_logic owns the sequence
 * number every card carries, and the guard that drops a late render matches
 * against it.
 */
void tk_app_post_service(const char *text, uint16_t len);

/**
 * Open the corpus the chosen language now names, and rebind the bag.
 *
 * Deliberately does not draw. docs/firmware_architecture.md's rule for
 * chan_corpus is that a new corpus applies on the next *requested* draw: the
 * question on the panel is one somebody is reading, and replacing it because a
 * setting changed would take the table's attention for no reason.
 */
void tk_app_reload_corpus(void);

/**
 * Tick the state machine until it stops moving.
 *
 * One event can walk it through several states — a press is SHOWING, then
 * DRAWING, then REFRESHING — so stopping after one tick would leave it
 * halfway.
 */
void tk_app_run(void);

/**
 * True when the current state has a timeout that has to be honoured.
 *
 * The loop blocks forever when this is false, which is what will let the
 * device reach deep sleep once power management is on.
 */
bool tk_app_needs_timeout(void);

/**
 * What corpus is loaded: its release version, and how many questions it holds.
 *
 * For the setup portal's status page, which is the one place a person can ask
 * the device what it is carrying. `version` is NUL-terminated on return.
 */
void tk_app_corpus(char *version, size_t version_size, uint16_t *count);

/** Current state, as an AppFsm::State value. For logging. */
int tk_app_state(void);

/**
 * True while the panel is being refreshed.
 *
 * A press arriving now is deliberately dropped — a full refresh takes seconds
 * and honouring presses made during it would spend them drawing questions
 * nobody has read. Exposed so the drop can be reported rather than being
 * silent, which from the table looks like a button that does not work.
 */
bool tk_app_is_busy(void);

/**
 * True when the question on the panel is settled and nothing is pending.
 *
 * This is the only state it is safe to sleep from. Every other one is
 * mid-decision: BOOT is still waiting for a valid selector reading, DRAWING and
 * REFRESHING have work in flight, and sleeping through any of them would
 * abandon it — and, since a wake is a fresh boot, lose it.
 */
bool tk_app_is_settled(void);

#ifdef __cplusplus
}
#endif
