/* The power state machine, and what it does to the rest of the firmware. */

#include "power_logic.h"

#include <zephyr/logging/log.h>
#include <zephyr/zbus/zbus.h>

#include "channels.h"
#include "power.h"
#include "power_fsm.hpp"

LOG_MODULE_REGISTER(tk_power, LOG_LEVEL_INF);

namespace
{

/* The C and C++ power enums must keep the same order. */
static_assert((int) tk::PowerState::UNKNOWN == TK_POWER_UNKNOWN, "power state order");
static_assert((int) tk::PowerState::NORMAL == TK_POWER_NORMAL, "power state order");
static_assert((int) tk::PowerState::LOW == TK_POWER_LOW, "power state order");
static_assert((int) tk::PowerState::CRITICAL == TK_POWER_CRITICAL, "power state order");
static_assert((int) tk::PowerState::CHARGING == TK_POWER_CHARGING, "power state order");
static_assert((int) tk::PowerState::CHARGED == TK_POWER_CHARGED, "power state order");

class Io : public tk::PowerIo
{
public:
    void publish(tk::PowerState state, uint16_t mv, bool usb) override
    {
        struct tk_power_msg msg = {
            .mv = mv,
            .state = (uint8_t) state,
            .usb = usb,
        };

        LOG_INF("%u mV, %s%s", mv, tk_power_name(msg.state), usb ? ", USB in" : "");

        /* Allow the worker to publish an immediate follow-up transition. */
        const int err = zbus_chan_pub(&chan_power, &msg, K_MSEC(50));

        if (err != 0) {
            LOG_WRN("could not publish power state: %d", err);
        }
    }

    void open_charge_window() override
    {
        LOG_INF("external power in; sync and updates are allowed for the next %lld s",
                (long long) (tk::kChargeWindowMs / 1000));
    }

    void close_charge_window() override
    {
        /* Nothing to tear down. */
        LOG_INF("charge window closed");
    }
};

Io io;
tk::PowerFsm fsm(io);

/* Tick until the state stops moving. */
void settle()
{
    /* Bound one sample to the number of power states. */
    for (int i = 0; i < 16; i++) {
        const int before = fsm.get_current_state();

        fsm.run();

        if (fsm.get_current_state() == before) {
            return;
        }
    }

    LOG_ERR("power state machine did not settle");
}

} // namespace

void tk_power_post_sample(uint16_t mv, bool usb)
{
    fsm.post_sample(mv, usb);
    settle();
}

void tk_power_post_usb(bool usb)
{
    fsm.post_usb(usb);
    settle();
}

uint8_t tk_power_state(void)
{
    return (uint8_t) fsm.state();
}

bool tk_power_refresh_allowed(void)
{
    return fsm.refresh_allowed();
}

uint16_t tk_power_millivolts(void)
{
    return fsm.millivolts();
}

bool tk_power_external(void)
{
    return fsm.external();
}

bool tk_power_charge_window_open(void)
{
    return fsm.charge_window_open();
}
