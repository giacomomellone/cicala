/*
 * What the rest of the firmware tells the status LEDs.
 *
 * Two things, because everything else the LEDs show they can find out for
 * themselves: power arrives on chan_power, and whether the portal is on air is
 * a question `net` already answers.
 *
 * Guarded the way net.h and power.h are, so callers ask unconditionally and an
 * image with no LEDs — qemu, native_sim, and any board without a tk_leds node
 * — folds the calls away.
 */

#pragma once

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef CONFIG_TK_STATUS_LED

/**
 * A sync or an update is moving bytes.
 *
 * One flag for both. They look the same on the LEDs because the panel names
 * which one is running, and a second rhythm to tell them apart would cost
 * legibility for something nobody is missing.
 */
void tk_status_set_activity(bool busy);

/**
 * The setup portal is on air, or has gone off it.
 *
 * A condition rather than an event: pass true for as long as the portal is
 * reachable, false once it is not. `net` posts it from its own loop, which
 * ticks while the portal runs and blocks when it does not, so both edges
 * arrive.
 */
void tk_status_set_portal(bool on_air);

/**
 * A press was turned away because the cell is under the refresh floor.
 *
 * An event, not a condition: called once per refused press. It is the only
 * answer such a press gets — saying anything on the panel would itself be the
 * refresh being refused — so a second press has to produce a second blink.
 */
void tk_status_note_refresh_blocked(void);

/**
 * Both pins low, and stop looking at them.
 *
 * Called from src/sleep.c on the way to sys_poweroff(), after every reason not
 * to sleep has been ruled out. Deep sleep isolates the pads, so the LEDs would
 * go dark on their own; doing it explicitly means the last thing in the log is
 * true, and it cancels a blink that would otherwise be rescheduled into a
 * device that no longer exists.
 */
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
