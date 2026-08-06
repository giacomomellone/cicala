/*
 * The radio, the sockets and the pages, behind PortalIo's six calls.
 *
 * C rather than C++ for the reason the rest of the glue is: HTTP_SERVICE_DEFINE
 * and HTTP_RESOURCE_DEFINE expand to out-of-order designated initializers,
 * which C allows and C++17 rejects.
 *
 * ## The order things come up in, and why it is not free to change
 *
 * The access point has to report itself enabled before the interface carries
 * 192.168.4.1, and a socket bound to an address the interface does not have
 * fails with -EADDRNOTAVAIL. So the address, the DHCP server, the DNS socket
 * and the HTTP server all wait for AP_ENABLE_RESULT, which is why raising the
 * access point and serving on it are two calls rather than one.
 *
 * The gateway is set before the address, because the DHCP server encodes the
 * interface's own gateway into option 3 and reads it at start.
 *
 * ## The captive sheet
 *
 * A phone decides it is behind a portal by fetching a known URL and not getting
 * what it expects. Both halves have to be wrong for it to notice: DNS has to
 * resolve that host to us, which the DHCP server's option 6 arranges and the
 * DNS responder answers, and the fetch has to come back as something other than
 * the expected 204 or body, which the fallback resource does.
 */

#include "portal.h"

#include <stdio.h>
#include <string.h>

#include <zephyr/drivers/hwinfo.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/dhcpv4_server.h>
#include <zephyr/net/http/server.h>
#include <zephyr/net/http/service.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/wifi_credentials.h>
#include <zephyr/net/wifi_mgmt.h>

#include "net.h"
#include "net_logic.h"

LOG_MODULE_DECLARE(tk_net, LOG_LEVEL_INF);

/* The address the portal lives at. Also the DNS answer and the DHCP router. */
#define AP_ADDR "192.168.4.1"
#define AP_NETMASK "255.255.255.0"

/* The same address the DNS responder hands out, as the responder wants it. */
#define AP_ADDR_HOST_ORDER 0xC0A80401U

/* Pool base: .11 upwards, leaving room under it for anything static. */
#define AP_POOL_OFFSET 10

#define DNS_PORT 53
#define HTTP_PORT 80

/* One page, built in full before any of it is sent. The setup page with a full
 * scan list is the biggest, at roughly 2 KB. */
#define PAGE_BUF_SIZE 4096

#define DNS_STACK_SIZE 2048
#define DNS_PRIORITY 9

static struct net_if *ap_iface;
static struct net_if *sta_iface;

static char ap_ssid[TK_SSID_MAX];

/* The scan list, written by the net_mgmt callback and read by the HTTP server
 * thread. Two threads, so a lock. */
static struct tk_scan_entry scan_results[CONFIG_TK_NET_SCAN_MAX];
static uint8_t scan_count;
static K_MUTEX_DEFINE(scan_lock);

/* What the form last submitted. Written by the HTTP thread, read by `net`. */
static char pending_ssid[TK_SSID_MAX];
static bool station_connected;
static char station_ip[NET_IPV4_ADDR_LEN];

static bool serving;
static int dns_sock = -1;

static uint8_t page_buf[PAGE_BUF_SIZE];

/* ---------------------------------------------------------------- helpers */

int tk_portal_init(void)
{
    uint8_t id[8] = {0};

    sta_iface = net_if_get_wifi_sta();
    ap_iface = net_if_get_wifi_sap();

    if (sta_iface == NULL || ap_iface == NULL) {
        LOG_ERR("no Wi-Fi interfaces");
        return -ENODEV;
    }

    /*
     * Both getters fall back to the first Wi-Fi interface when no manager has
     * registered one of that type, so a missing second devicetree node gives
     * two pointers to the same interface and nothing says so. The DHCP server
     * would then bind to the station and the portal would be unreachable.
     */
    if (ap_iface == sta_iface) {
        LOG_ERR("the AP and station interfaces are the same — is net.overlay merged?");
        return -ENODEV;
    }

    /* A suffix so two devices being set up on one table do not both announce
     * the same network. The low two bytes of the chip id are enough. */
    const ssize_t n = hwinfo_get_device_id(id, sizeof(id));

    if (n >= 2) {
        (void) snprintf(ap_ssid, sizeof(ap_ssid), "%s-%02X%02X", CONFIG_TK_PORTAL_SSID_PREFIX,
                        id[n - 2], id[n - 1]);
    } else {
        (void) snprintf(ap_ssid, sizeof(ap_ssid), "%s", CONFIG_TK_PORTAL_SSID_PREFIX);
    }

    LOG_INF("setup network will be %s", ap_ssid);

    return 0;
}

