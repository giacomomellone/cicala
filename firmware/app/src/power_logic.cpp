/* The power state machine, and what it does to the rest of the firmware. */

#include "power_logic.h"

#include <zephyr/logging/log.h>
#include <zephyr/zbus/zbus.h>

#include "channels.h"
#include "power.h"
#include "power_fsm.hpp"

LOG_MODULE_REGISTER(kveld_power, LOG_LEVEL_INF);

namespace
{

/* The C and C++ power enums must keep the same order. */
static_assert((int) kveld::PowerState::UNKNOWN == KVELD_POWER_UNKNOWN, "power state order");
static_assert((int) kveld::PowerState::NORMAL == KVELD_POWER_NORMAL, "power state order");
static_assert((int) kveld::PowerState::LOW == KVELD_POWER_LOW, "power state order");
static_assert((int) kveld::PowerState::CRITICAL == KVELD_POWER_CRITICAL, "power state order");
static_assert((int) kveld::PowerState::CHARGING == KVELD_POWER_CHARGING, "power state order");
static_assert((int) kveld::PowerState::CHARGED == KVELD_POWER_CHARGED, "power state order");

class Io : public kveld::PowerIo
{
public:
    void publish(kveld::PowerState state, uint16_t mv, bool usb) override
    {
        struct kveld_power_msg msg = {
            .mv = mv,
            .state = (uint8_t) state,
            .usb = usb,
        };

        LOG_INF("%u mV, %s%s", mv, kveld_power_name(msg.state), usb ? ", USB in" : "");

        /* Allow the worker to publish an immediate follow-up transition. */
        const int err = zbus_chan_pub(&chan_power, &msg, K_MSEC(50));

        if (err != 0) {
            LOG_WRN("could not publish power state: %d", err);
        }
    }

    void open_charge_window() override
    {
        LOG_INF("external power in; sync and updates are allowed for the next %lld s",
                (long long) (kveld::kChargeWindowMs / 1000));
    }

    void close_charge_window() override
    {
        /* Nothing to tear down. */
        LOG_INF("charge window closed");
    }
};

Io io;
kveld::PowerFsm fsm(io);

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

void kveld_power_post_sample(uint16_t mv, bool usb)
{
    fsm.post_sample(mv, usb);
    settle();
}

void kveld_power_post_usb(bool usb)
{
    fsm.post_usb(usb);
    settle();
}

uint8_t kveld_power_state(void)
{
    return (uint8_t) fsm.state();
}

bool kveld_power_refresh_allowed(void)
{
    return fsm.refresh_allowed();
}

uint16_t kveld_power_millivolts(void)
{
    return fsm.millivolts();
}

bool kveld_power_external(void)
{
    return fsm.external();
}

bool kveld_power_charge_window_open(void)
{
    return fsm.charge_window_open();
}
