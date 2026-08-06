/*
 * The pages the phone sees.
 *
 * No Zephyr headers, so the suite renders every page on the host and reads what
 * came out.
 *
 * HTML lives here rather than in the socket code for one reason that matters: a
 * network name is written by whoever owns the network, arrives over the air,
 * and is then printed into a page. Escaping it is the only thing standing
 * between a neighbour's access point named `<script>` and a setup page that
 * runs it. That is logic with branches over untrusted input, which belongs
 * where a test can drive it.
 *
 * The pages are plain HTML with a little inline CSS and no JavaScript. A phone
 * showing a captive sheet is a cut-down browser with no internet reachable
 * behind it, so anything fetched from elsewhere would simply not arrive.
 */

#pragma once

#include <stdint.h>

namespace tk
{

/** One network seen by a scan, trimmed to what the page shows. */
struct ScanEntry {
    /** NUL-terminated. Whatever the air said, so never trusted unescaped. */
    char ssid[33];
    int8_t rssi;
    bool secure;
};

/** What the status page reports. Every string is NUL-terminated. */
struct PortalStatus {
    const char *ap_ssid;
    const char *fw_version;
    const char *corpus_language;
    const char *corpus_version;
    uint16_t corpus_count;
    /** The network the device will join, or nullptr when none is saved. */
    const char *saved_ssid;
    bool station_connected;
    /** Dotted quad, or an empty string when not connected. */
    const char *station_ip;
};

/**
 * Escape text for HTML element content and double-quoted attributes.
 *
 * Covers `& < > " '`. The output is NUL-terminated on success.
 *
 * @return escaped length, or -1 when it would not fit.
 */
int html_escape(const char *in, uint16_t len, char *out, uint16_t out_size);

/**
 * The setup page: the networks in earshot, a password box, and the language.
 *
 * `count` may be zero, which renders the page with a note rather than an empty
 * list — a scan that found nothing is a normal outcome in a quiet room, and a
 * page with no explanation looks broken.
 *
 * @return bytes written, or -1 when `out` is too small.
 */
int page_setup(char *out, uint16_t out_size, const ScanEntry *nets, uint8_t count,
               const char *current_language);

/** The page shown after a form post, saying what is being attempted. */
int page_saved(char *out, uint16_t out_size, const char *ssid);

/** The read-only page: what the device is, and what it knows. */
int page_status(char *out, uint16_t out_size, const PortalStatus &status);

} // namespace tk
