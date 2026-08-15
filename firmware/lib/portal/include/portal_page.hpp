/* HTML renderers for setup and status pages. */

#pragma once

#include <stdint.h>

namespace kveld
{

/** Network fields shown by the setup page. */
struct ScanEntry {
    /** NUL-terminated and untrusted. */
    char ssid[33];
    int8_t rssi;
    bool secure;
};

/** Status page data. Every string is NUL-terminated. */
struct PortalStatus {
    const char *ap_ssid;
    const char *board;
    const char *corpus_language;
    const char *corpus_version;
    uint16_t corpus_count;
    const char *firmware_version;
    /** The network the device will join, or nullptr when none is saved. */
    const char *saved_ssid;
    bool station_connected;
    /** Dotted quad, or an empty string when not connected. */
    const char *station_ip;
    /** Human-readable reason for the last failed join, or an empty string. */
    const char *connection_error;
    /** Human-readable result of the last user-requested sync, or an empty string. */
    const char *sync_result;
    uint32_t window_remaining_s;
};

/** Escape HTML text and attributes. Return length, or -1 if it does not fit. */
int html_escape(const char *in, uint16_t len, char *out, uint16_t out_size);

/** Render the setup page. Return bytes written, or -1 if it does not fit. */
int page_setup(char *out, uint16_t out_size, const ScanEntry *nets, uint8_t count,
               const char *current_language);

/** The page shown after a form post, saying what is being attempted. */
int page_saved(char *out, uint16_t out_size, const char *ssid);

/** Render a short result page with a heading and body. */
int page_notice(char *out, uint16_t out_size, const char *heading, const char *body);

/** Render the confirmation page for removing saved Wi-Fi credentials. */
int page_forget_confirm(char *out, uint16_t out_size);

/** The read-only page: what the device is, and what it knows. */
int page_status(char *out, uint16_t out_size, const PortalStatus &status);

} // namespace kveld
