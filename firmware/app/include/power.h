/*
 * What the rest of the firmware needs to know about `power`.
 *
 * Almost nothing, deliberately, and none of it is a voltage. Everything that
 * turns millivolts into a decision happens in lib/power; what leaves this
 * header is three questions with yes-or-no answers, plus the reading itself for
 * the one caller that has to put a number in a log line.
 *
 * The pattern, and the reason for it, is net.h: sleep.c and app_logic.cpp ask
 * these unconditionally, and an image built without CONFIG_TK_POWER gets inline
 * answers the compiler folds away rather than a second translation unit.
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef CONFIG_TK_POWER

/**
 * True unless the cell is under CONFIG_TK_REFRESH_MIN_MV.
 *
 * A refresh near brownout can leave the panel in a corrupted state, so below
 * the floor the previous question stays on the glass. False in LOW and
 * CRITICAL; true in every other state, including UNKNOWN — a failed ADC must
 * not stop the device drawing questions.
 */
bool tk_power_refresh_allowed(void);

/** The last reading in millivolts at the pack, or 0 before there is one. */
uint16_t tk_power_millivolts(void);

/**
 * True whenever VBUS is present.
 *
 * src/sleep.c asks this before every sleep and stays awake while it is true.
 * The status LEDs need the SoC running to be lit, and red turning green across
 * a charge is most of the point of having them; on external power the current
 * that costs is somebody else's.
 */
bool tk_power_external(void);

/**
 * True while a plug-in still has sync and update time left on it.
 *
 * One window per plug-in, bounded by CONFIG_TK_POWER_CHARGE_WINDOW_MS. `net`
 * asks this rather than tk_power_external(), so a device left on a charger
 * overnight does not retry a download for eight hours.
 */
bool tk_power_charge_window_open(void);

#else

static inline bool tk_power_refresh_allowed(void)
{
    return true;
}

static inline uint16_t tk_power_millivolts(void)
{
    return 0;
}

static inline bool tk_power_external(void)
{
    return false;
}

static inline bool tk_power_charge_window_open(void)
{
    return false;
}

#endif /* CONFIG_TK_POWER */

#ifdef __cplusplus
}
#endif
