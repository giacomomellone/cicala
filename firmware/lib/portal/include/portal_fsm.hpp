/*
 * The setup portal's state machine — the only thing in `net` that decides
 * anything. Everything else opens sockets or reports.
 *
 * The transition table in portal_fsm.cpp is the specification; the diagram in
 * docs/firmware_architecture.md is the same table drawn. This class holds no
 * Zephyr headers and no sockets: events are pushed in with post_*(), effects go
 * out through PortalIo. That is what lets the suite drive a five-minute portal
 * window in microseconds, with no radio and no phone.
 *
 * ## Why the scan happens before the access point comes up
 *
 * The ESP32-S3 has one radio. A scan hops every channel in the band for a few
 * seconds, and a SoftAP sits on one — so scanning with a phone already
 * associated stalls it, and can drop the association outright. Scanning first
 * and serving the cached list costs nothing: the list is a few seconds old by
 * the time anybody reads it, and the networks in a room do not move.
 *
 * ## Why joining a network is the last thing that happens
 *
 * In AP+STA mode the SoftAP is forced onto whatever channel the station lands
 * on. Joining therefore knocks the phone off the setup network, which means the
 * portal cannot report the result to the browser that asked for it. The panel
 * does that instead, which is what the service card is for.
 */

#pragma once

#include <stdint.h>

#include "fsm.hpp"

namespace tk
{

#ifdef CONFIG_TK_PORTAL_WINDOW_MS
constexpr int64_t kPortalWindowMs = CONFIG_TK_PORTAL_WINDOW_MS;
#else
constexpr int64_t kPortalWindowMs = 300000;
#endif

#ifdef CONFIG_TK_PORTAL_SCAN_MS
constexpr int64_t kPortalScanMs = CONFIG_TK_PORTAL_SCAN_MS;
#else
constexpr int64_t kPortalScanMs = 8000;
#endif

#ifdef CONFIG_TK_NET_CONNECT_TIMEOUT_MS
constexpr int64_t kConnectTimeoutMs = CONFIG_TK_NET_CONNECT_TIMEOUT_MS;
#else
constexpr int64_t kConnectTimeoutMs = 20000;
#endif

/** What the panel should be saying. The tabletop face has no other states. */
enum class PortalCard {
    /** Join this network, open this address. */
    SETUP,
    /** Credentials taken, trying them. */
    CONNECTING,
    /** It worked. */
    CONNECTED,
    /** It did not — wrong password, or the network was not there. */
    REFUSED,
};

/** Everything the portal can do to the world outside itself. */
class PortalIo
{
public:
    virtual ~PortalIo() = default;

    /** Begin a scan. Its results arrive later as post_scan_done(). */
    virtual bool scan_start() = 0;

    /** Bring the SoftAP up. The result arrives as post_ap_ready(). */
    virtual bool ap_start() = 0;

    /**
     * Give the access point an address, a DHCP server, DNS and HTTP.
     *
     * Separate from ap_start() because it can only run once the interface has
     * an address, which is only true after the AP reports itself enabled.
     */
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

    /*
     * Named for what happened, not for where it goes — the table owns the
     * destinations. SUBMITTED, JOINED and REFUSED exist because SERVING and
     * CONNECTING each have more than one way out.
     */
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

    /** The scan finished, with results or without. */
    void post_scan_done();

    /** The access point reported itself up, or failed to. */
    void post_ap_ready(bool ok);

    /** The form arrived and the credentials have been stored. */
    void post_credentials();

    /** The station either joined or did not. */
    void post_connected(bool ok);

    /** True while anything is on air. The sleep path reads this. */
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

    /** The last attempt was refused, so the card says so on the way back. */
    bool _refused = false;

    /** The access point is on air; re-entering SERVING must not restart it. */
    bool _ap_up = false;

    static const Fsm::StateTransition _transitions[];
};

} // namespace tk
