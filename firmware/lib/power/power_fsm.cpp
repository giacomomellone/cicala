#include "power_fsm.hpp"

#include <zephyr/sys/util.h>

namespace cicala
{

// clang-format off
const Fsm::StateTransition PowerFsm::_transitions[] = {
//   Current State      Transition              Next State         Timeout
    {STATE(UNKNOWN),   TRANSITION(REPEAT),     STATE(UNKNOWN),    0       },
    {STATE(UNKNOWN),   TRANSITION(MEASURED),   STATE(NORMAL),     0       },
    {STATE(UNKNOWN),   TRANSITION(PLUGGED),    STATE(CHARGING),   0       },

    {STATE(NORMAL),    TRANSITION(REPEAT),     STATE(NORMAL),     0       },
    {STATE(NORMAL),    TRANSITION(SANK),       STATE(LOW),        0       },
    {STATE(NORMAL),    TRANSITION(PLUGGED),    STATE(CHARGING),   0       },
    {STATE(NORMAL),    TRANSITION(IMPLAUSIBLE),STATE(UNKNOWN),    0       },

    {STATE(LOW),       TRANSITION(REPEAT),     STATE(LOW),        0       },
    {STATE(LOW),       TRANSITION(SANK),       STATE(CRITICAL),   0       },
    {STATE(LOW),       TRANSITION(ROSE),       STATE(NORMAL),     0       },
    {STATE(LOW),       TRANSITION(PLUGGED),    STATE(CHARGING),   0       },
    {STATE(LOW),       TRANSITION(IMPLAUSIBLE),STATE(UNKNOWN),    0       },

    {STATE(CRITICAL),  TRANSITION(REPEAT),     STATE(CRITICAL),   0       },
    {STATE(CRITICAL),  TRANSITION(ROSE),       STATE(LOW),        0       },
    {STATE(CRITICAL),  TRANSITION(PLUGGED),    STATE(CHARGING),   0       },
    {STATE(CRITICAL),  TRANSITION(IMPLAUSIBLE),STATE(UNKNOWN),    0       },

    {STATE(CHARGING),  TRANSITION(REPEAT),     STATE(CHARGING),   0       },
    {STATE(CHARGING),  TRANSITION(FULL),       STATE(CHARGED),    0       },
    {STATE(CHARGING),  TRANSITION(UNPLUGGED),  STATE(UNKNOWN),    0       },

    {STATE(CHARGED),   TRANSITION(REPEAT),     STATE(CHARGED),    0       },
    {STATE(CHARGED),   TRANSITION(SANK),       STATE(CHARGING),   0       },
    {STATE(CHARGED),   TRANSITION(UNPLUGGED),  STATE(UNKNOWN),    0       },
};
// clang-format on

PowerFsm::PowerFsm(PowerIo &io)
    : Fsm(_transitions, ARRAY_SIZE(_transitions), STATE(UNKNOWN)), _io(io)
{
}

int PowerFsm::get_fail_state() const
{
    return STATE(UNKNOWN);
}

void PowerFsm::post_sample(uint16_t mv, bool usb)
{
    _mv = mv;
    _usb = usb;
    _measured = true;
}

void PowerFsm::post_usb(bool usb)
{
    _usb = usb;
}

bool PowerFsm::refresh_allowed() const
{
    const PowerState now = state();

    return now != PowerState::LOW && now != PowerState::CRITICAL;
}

void PowerFsm::report(bool force)
{
    if (!force && _published) {
        const int32_t moved = (int32_t) _mv - (int32_t) _published_mv;

        if (moved > -(int32_t) kPublishDeadbandMv && moved < (int32_t) kPublishDeadbandMv) {
            return;
        }
    }

    _io.publish(state(), _mv, _usb);

    _published_mv = _mv;
    _published = true;
}

void PowerFsm::on_enter_state(int state)
{
    const bool external = (state == STATE(CHARGING)) || (state == STATE(CHARGED));

    if (external && !_external) {
        _external = true;
        _window_open = true;
        _window_close_ms = now_ms() + kChargeWindowMs;
        _io.open_charge_window();
    } else if (!external && _external) {
        _external = false;

        if (_window_open) {
            _window_open = false;
            _io.close_charge_window();
        }
    }

    report(true);
}

int PowerFsm::handle_current_state()
{
    int transition = TRANSITION(REPEAT);

    switch (get_current_state()) {
    case STATE(UNKNOWN):
        transition = on_unknown();
        break;

    case STATE(NORMAL):
        transition = on_normal();
        break;

    case STATE(LOW):
        transition = on_low();
        break;

    case STATE(CRITICAL):
        transition = on_critical();
        break;

    case STATE(CHARGING):
        transition = on_charging();
        break;

    case STATE(CHARGED):
        transition = on_charged();
        break;

    default:
        break;
    }

    // State changes publish from on_enter_state().
    if (transition == TRANSITION(REPEAT)) {
        report(false);
    }

    return transition;
}

int PowerFsm::on_unknown()
{
    // VBUS remains useful when the ADC is unavailable.
    if (_usb) {
        return TRANSITION(PLUGGED);
    }

    if (!_measured) {
        return TRANSITION(REPEAT);
    }

    if (!plausible()) {
        return TRANSITION(REPEAT);
    }

    // Later states apply the voltage thresholds.
    return TRANSITION(MEASURED);
}

int PowerFsm::on_normal()
{
    if (_usb) {
        return TRANSITION(PLUGGED);
    }

    if (!plausible()) {
        return TRANSITION(IMPLAUSIBLE);
    }

    if (_mv < kRefreshMinMv) {
        return TRANSITION(SANK);
    }

    return TRANSITION(REPEAT);
}

int PowerFsm::on_low()
{
    if (_usb) {
        return TRANSITION(PLUGGED);
    }

    if (!plausible()) {
        return TRANSITION(IMPLAUSIBLE);
    }

    // Falling voltage applies immediately; recovery includes hysteresis.
    if (_mv < kCriticalMv) {
        return TRANSITION(SANK);
    }

    if ((uint32_t) _mv >= (uint32_t) kRefreshMinMv + kHysteresisMv) {
        return TRANSITION(ROSE);
    }

    return TRANSITION(REPEAT);
}

int PowerFsm::on_critical()
{
    if (_usb) {
        return TRANSITION(PLUGGED);
    }

    // Values below the plausible floor are treated as a missing measurement.
    if (!plausible()) {
        return TRANSITION(IMPLAUSIBLE);
    }

    if ((uint32_t) _mv >= (uint32_t) kCriticalMv + kHysteresisMv) {
        return TRANSITION(ROSE);
    }

    return TRANSITION(REPEAT);
}

int PowerFsm::on_external()
{
    if (!_usb) {
        return TRANSITION(UNPLUGGED);
    }

    if (_window_open && now_ms() >= _window_close_ms) {
        _window_open = false;
        _io.close_charge_window();
    }

    return TRANSITION(REPEAT);
}

int PowerFsm::on_charging()
{
    const int transition = on_external();

    if (transition != TRANSITION(REPEAT)) {
        return transition;
    }

    if (_mv >= kFullMv) {
        return TRANSITION(FULL);
    }

    return TRANSITION(REPEAT);
}

int PowerFsm::on_charged()
{
    const int transition = on_external();

    if (transition != TRANSITION(REPEAT)) {
        return transition;
    }

    // Hysteresis prevents oscillation at the full threshold.
    if ((uint32_t) _mv + kHysteresisMv <= (uint32_t) kFullMv) {
        return TRANSITION(SANK);
    }

    return TRANSITION(REPEAT);
}

} // namespace cicala
