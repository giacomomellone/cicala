/* Setup portal state machine. Inputs use post_*(); effects use PortalIo. */

#pragma once

#include <stdint.h>

#include "fsm.hpp"

namespace kveld
{

#ifdef CONFIG_KVELD_PORTAL_WINDOW_MS
constexpr int64_t kPortalWindowMs = CONFIG_KVELD_PORTAL_WINDOW_MS;
#else
constexpr int64_t kPortalWindowMs = 300000;
#endif

#ifdef CONFIG_KVELD_PORTAL_SCAN_MS
constexpr int64_t kPortalScanMs = CONFIG_KVELD_PORTAL_SCAN_MS;
#else
constexpr int64_t kPortalScanMs = 8000;
#endif

#ifdef CONFIG_KVELD_NET_CONNECT_TIMEOUT_MS
constexpr int64_t kConnectTimeoutMs = CONFIG_KVELD_NET_CONNECT_TIMEOUT_MS;
#else
constexpr int64_t kConnectTimeoutMs = 20000;
#endif

/** Service card shown on the panel. */
enum class PortalCard {
    SETUP,
    CONNECTING,
    CONNECTED,
    REFUSED,
};

/** Effects performed by the portal state machine. */
class PortalIo
{
public:
    virtual ~PortalIo() = default;

    /** Start a scan. Completion arrives through post_scan_done(). */
    virtual bool scan_start() = 0;

    /** Start the access point. Completion arrives through post_ap_ready(). */
    virtual bool ap_start() = 0;

    /** Start DHCP, DNS, and HTTP after the access point has an address. */
    virtual bool serve_start() = 0;

    /** Try the credentials last submitted. Result arrives as post_connected(). */
    virtual bool connect_start() = 0;

    /** Stop serving and take the access point down. Must be safe to repeat. */
    virtual void teardown() = 0;

    /** Put a card on the panel. */
    virtual void show(PortalCard card) = 0;
};

class PortalFsm : public Fsm
{
public:
    enum class State {
        OFF = 0,
        SCANNING,
        AP_STARTING,
        SERVING,
        CONNECTING,
        CONNECTED,
        SHUTDOWN,
    };

    enum class Transition {
        REPEAT = 0, ///< nothing to do; stay put
        CONTINUE,   ///< the ordinary way forward
        SUBMITTED,  ///< somebody posted the form
        JOINED,     ///< the network accepted us
        REFUSED,    ///< it did not, or it never answered
        FAILED,     ///< the radio would not do what it was asked
        STOPPED,    ///< something outside asked the portal to end
    };

    explicit PortalFsm(PortalIo &io);

    /** Enter the portal. Ignored when it is already running. */
    void post_start();

    /** Leave it, from wherever it is. */
    void post_stop();

    /** Post scan completion, including an empty result. */
    void post_scan_done();

    /** The access point reported itself up, or failed to. */
    void post_ap_ready(bool ok);

    /** The form arrived and the credentials have been stored. */
    void post_credentials();

    /** Post the station connection result. */
    void post_connected(bool ok);

    /** Return whether the portal keeps the radio active. */
    bool is_active() const { return get_current_state() != STATE(OFF); }

    /** True once a network has been joined this session. */
    bool is_connected() const { return get_current_state() == STATE(CONNECTED); }

protected:
    int get_fail_state() const override;
    int handle_current_state() override;
    void on_enter_state(int state) override;

private:
    int on_off();
    int on_scanning();
    int on_ap_starting();
    int on_serving();
    int on_connecting();
    int on_connected();
    int on_shutdown();

    PortalIo &_io;

    bool _start_pending = false;
    bool _stop_pending = false;
    bool _scan_done = false;
    bool _ap_pending = false;
    bool _ap_ok = false;
    bool _credentials_pending = false;
    bool _connect_pending = false;
    bool _connect_ok = false;

    /** Selects the refusal card when returning to the form. */
    bool _refused = false;

    /** Prevents SERVING re-entry from restarting the access point. */
    bool _ap_up = false;

    static const Fsm::StateTransition _transitions[];
};

} // namespace kveld
