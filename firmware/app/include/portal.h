/*
 * The radio and the sockets, behind the six things the state machine can ask
 * for.
 *
 * Every function here is one of PortalIo's, so `net_logic.cpp` is a thin
 * translation and this file holds all the Zephyr networking. Nothing here
 * decides anything; the order these are called in is portal_fsm.cpp's.
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** IEEE 802.11 caps an SSID at 32 bytes; the NUL makes 33. */
#define TK_SSID_MAX 33

/** One network in earshot, as the setup page shows it. */
struct tk_scan_entry {
    char ssid[TK_SSID_MAX];
    int8_t rssi;
    bool secure;
};

/** Find the two interfaces and build this device's access point name. */
int tk_portal_init(void);

/** The name this device's setup network announces. Valid after init. */
const char *tk_portal_ap_ssid(void);

/**
 * Begin a scan on the station interface.
 *
 * Never on the access point interface: the driver stores the callback against
 * the station device and the completion handler reads it back unconditionally,
 * so a scan issued on the other one dereferences a null pointer.
 */
bool tk_portal_scan_start(void);

/** Take one scan result. Called from the net_mgmt callback. */
void tk_portal_scan_add(const char *ssid, uint8_t len, int8_t rssi, bool secure);

/** Forget the previous session's results. */
void tk_portal_scan_reset(void);

/** Raise the SoftAP. The result arrives as an AP_ENABLE_RESULT event. */
bool tk_portal_ap_start(void);

/**
 * Give the access point its address, a DHCP server, DNS and HTTP.
 *
 * Separate from tk_portal_ap_start() because binding a socket to
 * 192.168.4.1 fails until the interface actually carries that address, which
 * is only true once the AP reports itself enabled.
 */
bool tk_portal_serve_start(void);

/** Try the credentials the form left in the store. */
bool tk_portal_connect_start(void);

/** Stop serving and take the access point down. Safe to call twice. */
void tk_portal_teardown(void);

/** Join a network already in the credential store, at boot. */
bool tk_portal_connect_stored(void);

/** True once the station has an address. */
bool tk_portal_station_connected(void);

/** Remember that the station joined, or dropped. */
void tk_portal_set_station_connected(bool connected);

#ifdef __cplusplus
}
#endif
