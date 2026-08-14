/** C face of the state machine and the question store. */

#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Open the selected corpus and bind the bag. Return 0 or `-EINVAL`. */
int tk_app_init(void);

/** One Category press happened: advance to the next deck and name it. */
void tk_app_post_category(void);

/** One Next press happened. */
void tk_app_post_next(void);

/** Post a render result; stale sequence numbers are ignored. */
void tk_app_post_render(bool ok, uint32_t seq);

/** Copy a setup-portal card into the state machine. */
void tk_app_post_service(const char *text, uint16_t len);

/** Reopen the selected corpus without changing the current panel. */
void tk_app_reload_corpus(void);

/** Tick the state machine until it stops moving. */
void tk_app_run(void);

/** True when the current state has a timeout that has to be honoured. */
bool tk_app_needs_timeout(void);

/** Copy the NUL-terminated corpus version and return its question count. */
void tk_app_corpus(char *version, size_t version_size, uint16_t *count);

/** Current state, as an AppFsm::State value. */
int tk_app_state(void);

/** True while the panel is refreshing and input will be dropped. */
bool tk_app_is_busy(void);

/** True when the app can safely enter deep sleep. */
bool tk_app_is_settled(void);

#ifdef __cplusplus
}
#endif