const char *tk_portal_ap_ssid(void)
{
    return ap_ssid;
}

bool tk_portal_station_connected(void)
{
    return station_connected;
}

void tk_portal_set_station_connected(bool connected)
{
    station_connected = connected;

    station_ip[0] = '\0';

    if (!connected || sta_iface == NULL) {
        return;
    }

    struct net_if_ipv4 *ipv4 = sta_iface->config.ip.ipv4;

    if (ipv4 == NULL) {
        return;
    }

    for (int i = 0; i < NET_IF_MAX_IPV4_ADDR; i++) {
        if (ipv4->unicast[i].ipv4.is_used) {
            (void) net_addr_ntop(NET_AF_INET, &ipv4->unicast[i].ipv4.address.in_addr, station_ip,
                                 sizeof(station_ip));
            break;
        }
    }
}

/* ------------------------------------------------------------------- scan */

void tk_portal_scan_reset(void)
{
    k_mutex_lock(&scan_lock, K_FOREVER);
    scan_count = 0;
    k_mutex_unlock(&scan_lock);
}

void tk_portal_scan_add(const char *ssid, uint8_t len, int8_t rssi, bool secure)
{
    if (len == 0 || len >= TK_SSID_MAX) {
        /* A zero length is a hidden network, which cannot be offered as a
         * choice — the page's typed field is what covers those. */
        return;
    }

    k_mutex_lock(&scan_lock, K_FOREVER);

    /* Networks repeat across bands and across access points on one network, and
     * a list with the same name four times is worse than a short one. */
    for (uint8_t i = 0; i < scan_count; i++) {
        if (strncmp(scan_results[i].ssid, ssid, len) == 0 && scan_results[i].ssid[len] == '\0') {
            if (rssi > scan_results[i].rssi) {
                scan_results[i].rssi = rssi;
            }

            k_mutex_unlock(&scan_lock);
            return;
        }
    }

    if (scan_count < ARRAY_SIZE(scan_results)) {
        memcpy(scan_results[scan_count].ssid, ssid, len);
        scan_results[scan_count].ssid[len] = '\0';
        scan_results[scan_count].rssi = rssi;
        scan_results[scan_count].secure = secure;
        scan_count++;
    }

    k_mutex_unlock(&scan_lock);
}

bool tk_portal_scan_start(void)
{
    struct wifi_scan_params params = {
        .scan_type = WIFI_SCAN_TYPE_ACTIVE,
        .bands = BIT(WIFI_FREQ_BAND_2_4_GHZ),
        .max_bss_cnt = CONFIG_TK_NET_SCAN_MAX,
    };

    tk_portal_scan_reset();

    if (sta_iface == NULL) {
        return false;
    }

    /* On the station interface only. The driver keeps the scan callback in the
     * station device's data and the completion handler reads it back from
     * there unconditionally, so a scan issued on the access point interface
     * ends in a null dereference inside the driver. */
    const int err = net_mgmt(NET_REQUEST_WIFI_SCAN, sta_iface, &params, sizeof(params));

    if (err != 0) {
        LOG_WRN("scan request failed: %d", err);
        return false;
    }

    return true;
}

/* --------------------------------------------------------- access point */

