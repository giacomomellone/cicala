#include "portal_fsm.hpp"

namespace tk
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

/*
 * Everything that goes wrong ends with the radio off.
 *
 * A table bug and a blown timeout are the two ways to get here, and neither is
 * a state the device should sit in with an open access point on air. SHUTDOWN
 * tears down whatever is up and returns to OFF, which is also what the
 * session-window timeout on SERVING uses: the portal closing by itself after
 * five minutes is the ordinary ending, not an error.
 */
int PortalFsm::get_fail_state() const
{
    return STATE(SHUTDOWN);
}

void PortalFsm::on_enter_state(int state)
{
    switch (state) {
    case STATE(SCANNING):
        _scan_done = false;

        /* One radio: this has to finish before the access point takes it. */
        if (!_io.scan_start()) {
            /* No scan is survivable — the setup page still takes a typed name,
             * which a hidden network needs anyway. Carry on without one. */
            _scan_done = true;
        }

        break;

    case STATE(AP_STARTING):
        _ap_pending = false;
        _ap_ok = false;

        if (!_io.ap_start()) {
            /* Report it as the failed result the state is waiting for, rather
             * than sitting here until the timeout. */
            _ap_pending = true;
            _ap_ok = false;
        }

        break;

    case STATE(SERVING):
        _credentials_pending = false;

        /* Only once. SERVING is re-entered after a refused password, and the
         * access point has been on air the whole time. */
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
    /* A stop that arrives with nothing running has nothing to do, and must not
     * be left queued for the next session. */
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

    /*
     * The deadline is here rather than in the table because a scan that never
     * reports back should still get an access point up — the page takes a
     * typed network name, so a missing list costs the user a little typing and
     * not the whole setup flow. A table timeout would end the session instead.
     */
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
        /* Nothing is listening, so nobody can ever submit anything. Sitting
         * here for the whole window with an access point that answers no
         * request is worse than ending. */
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

        /* Back to the page, with the panel saying why. A wrong password is the
         * single most likely thing to happen here, and the fix is to type it
         * again rather than to start the whole gesture over. */
        _refused = true;

        return TRANSITION(REFUSED);
    }

    /*
     * The deadline is here for the same reason it is in SCANNING: the useful
     * destination is not the fail state. A network that never answers should
     * put the user back on the form, not take the portal away.
     */
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

    /* The access point stays up for the rest of the window so the status page
     * can be read, then the table's timeout ends the session. */
    return TRANSITION(REPEAT);
}

int PortalFsm::on_shutdown()
{
    /* Teardown happened on the way in. Anything still queued belongs to a
     * session that is over. */
    _stop_pending = false;
    _credentials_pending = false;
    _connect_pending = false;
    _ap_pending = false;
    _refused = false;

    return TRANSITION(CONTINUE);
}

} // namespace tk
