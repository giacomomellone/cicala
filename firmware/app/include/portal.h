/** The radio and the sockets, behind the six things the state machine can ask for. */

#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** IEEE 802.11 caps an SSID at 32 bytes; the NUL makes 33. */
#define KVELD_SSID_MAX 33

/** One network in earshot, as the setup page shows it. */
struct kveld_scan_entry {
    char ssid[KVELD_SSID_MAX];
    int8_t rssi;
    bool secure;
};

/** Find the two interfaces and build this device's access point name. */
int kveld_portal_init(void);

/** The name this device's setup network announces. */
const char *kveld_portal_ap_ssid(void);

/** The password for this portal session's setup network. */
const char *kveld_portal_ap_password(void);

/** Begin a scan on the station interface. */
bool kveld_portal_scan_start(void);

/** Take one scan result. */
void kveld_portal_scan_add(const char *ssid, uint8_t len, int8_t rssi, bool secure);

/** Forget the previous session's results. */
void kveld_portal_scan_reset(void);

/** Raise the SoftAP. */
bool kveld_portal_ap_start(void);

/** Give the access point its address, a DHCP server, DNS and HTTP. */
bool kveld_portal_serve_start(void);

/** Try the credentials the form left in the store. */
bool kveld_portal_connect_start(void);

/** Stop serving and take the access point down. */
void kveld_portal_teardown(void);

/** Join a network already in the credential store, at boot. */
bool kveld_portal_connect_stored(void);

/** True once the station has an address. */
bool kveld_portal_station_connected(void);

/** Remember that the station joined, or dropped. */
void kveld_portal_set_station_connected(bool connected);

/** Remember why the last station connection failed. */
void kveld_portal_set_connection_error(int error);

/** Clear the last station connection error after a new attempt starts. */
void kveld_portal_clear_connection_error(void);

/** Copy the last station connection error into a caller-owned buffer. */
void kveld_portal_copy_connection_error(char *out, size_t out_size);

/** Copy the last sync result into a caller-owned buffer. */
void kveld_portal_copy_sync_result(char *out, size_t out_size);

/** Remember the result shown after a user-requested sync. */
void kveld_portal_set_sync_result(const char *result);

/** Seconds until the portal's hard deadline, rounded up. */
uint32_t kveld_portal_window_remaining_s(void);

/** Forget every saved Wi-Fi network. Return zero on success. */
int kveld_portal_forget_credentials(void);

#ifdef __cplusplus
}
#endif