bool tk_portal_ap_start(void)
{
    struct wifi_connect_req_params config = {
        .ssid = (const uint8_t *) ap_ssid,
        .ssid_length = strlen(ap_ssid),
        .psk = NULL,
        .psk_length = 0,
        .channel = WIFI_CHANNEL_ANY,
        .band = WIFI_FREQ_BAND_2_4_GHZ,
        /*
         * Open, and that is a decision rather than an omission. A WPA2 setup
         * network needs a passphrase the user has to be told, and the only
         * place to tell them is the panel or the case. Open, plus a physical
         * gesture to start it, plus a window that closes on its own, is the
         * same posture as most consumer setup flows. Nothing secret is served:
         * the status page names the saved network but never its password.
         */
        .security = WIFI_SECURITY_TYPE_NONE,
    };

    if (ap_iface == NULL) {
        return false;
    }

    if (!net_if_is_admin_up(ap_iface)) {
        (void) net_if_up(ap_iface);
    }

    const int err = net_mgmt(NET_REQUEST_WIFI_AP_ENABLE, ap_iface, &config, sizeof(config));

    if (err != 0) {
        LOG_ERR("could not enable the access point: %d", err);
        return false;
    }

    return true;
}

/* --------------------------------------------------------------- serving */

/** Answer every A query with our own address, for as long as the portal runs. */
static void dns_thread(void *p1, void *p2, void *p3)
{
    ARG_UNUSED(p1);
    ARG_UNUSED(p2);
    ARG_UNUSED(p3);

    static uint8_t query[TK_DNS_BUF_SIZE];
    static uint8_t reply[TK_DNS_BUF_SIZE];

    while (true) {
        const int sock = dns_sock;

        if (sock < 0) {
            k_sleep(K_MSEC(200));
            continue;
        }

        struct sockaddr_in from = {0};
        socklen_t from_len = sizeof(from);

        const ssize_t n =
            zsock_recvfrom(sock, query, sizeof(query), 0, (struct sockaddr *) &from, &from_len);

        if (n <= 0) {
            /* The socket was closed under us at teardown, or the receive timed
             * out so the loop can notice that it was. */
            continue;
        }

        const uint16_t len =
            tk_dns_hijack(query, (uint16_t) n, AP_ADDR_HOST_ORDER, reply, sizeof(reply));

        if (len == 0) {
            continue;
        }

        (void) zsock_sendto(sock, reply, len, 0, (struct sockaddr *) &from, from_len);
    }
}

K_THREAD_DEFINE(tk_dns_thread, DNS_STACK_SIZE, dns_thread, NULL, NULL, NULL, DNS_PRIORITY, 0, 0);

