#include "portal_fsm.hpp"

namespace kveld
{

// clang-format off
const Fsm::StateTransition PortalFsm::_transitions[] = {
//   Current State          Transition             Next State             Timeout
    {STATE(OFF),           TRANSITION(REPEAT),    STATE(OFF),            0                },
    {STATE(OFF),           TRANSITION(CONTINUE),  STATE(SCANNING),       0                },

    {STATE(SCANNING),      TRANSITION(REPEAT),    STATE(SCANNING),       0                },
    {STATE(SCANNING),      TRANSITION(CONTINUE),  STATE(AP_STARTING),    0                },
    {STATE(SCANNING),      TRANSITION(STOPPED),   STATE(SHUTDOWN),       0                },

    {STATE(AP_STARTING),   TRANSITION(REPEAT),    STATE(AP_STARTING),    kConnectTimeoutMs},
    {STATE(AP_STARTING),   TRANSITION(CONTINUE),  STATE(SERVING),        0                },
    {STATE(AP_STARTING),   TRANSITION(FAILED),    STATE(SHUTDOWN),       0                },
    {STATE(AP_STARTING),   TRANSITION(STOPPED),   STATE(SHUTDOWN),       0                },

    {STATE(SERVING),       TRANSITION(REPEAT),    STATE(SERVING),        kPortalWindowMs  },
    {STATE(SERVING),       TRANSITION(SUBMITTED), STATE(CONNECTING),     0                },
    {STATE(SERVING),       TRANSITION(FAILED),    STATE(SHUTDOWN),       0                },
    {STATE(SERVING),       TRANSITION(STOPPED),   STATE(SHUTDOWN),       0                },

    {STATE(CONNECTING),    TRANSITION(REPEAT),    STATE(CONNECTING),     0                },
    {STATE(CONNECTING),    TRANSITION(JOINED),    STATE(CONNECTED),      0                },
    {STATE(CONNECTING),    TRANSITION(REFUSED),   STATE(SERVING),        0                },
    {STATE(CONNECTING),    TRANSITION(STOPPED),   STATE(SHUTDOWN),       0                },

    {STATE(CONNECTED),     TRANSITION(REPEAT),    STATE(CONNECTED),      kPortalWindowMs  },
    {STATE(CONNECTED),     TRANSITION(STOPPED),   STATE(SHUTDOWN),       0                },

    {STATE(SHUTDOWN),      TRANSITION(REPEAT),    STATE(SHUTDOWN),       0                },
    {STATE(SHUTDOWN),      TRANSITION(CONTINUE),  STATE(OFF),            0                },
};
// clang-format on

PortalFsm::PortalFsm(PortalIo &io)
    : Fsm(_transitions, sizeof(_transitions) / sizeof(_transitions[0]), STATE(OFF)), _io(io)
{
}

int PortalFsm::get_fail_state() const
{
    return STATE(SHUTDOWN);
}

void PortalFsm::on_enter_state(int state)
{
    switch (state) {
    case STATE(SCANNING):
        _scan_done = false;

        // Scanning and the access point share one radio.
        if (!_io.scan_start()) {
            // The form accepts a typed SSID when scanning fails.
            _scan_done = true;
        }

        break;

    case STATE(AP_STARTING):
        _ap_pending = false;
        _ap_ok = false;

        if (!_io.ap_start()) {
            _ap_pending = true;
            _ap_ok = false;
        }

        break;

    case STATE(SERVING):
        _credentials_pending = false;

        // A refused password returns to the existing access point.
        if (!_ap_up) {
            _ap_up = _io.serve_start();
        }

        _io.show(_refused ? PortalCard::REFUSED : PortalCard::SETUP);
        break;

    case STATE(CONNECTING):
        _connect_pending = false;
        _connect_ok = false;
        _refused = false;

        _io.show(PortalCard::CONNECTING);

        if (!_io.connect_start()) {
            _connect_pending = true;
            _connect_ok = false;
        }

        break;

    case STATE(CONNECTED):
        _refused = false;
        _io.show(PortalCard::CONNECTED);
        break;

    case STATE(SHUTDOWN):
        _io.teardown();
        _ap_up = false;
        break;

    default:
        break;
    }
}

int PortalFsm::handle_current_state()
{
    switch (get_current_state()) {
    case STATE(OFF):
        return on_off();

    case STATE(SCANNING):
        return on_scanning();

    case STATE(AP_STARTING):
        return on_ap_starting();

    case STATE(SERVING):
        return on_serving();

    case STATE(CONNECTING):
        return on_connecting();

    case STATE(CONNECTED):
        return on_connected();

    case STATE(SHUTDOWN):
    default:
        return on_shutdown();
    }
}

void PortalFsm::post_start()
{
    _start_pending = true;
}

void PortalFsm::post_stop()
{
    _stop_pending = true;
}

void PortalFsm::post_scan_done()
{
    _scan_done = true;
}

void PortalFsm::post_ap_ready(bool ok)
{
    _ap_pending = true;
    _ap_ok = ok;
}

void PortalFsm::post_credentials()
{
    _credentials_pending = true;
}

void PortalFsm::post_connected(bool ok)
{
    _connect_pending = true;
    _connect_ok = ok;
}

int PortalFsm::on_off()
{
    _stop_pending = false;

    if (!_start_pending) {
        return TRANSITION(REPEAT);
    }

    _start_pending = false;

    return TRANSITION(CONTINUE);
}

int PortalFsm::on_scanning()
{
    if (_stop_pending) {
        _stop_pending = false;

        return TRANSITION(STOPPED);
    }

    // A scan timeout continues with an empty network list.
    if (_scan_done || get_elapsed_state_time() >= kPortalScanMs) {
        return TRANSITION(CONTINUE);
    }

    return TRANSITION(REPEAT);
}

int PortalFsm::on_ap_starting()
{
    if (_stop_pending) {
        _stop_pending = false;

        return TRANSITION(STOPPED);
    }

    if (!_ap_pending) {
        return TRANSITION(REPEAT);
    }

    _ap_pending = false;

    return _ap_ok ? TRANSITION(CONTINUE) : TRANSITION(FAILED);
}

int PortalFsm::on_serving()
{
    if (_stop_pending) {
        _stop_pending = false;

        return TRANSITION(STOPPED);
    }

    if (!_ap_up) {
        return TRANSITION(FAILED);
    }

    if (!_credentials_pending) {
        return TRANSITION(REPEAT);
    }

    _credentials_pending = false;

    return TRANSITION(SUBMITTED);
}

int PortalFsm::on_connecting()
{
    if (_stop_pending) {
        _stop_pending = false;

        return TRANSITION(STOPPED);
    }

    if (_connect_pending) {
        _connect_pending = false;

        if (_connect_ok) {
            return TRANSITION(JOINED);
        }

        // Keep the portal open so credentials can be corrected.
        _refused = true;

        return TRANSITION(REFUSED);
    }

    // A connection timeout returns to the form.
    if (get_elapsed_state_time() >= kConnectTimeoutMs) {
        _refused = true;

        return TRANSITION(REFUSED);
    }

    return TRANSITION(REPEAT);
}

int PortalFsm::on_connected()
{
    if (_stop_pending) {
        _stop_pending = false;

        return TRANSITION(STOPPED);
    }

    // Keep the status page available until the session ends.
    return TRANSITION(REPEAT);
}

int PortalFsm::on_shutdown()
{
    _stop_pending = false;
    _credentials_pending = false;
    _connect_pending = false;
    _ap_pending = false;
    _refused = false;

    return TRANSITION(CONTINUE);
}

} // namespace kveld
