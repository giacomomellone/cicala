/* The radio, the sockets and the pages, behind PortalIo's six calls. */

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
#include <zephyr/random/random.h>

#include <zephyr/zbus/zbus.h>

#include "channels.h"
#include "language.h"
#include "net.h"
#include "net_logic.h"

LOG_MODULE_DECLARE(kveld_net, LOG_LEVEL_INF);

/* The address the portal lives at. */
#define AP_ADDR "192.168.4.1"
#define AP_NETMASK "255.255.255.0"

/* The same address the DNS responder hands out, as the responder wants it. */
#define AP_ADDR_HOST_ORDER 0xC0A80401U

/* Pool base: .11 upwards, leaving room under it for anything static. */
#define AP_POOL_OFFSET 10

#define DNS_PORT 53
#define HTTP_PORT 80

#define AP_PASSWORD_MAX 32

/* One page, built in full before any of it is sent. */
#define PAGE_BUF_SIZE 4096

#define DNS_STACK_SIZE 2048
#define DNS_PRIORITY 9

static struct net_if *ap_iface;
static struct net_if *sta_iface;

static char ap_ssid[KVELD_SSID_MAX];
static char ap_password[AP_PASSWORD_MAX];

static const char *const ap_password_words[] = {
    "apple",   "beacon",  "birch",  "blue",   "cabin",  "cactus", "cedar",  "cloud",
    "comet",   "copper",  "dawn",   "delta",  "dove",   "ember",  "fern",   "flame",
    "forest",  "frost",   "glade",  "gold",   "harbor", "hazel",  "honey",  "island",
    "jade",    "juniper", "kite",   "lilac",  "maple",  "meadow", "mint",   "moon",
    "moss",    "north",   "ocean",  "olive",  "orbit",  "pebble", "pine",   "plum",
    "pond",    "quartz",  "rain",   "raven",  "river",  "robin",  "rose",   "sage",
    "sand",    "shore",   "silver", "sky",    "spruce", "stone",  "sun",    "swift",
    "thistle", "tulip",   "valley", "violet", "wave",   "willow", "winter", "wren",
};

/* The scan list, written by the net_mgmt callback and read by the HTTP server thread. */
static struct kveld_scan_entry scan_results[CONFIG_KVELD_NET_SCAN_MAX];
static uint8_t scan_count;
static K_MUTEX_DEFINE(scan_lock);

/* What the form last submitted. */
static char pending_ssid[KVELD_SSID_MAX];
static bool station_connected;
static char station_ip[NET_IPV4_ADDR_LEN];
static char connection_error[48];
static char sync_result[80];
static int64_t portal_started_ms;
static K_MUTEX_DEFINE(portal_state_lock);

static bool serving;
static int dns_sock = -1;

static uint8_t page_buf[PAGE_BUF_SIZE];

/* ---------------------------------------------------------------- helpers */

int kveld_portal_init(void)
{
    uint8_t id[8] = {0};

    sta_iface = net_if_get_wifi_sta();
    ap_iface = net_if_get_wifi_sap();

    if (sta_iface == NULL || ap_iface == NULL) {
        LOG_ERR("no Wi-Fi interfaces");
        return -ENODEV;
    }

    /* AP and station modes require distinct interfaces. */
    if (ap_iface == sta_iface) {
        LOG_ERR("the AP and station interfaces are the same — is net.overlay merged?");
        return -ENODEV;
    }

    /* A suffix so two devices being set up on one table do not both announce the same network. */
    const ssize_t n = hwinfo_get_device_id(id, sizeof(id));

    if (n >= 2) {
        (void) snprintf(ap_ssid, sizeof(ap_ssid), "%s-%02X%02X", CONFIG_KVELD_PORTAL_SSID_PREFIX,
                        id[n - 2], id[n - 1]);
    } else {
        (void) snprintf(ap_ssid, sizeof(ap_ssid), "%s", CONFIG_KVELD_PORTAL_SSID_PREFIX);
    }

    LOG_INF("setup network will be %s", ap_ssid);

    return 0;
}

const char *kveld_portal_ap_ssid(void)
{
    return ap_ssid;
}

const char *kveld_portal_ap_password(void)
{
    return ap_password;
}

