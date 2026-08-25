/* The LED arbiter, and nothing else. */

#include "status_logic.h"

#include "channels.h"
#include "status_led.hpp"

namespace
{

/* The two colour enums are the same list in the same order, and the two power enums already are. */
static_assert((int) cicala::Colour::OFF == CICALA_STATUS_OFF, "colour order");
static_assert((int) cicala::Colour::RED == CICALA_STATUS_RED, "colour order");
static_assert((int) cicala::Colour::GREEN == CICALA_STATUS_GREEN, "colour order");
static_assert((int) cicala::Colour::AMBER == CICALA_STATUS_AMBER, "colour order");

cicala::StatusLed led;

} // namespace

void cicala_status_post_power(uint8_t state, bool refresh_blocked)
{
    led.set_power(static_cast<cicala::PowerState>(state), refresh_blocked);
}

void cicala_status_post_portal(bool on_air)
{
    led.set_portal(on_air);
}

void cicala_status_post_activity(bool busy)
{
    led.set_activity(busy);
}

uint8_t cicala_status_output(int64_t now_ms)
{
    return (uint8_t) led.output(now_ms);
}

bool cicala_status_animating(int64_t now_ms)
{
    return led.animating(now_ms);
}
