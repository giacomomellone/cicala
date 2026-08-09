/*
 * The power state machine, and what it does to the rest of the firmware.
 *
 * Split from power.c for the reason net_logic.cpp is split from net.c: the
 * machine is C++ and the zbus macros around it are not.
 *
 * Nothing here reads an ADC or a pin. A reading arrives through
 * tk_power_post_sample() and leaves as a state on chan_power, a yes-or-no
 * answer to the three questions in power.h, and — once per plug-in — a window
 * `net` is allowed to sync in.
 */

#include "power_logic.h"

#include <zephyr/logging/log.h>
#include <zephyr/zbus/zbus.h>

#include "channels.h"
#include "power.h"
#include "power_fsm.hpp"

LOG_MODULE_REGISTER(tk_power, LOG_LEVEL_INF);

namespace
{

/*
 * The C enum and the C++ one are the same list in the same order, which is
 * what makes the cast below a cast rather than a table. Kept honest here so a
 * value added to one and not the other fails the build.
 */
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

        /*
         * A short timeout rather than K_NO_WAIT. This runs on the system
         * workqueue, so blocking here would stall every other work item — but
         * dropping the very first reading would leave `net` deciding whether
         * to sync against a channel that still says UNKNOWN.
         */
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
        /*
         * Nothing to tear down. `net` reads tk_power_charge_window_open()
         * when it is deciding whether to start something, and a transfer
         * already running is left to finish — stopping one part-way costs the
         * whole download and gains a few seconds of radio.
         */
        LOG_INF("charge window closed");
    }
};

Io io;
tk::PowerFsm fsm(io);

} // namespace

void tk_power_post_sample(uint16_t mv, bool usb)
{
    fsm.post_sample(mv, usb);
    tk_power_run();
}

void tk_power_run(void)
{
    /* Bounded rather than while(changed), for the reason tk_net_run() is: a
     * table bug that made two states point at each other would otherwise spin
     * here forever instead of being noticed. Four rungs is the longest honest
     * walk — UNKNOWN to CRITICAL — so sixteen is room to spare. */
    for (int i = 0; i < 16; i++) {
        const int before = fsm.get_current_state();

        fsm.run();

        if (fsm.get_current_state() == before) {
            return;
        }
    }

    LOG_ERR("power state machine did not settle");
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