static bool generate_ap_password(void)
{
    uint32_t random[4];

    if (sys_csrand_get(random, sizeof(random)) != 0) {
        LOG_ERR("could not obtain random bytes for the setup password");
        return false;
    }

    const int n = snprintf(ap_password, sizeof(ap_password), "%s-%s-%s-%u",
                           ap_password_words[random[0] % ARRAY_SIZE(ap_password_words)],
                           ap_password_words[random[1] % ARRAY_SIZE(ap_password_words)],
                           ap_password_words[random[2] % ARRAY_SIZE(ap_password_words)],
                           (unsigned int) (random[3] % 10));

    if (n <= 0 || (size_t) n >= sizeof(ap_password)) {
        ap_password[0] = '\0';
        LOG_ERR("generated setup password did not fit");
        return false;
    }

    return true;
}

bool kveld_portal_station_connected(void)
{
    return station_connected;
}

void kveld_portal_set_station_connected(bool connected)
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

void kveld_portal_set_connection_error(int error)
{
    k_mutex_lock(&portal_state_lock, K_FOREVER);
    (void) snprintf(connection_error, sizeof(connection_error), "Wi-Fi error %d", error);
    k_mutex_unlock(&portal_state_lock);
}

void kveld_portal_clear_connection_error(void)
{
    k_mutex_lock(&portal_state_lock, K_FOREVER);
    connection_error[0] = '\0';
    k_mutex_unlock(&portal_state_lock);
}

void kveld_portal_copy_connection_error(char *out, size_t out_size)
{
    if (out == NULL || out_size == 0) {
        return;
    }

    k_mutex_lock(&portal_state_lock, K_FOREVER);
    (void) strncpy(out, connection_error, out_size - 1);
    out[out_size - 1] = '\0';
    k_mutex_unlock(&portal_state_lock);
}

void kveld_portal_set_sync_result(const char *result)
{
    k_mutex_lock(&portal_state_lock, K_FOREVER);
    (void) strncpy(sync_result, result != NULL ? result : "", sizeof(sync_result) - 1);
    sync_result[sizeof(sync_result) - 1] = '\0';
    k_mutex_unlock(&portal_state_lock);
}

void kveld_portal_copy_sync_result(char *out, size_t out_size)
{
    if (out == NULL || out_size == 0) {
        return;
    }

    k_mutex_lock(&portal_state_lock, K_FOREVER);
    (void) strncpy(out, sync_result, out_size - 1);
    out[out_size - 1] = '\0';
    k_mutex_unlock(&portal_state_lock);
}

uint32_t kveld_portal_window_remaining_s(void)
{
    if (!serving || portal_started_ms == 0) {
        return 0;
    }

    const int64_t remaining = CONFIG_KVELD_PORTAL_WINDOW_MS - (k_uptime_get() - portal_started_ms);

    return remaining > 0 ? (uint32_t) ((remaining + 999) / 1000) : 0;
}

int kveld_portal_forget_credentials(void)
{
    const int err = wifi_credentials_delete_all();

    if (err != 0) {
        return err;
    }

    if (sta_iface != NULL) {
        (void) net_mgmt(NET_REQUEST_WIFI_DISCONNECT, sta_iface, NULL, 0);
    }

    pending_ssid[0] = '\0';
    kveld_portal_set_station_connected(false);
    kveld_portal_clear_connection_error();

    return 0;
}

/* ------------------------------------------------------------------- scan */

void kveld_portal_scan_reset(void)
{
    k_mutex_lock(&scan_lock, K_FOREVER);
    scan_count = 0;
    k_mutex_unlock(&scan_lock);
}

