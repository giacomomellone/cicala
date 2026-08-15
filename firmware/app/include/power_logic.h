/** C face of the power state machine. */

#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Hand the machine one reading and let it settle. */
void kveld_power_post_sample(uint16_t mv, bool usb);

/** Hand the machine the VBUS pin alone, and let it settle. */
void kveld_power_post_usb(bool usb);

/** Current state, as a `enum kveld_power_state` value. */
uint8_t kveld_power_state(void);

#ifdef __cplusplus
}
#endif
