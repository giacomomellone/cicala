/*
 * The HTTP GET both downloaders use. See fetch.h for the shape and the reason.
 *
 * This was the private half of src/sync.cpp until firmware updates needed the
 * same request with a different destination. Nothing about it changed in the
 * move except that the destination became a parameter: a bundle still lands in
 * a buffer, and an image now lands in a flash slot, through the same socket
 * code and the same one-request-per-connection rule.
 */

#include "fetch.h"

#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/http/client.h>
#include <zephyr/net/socket.h>

LOG_MODULE_REGISTER(tk_fetch, LOG_LEVEL_INF);

/*
 * Where the current transfer is going.
 *
 * File-scope because Zephyr's HTTP client hands its response callback a
 * `void *user_data` that it takes at request time, and there is exactly one
 * fetch in flight at a time — both callers run on the `net` thread.
 */
static const struct tk_fetch_sink *active_sink;
static size_t active_len;
static int active_err;

int tk_fetch_mem_write(void *ctx, const uint8_t *data, size_t len)
{
    struct tk_fetch_mem *mem = ctx;

    if (mem->len + len > mem->capacity) {
        /* Refused rather than truncated. A server that sends more than the
         * manifest promised is not one to take a prefix from. */
        mem->overflowed = true;

        return -EFBIG;
    }

    memcpy(mem->buf + mem->len, data, len);
    mem->len += len;

    return 0;
}

static int on_body(struct http_response *rsp, enum http_final_call final, void *user_data)
{
    ARG_UNUSED(final);
    ARG_UNUSED(user_data);

    if (rsp->body_frag_len == 0) {
        return 0;
    }

    if (active_err != 0) {
        /* Already given up. Keep draining rather than acting, because the HTTP
         * client has no way to be told to stop mid-response. */
        return 0;
    }

    const int err = active_sink->write(active_sink->ctx, rsp->body_frag_start, rsp->body_frag_len);

    if (err != 0) {
        active_err = err;

        return 0;
    }

    active_len += rsp->body_frag_len;

    return 0;
}

/** Open a socket to the configured host, with TLS unless told otherwise. */
static int connect_to_host(void)
{
    struct zsock_addrinfo hints = {0};
    struct zsock_addrinfo *res = NULL;

    hints.ai_family = NET_AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    char port[8];

    (void) snprintf(port, sizeof(port), "%d", CONFIG_TK_SYNC_PORT);

    int err = zsock_getaddrinfo(CONFIG_TK_SYNC_HOST, port, &hints, &res);

    if (err != 0 || res == NULL) {
        LOG_ERR("could not resolve %s: %d", CONFIG_TK_SYNC_HOST, err);

        return -EHOSTUNREACH;
    }

#ifdef CONFIG_TK_SYNC_INSECURE
    const int sock = zsock_socket(res->ai_family, res->ai_socktype, IPPROTO_TCP);
#else
    const int sock = zsock_socket(res->ai_family, res->ai_socktype, IPPROTO_TLS_1_2);
#endif

    if (sock < 0) {
        LOG_ERR("could not open a socket: %d", errno);
        zsock_freeaddrinfo(res);

        return -errno;
    }

#ifndef CONFIG_TK_SYNC_INSECURE
    /*
     * TLS without peer verification, deliberately and documented.
     *
     * Validating a certificate needs a trusted clock and this device has none:
     * no RTC source, no SNTP, and a wake is a fresh boot with no idea when it
     * is. Verifying with expiry checks disabled would accept a revoked or
     * expired certificate, which is most of what a certificate is for.
     *
     * So TLS here is the transport hosts will accept, plus confidentiality from
     * a passive observer. It authenticates nobody, and nothing downstream may
     * treat it as if it did — the Ed25519 signature is the whole of the
     * protection, for a bundle and for an image alike. See docs/decisions.md.
     */
    const int verify = TLS_PEER_VERIFY_NONE;

    if (zsock_setsockopt(sock, SOL_TLS, TLS_PEER_VERIFY, &verify, sizeof(verify)) < 0) {
        LOG_ERR("could not set the TLS verify mode: %d", errno);
        (void) zsock_close(sock);
        zsock_freeaddrinfo(res);

        return -EIO;
    }

    /* Sent anyway: shared hosts route on it, so without it the request reaches
     * the wrong site rather than failing honestly. */
    if (zsock_setsockopt(sock, SOL_TLS, TLS_HOSTNAME, CONFIG_TK_SYNC_HOST,
                         sizeof(CONFIG_TK_SYNC_HOST)) < 0) {
        LOG_WRN("could not set the TLS hostname: %d", errno);
    }
#endif

    err = zsock_connect(sock, res->ai_addr, res->ai_addrlen);

    zsock_freeaddrinfo(res);

    if (err < 0) {
        LOG_ERR("could not connect to %s: %d", CONFIG_TK_SYNC_HOST, errno);
        (void) zsock_close(sock);

        return -ECONNREFUSED;
    }

    return sock;
}

int tk_fetch(const char *path, const struct tk_fetch_sink *sink, int32_t timeout_ms)
{
    struct http_request req = {0};
    static uint8_t recv_buf[512];

    if (path == NULL || sink == NULL || sink->write == NULL) {
        return -EINVAL;
    }

    const int sock = connect_to_host();

    if (sock < 0) {
        return sock;
    }

    active_sink = sink;
    active_len = 0;
    active_err = 0;

    req.method = HTTP_GET;
    req.url = path;
    req.host = CONFIG_TK_SYNC_HOST;
    req.protocol = "HTTP/1.1";
    req.response = on_body;
    req.recv_buf = recv_buf;
    req.recv_buf_len = sizeof(recv_buf);

    const int err = http_client_req(sock, &req, timeout_ms, NULL);

    (void) zsock_close(sock);

    active_sink = NULL;

    if (err < 0) {
        LOG_ERR("GET %s failed: %d", path, err);

        return err;
    }

    if (active_err != 0) {
        LOG_ERR("GET %s: the body could not be stored: %d", path, active_err);

        return active_err;
    }

    if (active_len == 0) {
        LOG_ERR("GET %s returned nothing", path);

        return -ENODATA;
    }

    return (int) active_len;
}

const char *tk_fetch_path_of(const char *url)
{
    const char *at = strstr(url, "://");

    if (at == NULL) {
        return url;
    }

    at = strchr(at + 3, '/');

    return at != NULL ? at : "/";
}