void kveld_portal_scan_add(const char *ssid, uint8_t len, int8_t rssi, bool secure)
{
    if (len == 0 || len >= KVELD_SSID_MAX) {
        /* Hidden networks remain available through the typed SSID field. */
        return;
    }

    k_mutex_lock(&scan_lock, K_FOREVER);

    /* Show each SSID once, using the strongest scan result. */
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

bool kveld_portal_scan_start(void)
{
    struct wifi_scan_params params = {
        .scan_type = WIFI_SCAN_TYPE_ACTIVE,
        .bands = BIT(WIFI_FREQ_BAND_2_4_GHZ),
        .max_bss_cnt = CONFIG_KVELD_NET_SCAN_MAX,
    };

    kveld_portal_scan_reset();

    if (sta_iface == NULL) {
        return false;
    }

    /* On the station interface only. */
    const int err = net_mgmt(NET_REQUEST_WIFI_SCAN, sta_iface, &params, sizeof(params));

    if (err != 0) {
        LOG_WRN("scan request failed: %d", err);
        return false;
    }

    return true;
}

/* --------------------------------------------------------- access point */

bool kveld_portal_ap_start(void)
{
    if (!generate_ap_password()) {
        return false;
    }

    struct wifi_connect_req_params config = {
        .ssid = (const uint8_t *) ap_ssid,
        .ssid_length = strlen(ap_ssid),
        .psk = (const uint8_t *) ap_password,
        .psk_length = strlen(ap_password),
        .channel = WIFI_CHANNEL_ANY,
        .band = WIFI_FREQ_BAND_2_4_GHZ,
        .security = WIFI_SECURITY_TYPE_PSK,
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

/* Answer every A query with our own address, for as long as the portal runs. */
static void dns_thread(void *p1, void *p2, void *p3)
{
    ARG_UNUSED(p1);
    ARG_UNUSED(p2);
    ARG_UNUSED(p3);

    static uint8_t query[KVELD_DNS_BUF_SIZE];
    static uint8_t reply[KVELD_DNS_BUF_SIZE];

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
            /* Receive timeouts let the loop observe teardown. */
            continue;
        }

        const uint16_t len =
            kveld_dns_hijack(query, (uint16_t) n, AP_ADDR_HOST_ORDER, reply, sizeof(reply));

        if (len == 0) {
            continue;
        }

        (void) zsock_sendto(sock, reply, len, 0, (struct sockaddr *) &from, from_len);
    }
}

K_THREAD_DEFINE(kveld_dns_thread, DNS_STACK_SIZE, dns_thread, NULL, NULL, NULL, DNS_PRIORITY, 0, 0);

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

    /* A wildcard bind survives station address changes. */
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

bool kveld_portal_serve_start(void)
{
    struct net_in_addr addr;
    struct net_in_addr mask;

    if (ap_iface == NULL) {
        return false;
    }

    (void) net_addr_pton(NET_AF_INET, AP_ADDR, &addr);
    (void) net_addr_pton(NET_AF_INET, AP_NETMASK, &mask);

    /* DHCP reads the configured gateway when it starts. */
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
    portal_started_ms = k_uptime_get();

    LOG_INF("portal serving on http://%s", AP_ADDR);

    return true;
}

void kveld_portal_teardown(void)
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
        portal_started_ms = 0;
    }

    if (ap_iface != NULL) {
        (void) net_mgmt(NET_REQUEST_WIFI_AP_DISABLE, ap_iface, NULL, 0);

        (void) net_addr_pton(NET_AF_INET, AP_ADDR, &addr);
        (void) net_if_ipv4_addr_rm(ap_iface, &addr);
    }

    memset(ap_password, 0, sizeof(ap_password));

    LOG_INF("portal down");
}

/* ------------------------------------------------------------- connecting */

/* Join `ssid` using whatever the credential store holds for it. */
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

    /* Station startup completes asynchronously after interface enable. */
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

bool kveld_portal_connect_start(void)
{
    return connect_to(pending_ssid);
}

/* Collects the first stored SSID, for the connect-at-boot path. */
static void first_ssid(void *arg, const char *ssid, size_t len)
{
    char *out = arg;

    if (out[0] != '\0' || len == 0 || len >= KVELD_SSID_MAX) {
        return;
    }

    memcpy(out, ssid, len);
    out[len] = '\0';
}

bool kveld_portal_connect_stored(void)
{
    char ssid[KVELD_SSID_MAX] = {0};

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

/* Send one rendered page, or a 500 if it would not fit the buffer. */
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
    const int n = kveld_page_setup((char *) page_buf, sizeof(page_buf), scan_results, scan_count,
                                   kveld_language());
    k_mutex_unlock(&scan_lock);

    return send_page("/", n, response);
}

static int rescan_handler(struct http_client_ctx *client, enum http_transaction_status status,
                          const struct http_request_ctx *request,
                          struct http_response_ctx *response, void *user_data)
{
    ARG_UNUSED(client);
    ARG_UNUSED(request);
    ARG_UNUSED(user_data);

    if (status != HTTP_SERVER_REQUEST_DATA_FINAL) {
        return 0;
    }

