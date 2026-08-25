/* The power state machine, and what it does to the rest of the firmware. */

#include "power_logic.h"

#include <zephyr/logging/log.h>
#include <zephyr/zbus/zbus.h>

#include "channels.h"
#include "power.h"
#include "power_fsm.hpp"

LOG_MODULE_REGISTER(cicala_power, LOG_LEVEL_INF);

namespace
{

/* The C and C++ power enums must keep the same order. */
static_assert((int) cicala::PowerState::UNKNOWN == CICALA_POWER_UNKNOWN, "power state order");
static_assert((int) cicala::PowerState::NORMAL == CICALA_POWER_NORMAL, "power state order");
static_assert((int) cicala::PowerState::LOW == CICALA_POWER_LOW, "power state order");
static_assert((int) cicala::PowerState::CRITICAL == CICALA_POWER_CRITICAL, "power state order");
static_assert((int) cicala::PowerState::CHARGING == CICALA_POWER_CHARGING, "power state order");
static_assert((int) cicala::PowerState::CHARGED == CICALA_POWER_CHARGED, "power state order");

class Io : public cicala::PowerIo
{
public:
    void publish(cicala::PowerState state, uint16_t mv, bool usb) override
    {
        struct cicala_power_msg msg = {
            .mv = mv,
            .state = (uint8_t) state,
            .usb = usb,
        };

        LOG_INF("%u mV, %s%s", mv, cicala_power_name(msg.state), usb ? ", USB in" : "");

        /* Allow the worker to publish an immediate follow-up transition. */
        const int err = zbus_chan_pub(&chan_power, &msg, K_MSEC(50));

        if (err != 0) {
            LOG_WRN("could not publish power state: %d", err);
        }
    }

    void open_charge_window() override
    {
        LOG_INF("external power in; sync and updates are allowed for the next %lld s",
                (long long) (cicala::kChargeWindowMs / 1000));
    }

    void close_charge_window() override
    {
        /* Nothing to tear down. */
        LOG_INF("charge window closed");
    }
};

Io io;
cicala::PowerFsm fsm(io);

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

void cicala_power_post_sample(uint16_t mv, bool usb)
{
    fsm.post_sample(mv, usb);
    settle();
}

void cicala_power_post_usb(bool usb)
{
    fsm.post_usb(usb);
    settle();
}

uint8_t cicala_power_state(void)
{
    return (uint8_t) fsm.state();
}

bool cicala_power_refresh_allowed(void)
{
    return fsm.refresh_allowed();
}

uint16_t cicala_power_millivolts(void)
{
    return fsm.millivolts();
}

bool cicala_power_external(void)
{
    return fsm.external();
}

bool cicala_power_charge_window_open(void)
{
    return fsm.charge_window_open();
}
