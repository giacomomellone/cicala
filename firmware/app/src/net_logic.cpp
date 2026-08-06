/*
 * The portal's decisions, and the C++ that renders its pages, behind a C API.
 *
 * The state machine is lib/portal's and knows nothing about Zephyr; PortalIo
 * below is the whole of what it can do, and every one of those six calls lands
 * in portal.c. The one thing that does not is show(), which publishes on
 * chan_service rather than touching the panel — `app` owns the sequence number
 * every card carries, and a second publisher would break the guard that drops a
 * late render.
 */

#include "net_logic.h"

#include "net.h"

#include <string.h>

#include <zephyr/logging/log.h>
#include <zephyr/zbus/zbus.h>

#include "app_logic.h"
#include "channels.h"
#include "portal.h"
#include "portal_dns.hpp"
#include "portal_form.hpp"
#include "portal_fsm.hpp"
#include "portal_page.hpp"

LOG_MODULE_DECLARE(tk_net, LOG_LEVEL_INF);

namespace
{

/** Publishes cards and forwards everything else to portal.c. */
class Io : public tk::PortalIo
{
public:
    bool scan_start() override { return tk_portal_scan_start(); }

    bool ap_start() override { return tk_portal_ap_start(); }

    bool serve_start() override { return tk_portal_serve_start(); }

    bool connect_start() override { return tk_portal_connect_start(); }

    void teardown() override { tk_portal_teardown(); }

    void show(tk::PortalCard card) override
    {
        struct tk_service_msg msg = {};

        const char *text = nullptr;

        switch (card) {
        case tk::PortalCard::SETUP:
            /*
             * The one card that has to carry information rather than a status:
             * a phone cannot join a network nobody has named. The address is
             * there for the case where the captive sheet does not open by
             * itself, which happens often enough to be worth two lines.
             */
            write_setup(msg);
            break;

        case tk::PortalCard::CONNECTING:
            text = "Setup: joining the network...";
            break;

        case tk::PortalCard::CONNECTED:
            text = "Setup: connected. Wi-Fi is ready.";
            break;

        case tk::PortalCard::REFUSED:
            text = "Setup: that did not work. Check the password and try again.";
            break;
        }

        if (text != nullptr) {
            const size_t len = strlen(text);

            msg.len = static_cast<uint16_t>(len);
            memcpy(msg.text, text, len);
        }

        LOG_INF("card: %.*s", (int) msg.len, msg.text);

        (void) zbus_chan_pub(&chan_service, &msg, K_MSEC(100));
    }

private:
    static void write_setup(struct tk_service_msg &msg)
    {
        const char *const prefix = "Setup: join ";
        const char *const middle = " then open 192.168.4.1";
        const char *const ssid = tk_portal_ap_ssid();

        const size_t n = strlen(prefix) + strlen(ssid) + strlen(middle);

        if (n >= sizeof(msg.text)) {
            /* Cannot happen with any sane prefix — the SSID is bounded at 32
             * bytes and the text buffer is 128 — but a truncated instruction is
             * worse than a short one, so say the minimum instead. */
            const char *const fallback = "Setup mode. Open 192.168.4.1";

            msg.len = static_cast<uint16_t>(strlen(fallback));
            memcpy(msg.text, fallback, msg.len);

            return;
        }

        char *at = msg.text;

        at = stpcpy_bounded(at, prefix);
        at = stpcpy_bounded(at, ssid);
        at = stpcpy_bounded(at, middle);

        msg.len = static_cast<uint16_t>(at - msg.text);
    }

    static char *stpcpy_bounded(char *at, const char *s)
    {
        const size_t n = strlen(s);

        memcpy(at, s, n);

        return at + n;
    }
};

Io io;
tk::PortalFsm fsm(io);

} // namespace

void tk_net_post_start(void)
{
    fsm.post_start();
}

void tk_net_post_stop(void)
{
    fsm.post_stop();
}

void tk_net_post_scan_done(void)
{
    fsm.post_scan_done();
}

void tk_net_post_ap_ready(bool ok)
{
    fsm.post_ap_ready(ok);
}

void tk_net_post_credentials(void)
{
    fsm.post_credentials();
}

void tk_net_post_connected(bool ok)
{
    fsm.post_connected(ok);
}

void tk_net_run(void)
{
    /* Bounded rather than while(changed), for the reason tk_app_run() is: a
     * table bug that made two states point at each other would otherwise spin
     * here forever instead of being noticed. */
    for (int i = 0; i < 16; i++) {
        const int before = fsm.get_current_state();

        fsm.run();

        if (fsm.get_current_state() == before) {
            return;
        }
    }

    LOG_ERR("portal state machine did not settle");
}

int tk_net_state(void)
{
    return fsm.get_current_state();
}

bool tk_net_is_active(void)
{
    return fsm.is_active();
}

/* -------------------------------------------------------------- the pages */

int tk_page_setup(char *out, size_t out_size, const struct tk_scan_entry *nets, uint8_t count,
                  const char *language)
{
    /*
     * tk_scan_entry and tk::ScanEntry are the same three fields, but one is the
     * C face and the other is the renderer's, and letting them alias would make
     * a field reordering a silent corruption rather than a compile error. The
     * copy is at most CONFIG_TK_NET_SCAN_MAX entries, once per page.
     */
    tk::ScanEntry entries[CONFIG_TK_NET_SCAN_MAX];

    if (count > CONFIG_TK_NET_SCAN_MAX) {
        count = CONFIG_TK_NET_SCAN_MAX;
    }

    for (uint8_t i = 0; i < count; i++) {
        memcpy(entries[i].ssid, nets[i].ssid, sizeof(entries[i].ssid));
        entries[i].ssid[sizeof(entries[i].ssid) - 1] = '\0';
        entries[i].rssi = nets[i].rssi;
        entries[i].secure = nets[i].secure;
    }

    return tk::page_setup(out, static_cast<uint16_t>(out_size), count > 0 ? entries : nullptr,
                          count, language);
}

int tk_page_saved(char *out, size_t out_size, const char *ssid)
{
    return tk::page_saved(out, static_cast<uint16_t>(out_size), ssid);
}

int tk_page_status(char *out, size_t out_size, const char *ap_ssid, const char *saved_ssid,
                   bool connected, const char *station_ip)
{
    tk::PortalStatus status = {};

    char corpus_version[32] = {};
    uint16_t corpus_count = 0;

    tk_app_corpus(corpus_version, sizeof(corpus_version), &corpus_count);

    status.ap_ssid = ap_ssid;
    /* The board rather than a version string: nothing in this tree stamps a
     * firmware version yet, and inventing one on a status page is worse than
     * reporting the thing that is actually known. */
    status.board = CONFIG_BOARD_TARGET;
    status.corpus_language = CONFIG_TK_CORPUS_LANGUAGE;
    status.corpus_version = corpus_version;
    status.corpus_count = corpus_count;
    status.saved_ssid = saved_ssid;
    status.station_connected = connected;
    status.station_ip = station_ip;

    return tk::page_status(out, static_cast<uint16_t>(out_size), status);
}

/* --------------------------------------------------------------- the wire */

int tk_form_field(const char *body, uint16_t len, const char *key, char *out, uint16_t out_size)
{
    return tk::form_field(body, len, key, out, out_size);
}

uint16_t tk_dns_hijack(const uint8_t *query, uint16_t len, uint32_t addr, uint8_t *out,
                       uint16_t out_size)
{
    return tk::dns_hijack(query, len, addr, out, out_size);
}
