/** What the rest of the firmware tells the status LEDs. */

#pragma once

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef CONFIG_TK_STATUS_LED

/** A sync or an update is moving bytes. */
void tk_status_set_activity(bool busy);

/** The setup portal is on air, or has gone off it. */
void tk_status_set_portal(bool on_air);

/** A press was turned away because the cell is under the refresh floor. */
void tk_status_note_refresh_blocked(void);

/** Both pins low, and stop looking at them. */
void tk_status_off(void);

#else

static inline void tk_status_set_activity(bool busy)
{
    (void) busy;
}

static inline void tk_status_set_portal(bool on_air)
{
    (void) on_air;
}

static inline void tk_status_note_refresh_blocked(void) {}

static inline void tk_status_off(void) {}

#endif /* CONFIG_TK_STATUS_LED */

#ifdef __cplusplus
}
#endif