static int dns_start(void)
{
    struct sockaddr_in addr = {
        .sin_family = NET_AF_INET,
        .sin_port = htons(DNS_PORT),
    };

    const int sock = zsock_socket(NET_AF_INET, SOCK_DGRAM, IPPROTO_UDP);

    if (sock < 0) {
        LOG_ERR("could not open the DNS socket: %d", errno);
        return -errno;
    }

    /* Bound to the wildcard rather than to the AP address: the station may not
     * have an address yet, and a wildcard bind is what keeps the responder
     * working across the moment the station joins and the interfaces change. */
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (zsock_bind(sock, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
        LOG_ERR("could not bind the DNS socket: %d", errno);
        (void) zsock_close(sock);
        return -errno;
    }

    /* So the receive returns periodically and the thread can see a teardown. */
    const struct zsock_timeval timeout = {.tv_sec = 1, .tv_usec = 0};

    (void) zsock_setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

    dns_sock = sock;

    return 0;
}

static void dns_stop(void)
{
    const int sock = dns_sock;

    dns_sock = -1;

    if (sock >= 0) {
        (void) zsock_close(sock);
    }
}

bool tk_portal_serve_start(void)
{
    struct net_in_addr addr;
    struct net_in_addr mask;

    if (ap_iface == NULL) {
        return false;
    }

    (void) net_addr_pton(NET_AF_INET, AP_ADDR, &addr);
    (void) net_addr_pton(NET_AF_INET, AP_NETMASK, &mask);

    /* Gateway first: the DHCP server reads the interface's own gateway when it
     * starts, and encodes it as the router option. */
    net_if_ipv4_set_gw(ap_iface, &addr);

    if (net_if_ipv4_addr_add(ap_iface, &addr, NET_ADDR_MANUAL, 0) == NULL) {
        LOG_ERR("could not give the access point an address");
        return false;
    }

    (void) net_if_ipv4_set_netmask_by_addr(ap_iface, &addr, &mask);

    struct net_in_addr pool = addr;

    pool.s4_addr[3] += AP_POOL_OFFSET;

    int err = net_dhcpv4_server_start(ap_iface, &pool);

    if (err != 0 && err != -EALREADY) {
        LOG_ERR("could not start the DHCP server: %d", err);
        return false;
    }

    err = dns_start();

    if (err != 0) {
        return false;
    }

    err = http_server_start();

    if (err != 0) {
        LOG_ERR("could not start the HTTP server: %d", err);
        dns_stop();
        return false;
    }

    serving = true;

    LOG_INF("portal serving on http://%s", AP_ADDR);

    return true;
}

void tk_portal_teardown(void)
{
    struct net_in_addr addr;

    if (!serving && ap_iface == NULL) {
        return;
    }

    if (serving) {
        (void) http_server_stop();
        dns_stop();

        if (ap_iface != NULL) {
            (void) net_dhcpv4_server_stop(ap_iface);
        }

        serving = false;
    }

    if (ap_iface != NULL) {
        (void) net_mgmt(NET_REQUEST_WIFI_AP_DISABLE, ap_iface, NULL, 0);

        (void) net_addr_pton(NET_AF_INET, AP_ADDR, &addr);
        (void) net_if_ipv4_addr_rm(ap_iface, &addr);
    }

    LOG_INF("portal down");
}

/* ------------------------------------------------------------- connecting */

/** Join `ssid` using whatever the credential store holds for it. */
static bool connect_to(const char *ssid)
{
    struct wifi_credentials_personal creds = {0};
    struct wifi_connect_req_params params = {0};

    const size_t ssid_len = strlen(ssid);

    if (ssid_len == 0 || sta_iface == NULL) {
        return false;
    }

    const int err = wifi_credentials_get_by_ssid_personal_struct(ssid, ssid_len, &creds);

    if (err != 0) {
        LOG_ERR("no stored credentials for %s: %d", ssid, err);
        return false;
    }

    params.ssid = creds.header.ssid;
    params.ssid_length = creds.header.ssid_len;
    params.security = creds.header.type;
    params.channel = WIFI_CHANNEL_ANY;
    params.band = WIFI_FREQ_BAND_2_4_GHZ;
    params.timeout = SYS_FOREVER_MS;

    if (creds.header.type != WIFI_SECURITY_TYPE_NONE) {
        params.psk = creds.password;
        params.psk_length = creds.password_len;
    }

    if (!net_if_is_admin_up(sta_iface)) {
        (void) net_if_up(sta_iface);
    }

    /*
     * The driver refuses with -EIO until its station task has reported itself
     * started, which happens asynchronously after the interface comes up. A few
     * attempts a quarter-second apart cover it; the alternative in the upstream
     * sample is an unconditional five-second sleep.
     */
    for (int attempt = 0; attempt < 20; attempt++) {
        const int ret = net_mgmt(NET_REQUEST_WIFI_CONNECT, sta_iface, &params, sizeof(params));

        if (ret == 0) {
            return true;
        }

        if (ret != -EIO && ret != -EBUSY) {
            LOG_ERR("could not ask to join %s: %d", ssid, ret);
            return false;
        }

        k_sleep(K_MSEC(250));
    }

    LOG_ERR("the station never came up, so %s was never tried", ssid);

    return false;
}

bool tk_portal_connect_start(void)
{
    return connect_to(pending_ssid);
}

/** Collects the first stored SSID, for the connect-at-boot path. */
static void first_ssid(void *arg, const char *ssid, size_t len)
{
    char *out = arg;

    if (out[0] != '\0' || len == 0 || len >= TK_SSID_MAX) {
        return;
    }

    memcpy(out, ssid, len);
    out[len] = '\0';
}

bool tk_portal_connect_stored(void)
{
    char ssid[TK_SSID_MAX] = {0};

    if (wifi_credentials_is_empty()) {
        LOG_INF("no stored network — the compiled-in corpus is all this device needs");
        return false;
    }

    wifi_credentials_for_each_ssid(first_ssid, ssid);

    if (ssid[0] == '\0') {
        return false;
    }

    LOG_INF("joining %s", ssid);

    (void) strncpy(pending_ssid, ssid, sizeof(pending_ssid) - 1);

    return connect_to(ssid);
}

/* ------------------------------------------------------------------- HTTP */

/** Send one rendered page, or a 500 if it would not fit the buffer. */
static int send_page(const char *what, int rendered, struct http_response_ctx *response)
{
    LOG_INF("GET %s: %d bytes", what, rendered);

    if (rendered < 0) {
        response->status = HTTP_500_INTERNAL_SERVER_ERROR;
        response->final_chunk = true;
        return 0;
    }

    response->status = HTTP_200_OK;
    response->body = page_buf;
    response->body_len = (size_t) rendered;
    response->final_chunk = true;

    return 0;
}

static int setup_handler(struct http_client_ctx *client, enum http_transaction_status status,
                         const struct http_request_ctx *request, struct http_response_ctx *response,
                         void *user_data)
{
    ARG_UNUSED(client);
    ARG_UNUSED(request);
    ARG_UNUSED(user_data);

    if (status != HTTP_SERVER_REQUEST_DATA_FINAL) {
        return 0;
    }

    k_mutex_lock(&scan_lock, K_FOREVER);
    const int n = tk_page_setup((char *) page_buf, sizeof(page_buf), scan_results, scan_count,
                                CONFIG_TK_CORPUS_LANGUAGE);
    k_mutex_unlock(&scan_lock);

    return send_page("/", n, response);
}

static int status_handler(struct http_client_ctx *client, enum http_transaction_status status,
                          const struct http_request_ctx *request,
                          struct http_response_ctx *response, void *user_data)
{
    ARG_UNUSED(client);
    ARG_UNUSED(request);
    ARG_UNUSED(user_data);

    if (status != HTTP_SERVER_REQUEST_DATA_FINAL) {
        return 0;
    }

    const int n = tk_page_status((char *) page_buf, sizeof(page_buf), ap_ssid,
                                 pending_ssid[0] != '\0' ? pending_ssid : NULL, station_connected,
                                 station_ip);

    return send_page("/status", n, response);
}

/*
 * A form body arrives as a slice of the server's client buffer: not
 * NUL-terminated, and split across calls when it does not fit. It is
 * accumulated here and parsed once, at DATA_FINAL.
 */
static char form_body[512];
static size_t form_len;

static int save_handler(struct http_client_ctx *client, enum http_transaction_status status,
                        const struct http_request_ctx *request, struct http_response_ctx *response,
                        void *user_data)
{
    ARG_UNUSED(client);
    ARG_UNUSED(user_data);

    char ssid[TK_SSID_MAX] = {0};
    char psk[TK_PSK_MAX] = {0};

    if (status == HTTP_SERVER_TRANSACTION_ABORTED || status == HTTP_SERVER_TRANSACTION_COMPLETE) {
        form_len = 0;
        return 0;
    }

    if (request != NULL && request->data_len > 0) {
        const size_t room = sizeof(form_body) - form_len;
        const size_t take = request->data_len < room ? request->data_len : room;

        memcpy(&form_body[form_len], request->data, take);
        form_len += take;
    }

    if (status != HTTP_SERVER_REQUEST_DATA_FINAL) {
        /* Nothing may be sent before the body is complete. An empty response
         * context is how the server is told there is more to come. */
        return 0;
    }

    const int ssid_len = tk_form_field(form_body, (uint16_t) form_len, "ssid", ssid, sizeof(ssid));
    const int psk_len = tk_form_field(form_body, (uint16_t) form_len, "psk", psk, sizeof(psk));

    form_len = 0;

    if (ssid_len <= 0) {
        LOG_WRN("a form arrived with no network name");
        response->status = HTTP_400_BAD_REQUEST;
        response->final_chunk = true;
        return 0;
    }

    const enum wifi_security_type type =
        psk_len > 0 ? WIFI_SECURITY_TYPE_PSK : WIFI_SECURITY_TYPE_NONE;

    /*
     * Written before the station is asked to join, and deliberately not from
     * inside the connect path: an NVS write disables the instruction cache on
     * this SoC, which is a bad thing to do while the Wi-Fi task is running out
     * of it.
     */
    const int err = wifi_credentials_set_personal(ssid, (size_t) ssid_len, type, NULL, 0, psk,
                                                  (size_t) psk_len, 0, 0, 0);

    if (err != 0) {
        LOG_ERR("could not store the credentials: %d", err);
        response->status = HTTP_507_INSUFFICIENT_STORAGE;
        response->final_chunk = true;
        return 0;
    }

    (void) strncpy(pending_ssid, ssid, sizeof(pending_ssid) - 1);

    const int n = tk_page_saved((char *) page_buf, sizeof(page_buf), ssid);

    /*
     * The page goes out first and the join is left to `net`. Joining moves the
     * access point onto the home network's channel, which drops the phone —
     * responding afterwards would mean responding to a browser that is no
     * longer there.
     */
    tk_net_notify_credentials();

    return send_page("/save", n, response);
}

/**
 * Everything else.
 *
 * A phone probing for a portal asks for a URL on some other host and expects a
 * 204 or a particular body. A redirect here is what turns that into a sign-in
 * sheet.
 */
static int catchall_handler(struct http_client_ctx *client, enum http_transaction_status status,
                            const struct http_request_ctx *request,
                            struct http_response_ctx *response, void *user_data)
{
    ARG_UNUSED(client);
    ARG_UNUSED(request);
    ARG_UNUSED(user_data);

    static const struct http_header location[] = {
        {.name = "Location", .value = "http://" AP_ADDR "/"},
    };

    if (status != HTTP_SERVER_REQUEST_DATA_FINAL) {
        return 0;
    }

    LOG_INF("redirecting a probe to the portal");

    response->status = HTTP_302_FOUND;
    response->headers = location;
    response->header_count = 1;
    response->final_chunk = true;

    return 0;
}

/* Each resource gets its own detail struct. They are not shared, because the
 * server rejects a second client with 409 while one holds a resource, and a
 * phone opening a captive sheet fires several requests at once. */
static struct http_resource_detail_dynamic setup_detail = {
    .common = {.type = HTTP_RESOURCE_TYPE_DYNAMIC,
               .bitmask_of_supported_http_methods = BIT(HTTP_GET),
               .content_type = "text/html"},
    .cb = setup_handler,
};

static struct http_resource_detail_dynamic status_detail = {
    .common = {.type = HTTP_RESOURCE_TYPE_DYNAMIC,
               .bitmask_of_supported_http_methods = BIT(HTTP_GET),
               .content_type = "text/html"},
    .cb = status_handler,
};

static struct http_resource_detail_dynamic save_detail = {
    .common = {.type = HTTP_RESOURCE_TYPE_DYNAMIC,
               .bitmask_of_supported_http_methods = BIT(HTTP_POST),
               .content_type = "text/html"},
    .cb = save_handler,
};

static struct http_resource_detail_dynamic catchall_detail = {
    .common = {.type = HTTP_RESOURCE_TYPE_DYNAMIC,
               .bitmask_of_supported_http_methods = BIT(HTTP_GET) | BIT(HTTP_POST),
               .content_type = "text/html"},
    .cb = catchall_handler,
};

static uint16_t http_port = HTTP_PORT;

/*
 * Bound to every address rather than to 192.168.4.1, so the service survives
 * the station joining a network and the interfaces changing under it. The
 * fallback resource is what makes a phone's captive-detection probe — which
 * asks for a URL on somebody else's host — reach the redirect.
 */
HTTP_SERVICE_DEFINE(tk_portal, NULL, &http_port, CONFIG_HTTP_SERVER_MAX_CLIENTS, 4, NULL,
                    &catchall_detail.common, NULL);

HTTP_RESOURCE_DEFINE(setup_resource, tk_portal, "/", &setup_detail);
HTTP_RESOURCE_DEFINE(status_resource, tk_portal, "/status", &status_detail);
HTTP_RESOURCE_DEFINE(save_resource, tk_portal, "/save", &save_detail);
