/* Battery and external-power state machine. */

#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "fsm.hpp"
#include "power_state.hpp"

namespace kveld
{

#ifdef CONFIG_KVELD_REFRESH_MIN_MV
constexpr uint16_t kRefreshMinMv = CONFIG_KVELD_REFRESH_MIN_MV;
#else
constexpr uint16_t kRefreshMinMv = 3200;
#endif

#ifdef CONFIG_KVELD_POWER_CRITICAL_MV
constexpr uint16_t kCriticalMv = CONFIG_KVELD_POWER_CRITICAL_MV;
#else
constexpr uint16_t kCriticalMv = 3000;
#endif

#ifdef CONFIG_KVELD_POWER_PLAUSIBLE_MV
constexpr uint16_t kPlausibleMv = CONFIG_KVELD_POWER_PLAUSIBLE_MV;
#else
constexpr uint16_t kPlausibleMv = 2500;
#endif

#ifdef CONFIG_KVELD_POWER_FULL_MV
constexpr uint16_t kFullMv = CONFIG_KVELD_POWER_FULL_MV;
#else
constexpr uint16_t kFullMv = 4050;
#endif

#ifdef CONFIG_KVELD_POWER_HYSTERESIS_MV
constexpr uint16_t kHysteresisMv = CONFIG_KVELD_POWER_HYSTERESIS_MV;
#else
constexpr uint16_t kHysteresisMv = 120;
#endif

#ifdef CONFIG_KVELD_POWER_CHARGE_WINDOW_MS
constexpr int64_t kChargeWindowMs = CONFIG_KVELD_POWER_CHARGE_WINDOW_MS;
#else
constexpr int64_t kChargeWindowMs = 300000;
#endif

/** Minimum voltage change published without a state change. */
constexpr uint16_t kPublishDeadbandMv = 20;

/** Effects performed by the power state machine. */
class PowerIo
{
public:
    virtual ~PowerIo() = default;

    /** Publish every state change and voltage changes above the deadband. */
    virtual void publish(PowerState state, uint16_t mv, bool usb) = 0;

    /** Open the sync and update window once per connection to external power. */
    virtual void open_charge_window() = 0;

    /** The window ran out, or external power went away. Must be safe to repeat. */
    virtual void close_charge_window() = 0;
};

class PowerFsm : public Fsm
{
public:
    // Shared with lib/status without exposing this state machine.
    using State = PowerState;

    enum class Transition {
        REPEAT = 0,  ///< nothing changed; stay put
        MEASURED,    ///< the first honest reading of this boot arrived
        SANK,        ///< the cell dropped past a threshold
        ROSE,        ///< it came back past one, by more than the hysteresis
        PLUGGED,     ///< external power is present
        UNPLUGGED,   ///< it is not, and was
        FULL,        ///< the cell reads full while charging
        IMPLAUSIBLE, ///< the reading is too low to be a cell at all
    };

    explicit PowerFsm(PowerIo &io);

    /** Post pack millivolts and VBUS state. The latest value remains available. */
    void post_sample(uint16_t mv, bool usb);

    /** Post VBUS without changing the stored voltage or measurement state. */
    void post_usb(bool usb);

    PowerState state() const { return static_cast<PowerState>(get_current_state()); }

    /** Return false in LOW and CRITICAL. */
    bool refresh_allowed() const;

    /** True while external power is in and the window has not run out. */
    bool charge_window_open() const { return _window_open; }

    /** Return whether VBUS is present. */
    bool external() const { return _usb; }

    /** The last reading, or 0 before one arrives. */
    uint16_t millivolts() const { return _mv; }

protected:
    int get_fail_state() const override;
    int handle_current_state() override;
    void on_enter_state(int state) override;

private:
    int on_unknown();
    int on_normal();
    int on_low();
    int on_critical();
    int on_charging();
    int on_charged();

    int on_external();

    /** False when the last reading is too low to have come from a cell. */
    bool plausible() const { return _mv >= kPlausibleMv; }

    /** Publish if the state just changed or the reading has moved enough. */
    void report(bool force);

    PowerIo &_io;

    /** Latest reading. */
    uint16_t _mv = 0;
    bool _usb = false;
    bool _measured = false;

    uint16_t _published_mv = 0;
    bool _published = false;

    /** Tracks one continuous connection to external power. */
    bool _external = false;
    bool _window_open = false;
    int64_t _window_close_ms = 0;

    static const Fsm::StateTransition _transitions[];
};

} // namespace kveld