    const bool started = kveld_portal_scan_start();
    const int n = kveld_page_notice(
        (char *) page_buf, sizeof(page_buf), started ? "Scanning" : "Scan unavailable",
        started ? "The network list is refreshing. Return to setup in a few seconds."
                : "The radio could not start a scan. You can still type a network name.");

    return send_page("/scan", n, response);
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

    char last_error[48] = {};
    char last_sync[80] = {};

    kveld_portal_copy_connection_error(last_error, sizeof(last_error));
    kveld_portal_copy_sync_result(last_sync, sizeof(last_sync));

    const int n = kveld_page_status(
        (char *) page_buf, sizeof(page_buf), ap_ssid, pending_ssid[0] != '\0' ? pending_ssid : NULL,
        station_connected, station_ip, last_error, last_sync, kveld_portal_window_remaining_s());

    return send_page("/status", n, response);
}

/* Request bodies may be split across non-NUL-terminated slices. */
static char form_body[512];
static size_t form_len;

static int save_handler(struct http_client_ctx *client, enum http_transaction_status status,
                        const struct http_request_ctx *request, struct http_response_ctx *response,
                        void *user_data)
{
    ARG_UNUSED(client);
    ARG_UNUSED(user_data);

    char ssid[KVELD_SSID_MAX] = {0};
    char psk[KVELD_PSK_MAX] = {0};

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
        /* Nothing may be sent before the body is complete. */
        return 0;
    }

    const int ssid_len =
        kveld_form_field(form_body, (uint16_t) form_len, "ssid", ssid, sizeof(ssid));
    const int psk_len = kveld_form_field(form_body, (uint16_t) form_len, "psk", psk, sizeof(psk));

    form_len = 0;

    if (ssid_len <= 0) {
        const int n = kveld_page_notice((char *) page_buf, sizeof(page_buf), "No network selected",
                                        "Choose a network or type its name, then save it.");

        return send_page("/save", n, response);
    }

    kveld_portal_clear_connection_error();

    const enum wifi_security_type type =
        psk_len > 0 ? WIFI_SECURITY_TYPE_PSK : WIFI_SECURITY_TYPE_NONE;

    /* Store credentials before Wi-Fi starts; NVS writes disable the instruction cache. */
    const int err = wifi_credentials_set_personal(ssid, (size_t) ssid_len, type, NULL, 0, psk,
                                                  (size_t) psk_len, 0, 0, 0);

    if (err != 0) {
        LOG_ERR("could not store the credentials: %d", err);
        response->status = HTTP_507_INSUFFICIENT_STORAGE;
        response->final_chunk = true;
        return 0;
    }

    (void) strncpy(pending_ssid, ssid, sizeof(pending_ssid) - 1);

    const int n = kveld_page_saved((char *) page_buf, sizeof(page_buf), ssid);

    /* The page goes out first and the join is left to `net`. */
    kveld_net_notify_credentials();

    return send_page("/save", n, response);
}

static int language_handler(struct http_client_ctx *client, enum http_transaction_status status,
                            const struct http_request_ctx *request,
                            struct http_response_ctx *response, void *user_data)
{
    ARG_UNUSED(client);
    ARG_UNUSED(user_data);

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
        return 0;
    }

    char lang[KVELD_LANGUAGE_LEN] = {0};
    const int lang_len =
        kveld_form_field(form_body, (uint16_t) form_len, "lang", lang, sizeof(lang));

    form_len = 0;

    if (lang_len != 2 || kveld_language_set(lang) != 0) {
        const int n = kveld_page_notice((char *) page_buf, sizeof(page_buf), "Language not saved",
                                        "That language is not available in this device image.");

        return send_page("/language", n, response);
    }

    const struct kveld_corpus_msg msg = {.language = {lang[0], lang[1], '\0', '\0'}};

    /* `app` reopens the store. */
    (void) zbus_chan_pub(&chan_corpus, &msg, K_MSEC(100));

    const int n = kveld_page_saved((char *) page_buf, sizeof(page_buf), NULL);

    return send_page("/language", n, response);
}

