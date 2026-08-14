/** C face of the portal's state machine, and of the C++ that renders its pages. */

#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "portal.h"

#ifdef __cplusplus
extern "C" {
#endif

/** WPA2 passphrases run to 63 characters, or 64 hex digits for a raw PSK. */
#define TK_PSK_MAX 65

/** Classic DNS over UDP, before EDNS0 raises it. */
#define TK_DNS_BUF_SIZE 512

/** ------------------------------------------------------------- the machine */

/** Enter the portal. */
void tk_net_post_start(void);

/** Leave it, from wherever it is. */
void tk_net_post_stop(void);

void tk_net_post_scan_done(void);
void tk_net_post_ap_ready(bool ok);
void tk_net_post_credentials(void);
void tk_net_post_connected(bool ok);

/** Tick the machine until it stops moving. */
void tk_net_run(void);

/** Current state, as a PortalFsm::State value. */
int tk_net_state(void);

/** True while the portal machine is anywhere but OFF. */
bool tk_net_portal_active(void);

/** -------------------------------------------------------------- the pages */

/** Render the setup page into `out`. */
int tk_page_setup(char *out, size_t out_size, const struct tk_scan_entry *nets, uint8_t count,
                  const char *language);

int tk_page_saved(char *out, size_t out_size, const char *ssid);

int tk_page_notice(char *out, size_t out_size, const char *heading, const char *body);

int tk_page_forget_confirm(char *out, size_t out_size);

int tk_page_status(char *out, size_t out_size, const char *ap_ssid, const char *saved_ssid,
                   bool connected, const char *station_ip, const char *connection_error,
                   const char *sync_result, uint32_t window_remaining_s);

/** --------------------------------------------------------------- the wire */

/** Read one field out of a posted form body. */
int tk_form_field(const char *body, uint16_t len, const char *key, char *out, uint16_t out_size);

/** Answer a DNS query with the portal's own address. */
uint16_t tk_dns_hijack(const uint8_t *query, uint16_t len, uint32_t addr, uint8_t *out,
                       uint16_t out_size);

#ifdef __cplusplus
}
#endif
