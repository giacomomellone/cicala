/*
 * What the rest of the firmware needs to know about `net`.
 *
 * Almost nothing, deliberately. The tabletop loop does not know the radio
 * exists: the portal reaches the panel through chan_service like anything else,
 * and the only other question anybody asks is whether it is safe to sleep.
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef CONFIG_TK_NET

/**
 * True while anything is on air.
 *
 * Deep sleep stops the SoC, which would take the access point down mid-setup
 * and, since a wake is a fresh boot, lose the session. src/sleep.c asks this
 * before every sleep. docs/firmware_architecture.md calls it the PM lock; with
 * sys_poweroff() rather than a PM policy state, an explicit guard is the honest
 * version of the same thing.
 */
bool tk_net_is_active(void);

/**
 * The form was posted and the credentials are stored.
 *
 * Called from the HTTP server's own thread, so it only sets a bit and wakes
 * `net`. Everything that touches the state machine happens on the net thread.
 */
void tk_net_notify_credentials(void);

#else

/* No radio in this image, so nothing is ever on air. Inline rather than a
 * second translation unit, so sleep.c can ask unconditionally and the compiler
 * drops the branch — the same shape as tk_wake_button() in sleep.h. */
static inline bool tk_net_is_active(void)
{
    return false;
}

#endif /* CONFIG_TK_NET */

#ifdef __cplusplus
}
#endif
