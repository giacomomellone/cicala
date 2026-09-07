/** What the rest of the firmware needs to know about `power`. */

#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef CONFIG_CICALA_POWER

/** True unless the cell is under CONFIG_CICALA_REFRESH_MIN_MV. */
bool cicala_power_refresh_allowed(void);

/** The last reading in millivolts at the pack, or 0 before there is one. */
uint16_t cicala_power_millivolts(void);

/** True whenever VBUS is present. */
bool cicala_power_external(void);

/** True while a plug-in still has sync and update time left on it. */
bool cicala_power_charge_window_open(void);

/** Finish any ADC burst and disable its switch before GPIO sleep holds. */
void cicala_power_prepare_sleep(void);

#else

static inline bool cicala_power_refresh_allowed(void)
{
    return true;
}

static inline uint16_t cicala_power_millivolts(void)
{
    return 0;
}

static inline bool cicala_power_external(void)
{
    return false;
}

static inline bool cicala_power_charge_window_open(void)
{
    return false;
}

static inline void cicala_power_prepare_sleep(void) {}

#endif /* CONFIG_CICALA_POWER */

#ifdef __cplusplus
}
#endif
