/*
 * C face of the state machine and the question store.
 *
 * `app_logic.cpp` holds the C++17 objects — AppFsm, Qdb, Bag — and `app.c`
 * owns the thread and the zbus subscriber, because the observer macros are
 * the ones that do not compile as C++. This header is the seam.
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Open the built-in corpus and bind the bag to it.
 *
 * @return 0, or -EINVAL if the embedded bundle does not parse, which would
 *         mean the build embedded something that is not a TKB2 image.
 */
int tk_app_init(void);

void tk_app_post_selector(uint8_t deck, bool valid);
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

/** Current state, as an AppFsm::State value. For logging. */
int tk_app_state(void);

#ifdef __cplusplus
}
#endif
