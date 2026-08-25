/* The portal's decisions, and the C++ that renders its pages, behind a C API. */

#include "net_logic.h"

#include "net.h"

#include <string.h>

#include <app_version.h>
#include <zephyr/logging/log.h>
#include <zephyr/zbus/zbus.h>

#include "app_logic.h"
#include "channels.h"
#include "language.h"
#include "portal.h"
#include "portal_dns.hpp"
#include "portal_form.hpp"
#include "portal_fsm.hpp"
#include "portal_page.hpp"

LOG_MODULE_DECLARE(cicala_net, LOG_LEVEL_INF);

namespace
{

/* Publishes cards and forwards everything else to portal.c. */
class Io : public cicala::PortalIo
{
public:
    bool scan_start() override { return cicala_portal_scan_start(); }

    bool ap_start() override { return cicala_portal_ap_start(); }

    bool serve_start() override { return cicala_portal_serve_start(); }

    bool connect_start() override { return cicala_portal_connect_start(); }

    void teardown() override { cicala_portal_teardown(); }

    void show(cicala::PortalCard card) override
    {
        struct cicala_service_msg msg = {};

        const char *text = nullptr;

        switch (card) {
        case cicala::PortalCard::SETUP:
            /* The setup card includes the access-point name and session password. */
            write_setup(msg);
            break;

        case cicala::PortalCard::CONNECTING:
            text = "Setup: joining the network...";
            break;

        case cicala::PortalCard::CONNECTED:
            text = "Setup: connected. Wi-Fi is ready.";
            break;

        case cicala::PortalCard::REFUSED:
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
    static void write_setup(struct cicala_service_msg &msg)
    {
        const char *const prefix = "Setup: join \"";
        const char *const middle = "\"\npassword: \"";
        const char *const suffix = "\"\nthen open 192.168.4.1";
        const char *const ssid = cicala_portal_ap_ssid();
        const char *const password = cicala_portal_ap_password();

        const size_t n =
            strlen(prefix) + strlen(ssid) + strlen(middle) + strlen(password) + strlen(suffix);

        if (n >= sizeof(msg.text)) {
            /* Fall back to the SSID if the full instruction does not fit. */
            const char *const fallback = "Setup mode. Open 192.168.4.1";

            msg.len = static_cast<uint16_t>(strlen(fallback));
            memcpy(msg.text, fallback, msg.len);

            return;
        }

        char *at = msg.text;

        at = stpcpy_bounded(at, prefix);
        at = stpcpy_bounded(at, ssid);
        at = stpcpy_bounded(at, middle);
        at = stpcpy_bounded(at, password);
        at = stpcpy_bounded(at, suffix);

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
cicala::PortalFsm fsm(io);

} // namespace

void cicala_net_post_start(void)
{
    fsm.post_start();
}

void cicala_net_post_stop(void)
{
    fsm.post_stop();
}

void cicala_net_post_scan_done(void)
{
    fsm.post_scan_done();
}

void cicala_net_post_ap_ready(bool ok)
{
    fsm.post_ap_ready(ok);
}

void cicala_net_post_credentials(void)
{
    fsm.post_credentials();
}

void cicala_net_post_connected(bool ok)
{
    fsm.post_connected(ok);
}

void cicala_net_run(void)
{
    /* Bound one event to the number of portal states. */
    for (int i = 0; i < 16; i++) {
        const int before = fsm.get_current_state();

        fsm.run();

        if (fsm.get_current_state() == before) {
            return;
        }
    }

    LOG_ERR("portal state machine did not settle");
}

int cicala_net_state(void)
{
    return fsm.get_current_state();
}

bool cicala_net_portal_active(void)
{
    return fsm.is_active();
}

/* -------------------------------------------------------------- the pages */

int cicala_page_setup(char *out, size_t out_size, const struct cicala_scan_entry *nets,
                      uint8_t count, const char *language)
{
    /* Copy across the C boundary so field layout is not an ABI contract. */
    cicala::ScanEntry entries[CONFIG_CICALA_NET_SCAN_MAX];

    if (count > CONFIG_CICALA_NET_SCAN_MAX) {
        count = CONFIG_CICALA_NET_SCAN_MAX;
    }

    for (uint8_t i = 0; i < count; i++) {
        memcpy(entries[i].ssid, nets[i].ssid, sizeof(entries[i].ssid));
        entries[i].ssid[sizeof(entries[i].ssid) - 1] = '\0';
        entries[i].rssi = nets[i].rssi;
        entries[i].secure = nets[i].secure;
    }

    return cicala::page_setup(out, static_cast<uint16_t>(out_size), count > 0 ? entries : nullptr,
                              count, language);
}

int cicala_page_saved(char *out, size_t out_size, const char *ssid)
{
    return cicala::page_saved(out, static_cast<uint16_t>(out_size), ssid);
}

int cicala_page_notice(char *out, size_t out_size, const char *heading, const char *body)
{
    return cicala::page_notice(out, static_cast<uint16_t>(out_size), heading, body);
}

int cicala_page_forget_confirm(char *out, size_t out_size)
{
    return cicala::page_forget_confirm(out, static_cast<uint16_t>(out_size));
}

int cicala_page_status(char *out, size_t out_size, const char *ap_ssid, const char *saved_ssid,
                       bool connected, const char *station_ip, const char *connection_error,
                       const char *sync_result, uint32_t window_remaining_s)
{
    cicala::PortalStatus status = {};

    char corpus_version[32] = {};
    uint16_t corpus_count = 0;

    cicala_app_corpus(corpus_version, sizeof(corpus_version), &corpus_count);

    status.ap_ssid = ap_ssid;
    status.board = CONFIG_BOARD_TARGET;
    status.firmware_version = APP_VERSION_STRING;
    status.corpus_language = cicala_language();
    status.corpus_version = corpus_version;
    status.corpus_count = corpus_count;
    status.saved_ssid = saved_ssid;
    status.station_connected = connected;
    status.station_ip = station_ip;
    status.connection_error = connection_error;
    status.sync_result = sync_result;
    status.window_remaining_s = window_remaining_s;

    return cicala::page_status(out, static_cast<uint16_t>(out_size), status);
}

/* --------------------------------------------------------------- the wire */

int cicala_form_field(const char *body, uint16_t len, const char *key, char *out, uint16_t out_size)
{
    return cicala::form_field(body, len, key, out, out_size);
}

uint16_t cicala_dns_hijack(const uint8_t *query, uint16_t len, uint32_t addr, uint8_t *out,
                           uint16_t out_size)
{
    return cicala::dns_hijack(query, len, addr, out, out_size);
}
