/*
 * C face of the power state machine.
 *
 * `power_logic.cpp` holds the PowerFsm and the PowerIo that drives it;
 * `power.c` owns the ADC, the VBUS pin, the work item and the zbus channel,
 * because ZBUS_CHAN_DEFINE and friends do not compile as C++. This header is
 * the seam, the same way net_logic.h is.
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Hand the machine one reading and let it settle.
 *
 * Settling matters here rather than being a detail: readings are sticky, so a
 * first sample taken on a flat cell has to walk UNKNOWN, NORMAL, LOW and
 * CRITICAL in one call. On battery there is no second sample coming — a wake is
 * a fresh boot and the device is awake for about two seconds.
 */
void tk_power_post_sample(uint16_t mv, bool usb);

/** Tick the machine once. For the callers that need a clock, not a reading. */
void tk_power_run(void);

/** Current state, as a `enum tk_power_state` value. */
uint8_t tk_power_state(void);

#ifdef __cplusplus
}
#endif
