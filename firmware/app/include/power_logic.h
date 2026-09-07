/** C face of the power state machine. */

#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Hand the machine one reading and let it settle. */
void cicala_power_post_sample(uint16_t mv, bool usb, uint8_t charger);

/** Hand the machine the VBUS pin alone, and let it settle. */
void cicala_power_post_usb(bool usb, uint8_t charger);

enum cicala_charger_status {
    CICALA_CHARGER_NOT_MONITORED = 0,
    CICALA_CHARGER_CHARGING,
    CICALA_CHARGER_IDLE,
    CICALA_CHARGER_RECOVERABLE_FAULT,
    CICALA_CHARGER_LATCHED_FAULT,
    CICALA_CHARGER_UNAVAILABLE,
};

/** Current state, as a `enum cicala_power_state` value. */
uint8_t cicala_power_state(void);

#ifdef __cplusplus
}
#endif
