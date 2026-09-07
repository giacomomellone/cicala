/* The power state machine, and what it does to the rest of the firmware. */

#include "power_logic.h"

#include <zephyr/kernel.h>
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
static_assert((int) cicala::PowerState::EXTERNAL_IDLE == CICALA_POWER_EXTERNAL_IDLE,
              "power state order");
static_assert((int) cicala::PowerState::CHARGE_FAULT == CICALA_POWER_CHARGE_FAULT,
              "power state order");
static_assert((int) cicala::ChargerStatus::NOT_MONITORED == CICALA_CHARGER_NOT_MONITORED);
static_assert((int) cicala::ChargerStatus::CHARGING == CICALA_CHARGER_CHARGING);
static_assert((int) cicala::ChargerStatus::IDLE == CICALA_CHARGER_IDLE);
static_assert((int) cicala::ChargerStatus::RECOVERABLE_FAULT == CICALA_CHARGER_RECOVERABLE_FAULT);
static_assert((int) cicala::ChargerStatus::LATCHED_FAULT == CICALA_CHARGER_LATCHED_FAULT);
static_assert((int) cicala::ChargerStatus::UNAVAILABLE == CICALA_CHARGER_UNAVAILABLE);

K_MUTEX_DEFINE(power_mutex);

class PowerLock
{
public:
    PowerLock() { k_mutex_lock(&power_mutex, K_FOREVER); }
    ~PowerLock() { k_mutex_unlock(&power_mutex); }
};

const char *const charger_names[] = {
    "not monitored",     "charging",      "idle or disabled",
    "recoverable fault", "latched fault", "unavailable",
};

class Io : public cicala::PowerIo
{
public:
    void publish(cicala::PowerState state, uint16_t mv, bool usb,
                 cicala::ChargerStatus charger) override
    {
        struct cicala_power_msg msg = {
            .mv = mv,
            .state = (uint8_t) state,
            .usb = usb,
            .charger = (uint8_t) charger,
        };

        LOG_INF("%u mV, %s%s", mv, cicala_power_name(msg.state), usb ? ", USB in" : "");
        if (usb && charger != cicala::ChargerStatus::NOT_MONITORED) {
            LOG_INF("BQ25185: %s", charger_names[msg.charger]);
        }

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

void cicala_power_post_sample(uint16_t mv, bool usb, uint8_t charger)
{
    PowerLock lock;
    fsm.post_charger(static_cast<cicala::ChargerStatus>(charger));
    fsm.post_sample(mv, usb);
    settle();
}

void cicala_power_post_usb(bool usb, uint8_t charger)
{
    PowerLock lock;
    fsm.post_charger(static_cast<cicala::ChargerStatus>(charger));
    fsm.post_usb(usb);
    settle();
}

uint8_t cicala_power_state(void)
{
    PowerLock lock;
    return (uint8_t) fsm.state();
}

bool cicala_power_refresh_allowed(void)
{
    PowerLock lock;
    return fsm.refresh_allowed();
}

uint16_t cicala_power_millivolts(void)
{
    PowerLock lock;
    return fsm.millivolts();
}

bool cicala_power_external(void)
{
    PowerLock lock;
    return fsm.external();
}

bool cicala_power_charge_window_open(void)
{
    PowerLock lock;
    return fsm.charge_window_open();
}
