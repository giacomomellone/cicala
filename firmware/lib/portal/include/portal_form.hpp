/* Parser for application/x-www-form-urlencoded setup requests. */

#pragma once

#include <stdint.h>

namespace tk
{

/** IEEE 802.11 SSID limit plus a terminator. */
constexpr uint16_t kSsidBufSize = 33;

/** WPA2 passphrase or raw PSK limit plus a terminator. */
constexpr uint16_t kPskBufSize = 65;

/** Decode one exact field. Return its length, or -1 on absence or error. */
int form_field(const char *body, uint16_t len, const char *key, char *out, uint16_t out_size);

/** Percent-decode a value. Return its length, or -1 on error. */
int form_decode(const char *value, uint16_t len, char *out, uint16_t out_size);

} // namespace tk
