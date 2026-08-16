/** What the rest of the firmware needs to know about `power`. */

#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef CONFIG_KVELD_POWER

/** True unless the cell is under CONFIG_KVELD_REFRESH_MIN_MV. */
bool kveld_power_refresh_allowed(void);

/** The last reading in millivolts at the pack, or 0 before there is one. */
uint16_t kveld_power_millivolts(void);

/** True whenever VBUS is present. */
bool kveld_power_external(void);

/** True while a plug-in still has sync and update time left on it. */
bool kveld_power_charge_window_open(void);

#else

static inline bool kveld_power_refresh_allowed(void)
{
    return true;
}

static inline uint16_t kveld_power_millivolts(void)
{
    return 0;
}

static inline bool kveld_power_external(void)
{
    return false;
}

static inline bool kveld_power_charge_window_open(void)
{
    return false;
}

#endif /* CONFIG_KVELD_POWER */

#ifdef __cplusplus
}
#endif