static int forget_handler(struct http_client_ctx *client, enum http_transaction_status status,
                          const struct http_request_ctx *request,
                          struct http_response_ctx *response, void *user_data)
{
    ARG_UNUSED(client);
    ARG_UNUSED(request);
    ARG_UNUSED(user_data);

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
        return 0;
    }

    char confirm[8] = {0};
    const int confirm_len =
        kveld_form_field(form_body, (uint16_t) form_len, "confirm", confirm, sizeof(confirm));

    form_len = 0;

    if (confirm_len != 6 || strcmp(confirm, "forget") != 0) {
        const int n = kveld_page_forget_confirm((char *) page_buf, sizeof(page_buf));

        return send_page("/forget", n, response);
    }

    const int err = kveld_portal_forget_credentials();
    const int n = kveld_page_notice(
        (char *) page_buf, sizeof(page_buf),
        err == 0 ? "Wi-Fi forgotten" : "Could not forget Wi-Fi",
        err == 0 ? "The saved network was removed. The device will stay offline until you set it "
                   "up again."
                 : "The saved network could not be removed. Try again or restart the device.");

    return send_page("/forget", n, response);
}

/* Check for new questions and firmware. */
static int sync_handler(struct http_client_ctx *client, enum http_transaction_status status,
                        const struct http_request_ctx *request, struct http_response_ctx *response,
                        void *user_data)
{
    ARG_UNUSED(client);
    ARG_UNUSED(request);
    ARG_UNUSED(user_data);

    if (status != HTTP_SERVER_REQUEST_DATA_FINAL) {
        return 0;
    }

    kveld_net_notify_sync();

    const int n = kveld_page_notice((char *) page_buf, sizeof(page_buf), "Update requested",
                                    "The device is checking for new questions and firmware. The "
                                    "result will appear on the device.");

    return send_page("/sync", n, response);
}

/* Everything else. */
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

/* Each resource gets its own detail struct. */
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

static struct http_resource_detail_dynamic rescan_detail = {
    .common = {.type = HTTP_RESOURCE_TYPE_DYNAMIC,
               .bitmask_of_supported_http_methods = BIT(HTTP_POST),
               .content_type = "text/html"},
    .cb = rescan_handler,
};

static struct http_resource_detail_dynamic save_detail = {
    .common = {.type = HTTP_RESOURCE_TYPE_DYNAMIC,
               .bitmask_of_supported_http_methods = BIT(HTTP_POST),
               .content_type = "text/html"},
    .cb = save_handler,
};

static struct http_resource_detail_dynamic language_detail = {
    .common = {.type = HTTP_RESOURCE_TYPE_DYNAMIC,
               .bitmask_of_supported_http_methods = BIT(HTTP_POST),
               .content_type = "text/html"},
    .cb = language_handler,
};

static struct http_resource_detail_dynamic sync_detail = {
    .common = {.type = HTTP_RESOURCE_TYPE_DYNAMIC,
               .bitmask_of_supported_http_methods = BIT(HTTP_POST),
               .content_type = "text/html"},
    .cb = sync_handler,
};

static struct http_resource_detail_dynamic forget_detail = {
    .common = {.type = HTTP_RESOURCE_TYPE_DYNAMIC,
               .bitmask_of_supported_http_methods = BIT(HTTP_POST),
               .content_type = "text/html"},
    .cb = forget_handler,
};

static struct http_resource_detail_dynamic catchall_detail = {
    .common = {.type = HTTP_RESOURCE_TYPE_DYNAMIC,
               .bitmask_of_supported_http_methods = BIT(HTTP_GET) | BIT(HTTP_POST),
               .content_type = "text/html"},
    .cb = catchall_handler,
};

static uint16_t http_port = HTTP_PORT;

/* Wildcard binding keeps HTTP available across interface changes. */
HTTP_SERVICE_DEFINE(kveld_portal, NULL, &http_port, CONFIG_HTTP_SERVER_MAX_CLIENTS, 4, NULL,
                    &catchall_detail.common, NULL);

HTTP_RESOURCE_DEFINE(setup_resource, kveld_portal, "/", &setup_detail);
HTTP_RESOURCE_DEFINE(status_resource, kveld_portal, "/status", &status_detail);
HTTP_RESOURCE_DEFINE(rescan_resource, kveld_portal, "/scan", &rescan_detail);
HTTP_RESOURCE_DEFINE(save_resource, kveld_portal, "/save", &save_detail);
HTTP_RESOURCE_DEFINE(language_resource, kveld_portal, "/language", &language_detail);
HTTP_RESOURCE_DEFINE(sync_resource, kveld_portal, "/sync", &sync_detail);
HTTP_RESOURCE_DEFINE(forget_resource, kveld_portal, "/forget", &forget_detail);
