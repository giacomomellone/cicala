/*
 * The power state machine — the only thing in the firmware that decides what a
 * millivolt reading means. Everything else measures or reacts.
 *
 * The transition table in power_fsm.cpp is the specification; the diagram in
 * docs/firmware_architecture.md is the same table drawn. This class holds no
 * Zephyr headers and no ADC: readings are pushed in with post_sample(), effects
 * go out through PowerIo. That is what lets the suite drive a five-minute
 * charge window in microseconds, with no cell and no charger.
 *
 * ## CHARGED is an estimate, and nothing may treat it as more than one
 *
 * The bq25185 on the bench board exposes no /CHG pin, so charge termination
 * cannot be sensed. CHARGED is entered when the cell reads above
 * TK_POWER_FULL_MV with external power in, and a lithium cell sits near 4.2 V
 * for the last hour of a constant-voltage taper — so this runs early, and by an
 * amount that depends on the load. That is acceptable for a lamp somebody
 * glances at. It is not acceptable as an input to anything that decides
 * something, which is why the only consumer is the LED.
 *
 * ## Why hysteresis is here and sample counting is not
 *
 * A "wait for N consecutive readings" rule would be dead code on battery.
 * CONFIG_TK_SLEEP_IDLE_MS is two seconds and a wake is a fresh boot, so on the
 * cell this object is constructed, sees one or two samples, and is destroyed by
 * sys_poweroff(). Debouncing by count needs an awake window it will not get.
 *
 * Millivolt hysteresis is different: it costs nothing per sample and it earns
 * its place during the two windows that really do last minutes — a charge, and
 * a portal session. Averaging across a burst of readings, which is the thing
 * that actually has to work on battery, belongs to whoever calls post_sample().
 *
 * ## The floor under the ladder
 *
 * A reading below kPlausibleMv is not treated as a very flat cell but as no
 * cell at all, and lands the machine in UNKNOWN. An SoC that is still running
 * is not being fed by a pack at 2.4 V — the protection circuit and the buck
 * both give up before that — so such a reading is a divider that is not there:
 * a floating pin on a rig where nobody has fitted one. Without the floor that
 * rig walks the ladder to CRITICAL before any thread starts and refuses every
 * refresh, including the first card, which is the one failure the panel cannot
 * report. UNKNOWN permits refreshes, so this fails towards a device that works.
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "fsm.hpp"
#include "power_state.hpp"

namespace tk
{

#ifdef CONFIG_TK_REFRESH_MIN_MV
constexpr uint16_t kRefreshMinMv = CONFIG_TK_REFRESH_MIN_MV;
#else
constexpr uint16_t kRefreshMinMv = 3200;
#endif

#ifdef CONFIG_TK_POWER_CRITICAL_MV
constexpr uint16_t kCriticalMv = CONFIG_TK_POWER_CRITICAL_MV;
#else
constexpr uint16_t kCriticalMv = 3000;
#endif

#ifdef CONFIG_TK_POWER_PLAUSIBLE_MV
constexpr uint16_t kPlausibleMv = CONFIG_TK_POWER_PLAUSIBLE_MV;
#else
constexpr uint16_t kPlausibleMv = 2500;
#endif

#ifdef CONFIG_TK_POWER_FULL_MV
constexpr uint16_t kFullMv = CONFIG_TK_POWER_FULL_MV;
#else
constexpr uint16_t kFullMv = 4050;
#endif

#ifdef CONFIG_TK_POWER_HYSTERESIS_MV
constexpr uint16_t kHysteresisMv = CONFIG_TK_POWER_HYSTERESIS_MV;
#else
constexpr uint16_t kHysteresisMv = 120;
#endif

#ifdef CONFIG_TK_POWER_CHARGE_WINDOW_MS
constexpr int64_t kChargeWindowMs = CONFIG_TK_POWER_CHARGE_WINDOW_MS;
#else
constexpr int64_t kChargeWindowMs = 300000;
#endif

/**
 * Millivolts of movement worth telling anyone about.
 *
 * Not a Kconfig symbol: it exists to stop a state channel carrying ADC noise,
 * and no board would want a different answer. A state change publishes
 * regardless of this.
 */
constexpr uint16_t kPublishDeadbandMv = 20;

/** Everything the power machine can do to the world outside itself. */
class PowerIo
{
public:
    virtual ~PowerIo() = default;

    /**
     * Say where power stands.
     *
     * Called on every state change, and on any reading that has moved more
     * than kPublishDeadbandMv since the last time this was called.
     */
    virtual void publish(PowerState state, uint16_t mv, bool usb) = 0;

    /**
     * External power arrived; the sync and update window opens here.
     *
     * Called once per plug-in, not once per state change — moving between
     * CHARGING and CHARGED must not buy a second window.
     */
    virtual void open_charge_window() = 0;

    /** The window ran out, or external power went away. Must be safe to repeat. */
    virtual void close_charge_window() = 0;
};

class PowerFsm : public Fsm
{
public:
    /*
     * The states are PowerState, rather than an enum of this class's own, so
     * that `lib/status` can name one without depending on this header. STATE()
     * still works: it only needs `State::X` to resolve.
     */
    using State = PowerState;

    /*
     * Named for what happened, not for where it goes — the table owns the
     * destinations. SANK and ROSE carry the whole battery ladder in two names,
     * and FULL exists because CHARGING has two ways out that are not UNPLUGGED.
     */
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

    /**
     * One reading: millivolts at the pack, and whether VBUS is present.
     *
     * The value is kept rather than consumed, so the ladder can walk more than
     * one step from a single sample — a first reading taken on a flat cell
     * reaches CRITICAL over the next two ticks instead of waiting for two more
     * samples that, on battery, will never come.
     */
    void post_sample(uint16_t mv, bool usb);

    /**
     * Whether VBUS is present, with no reading attached.
     *
     * For the tick that has a pin but no conversion. VBUS is a plain GPIO and
     * the ADC is a divider, so a broken divider must not freeze the USB bit:
     * a device whose last sample said CHARGING and whose ADC then failed would
     * otherwise believe it was still plugged in until it was rebooted, and
     * never sleep again.
     *
     * Deliberately not a measurement — it leaves `_mv` alone and does not make
     * the machine believe it has read the cell.
     */
    void post_usb(bool usb);

    PowerState state() const { return static_cast<PowerState>(get_current_state()); }

    /** False only in LOW and CRITICAL. UNKNOWN allows, deliberately. */
    bool refresh_allowed() const;

    /** True while external power is in and the window has not run out. */
    bool charge_window_open() const { return _window_open; }

    /** True whenever VBUS is present, window or no window. Sleep reads this. */
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

    /** Shared by CHARGING and CHARGED, which differ only in the FULL test. */
    int on_external();

    /** False when the last reading is too low to have come from a cell. */
    bool plausible() const { return _mv >= kPlausibleMv; }

    /** Publish if the state just changed or the reading has moved enough. */
    void report(bool force);

    PowerIo &_io;

    /** Sticky: the last reading, not a queue. See post_sample(). */
    uint16_t _mv = 0;
    bool _usb = false;
    bool _measured = false;

    uint16_t _published_mv = 0;
    bool _published = false;

    /** Tracks the window across CHARGING and CHARGED as one visit. */
    bool _external = false;
    bool _window_open = false;
    int64_t _window_close_ms = 0;

    static const Fsm::StateTransition _transitions[];
};

} // namespace tk
