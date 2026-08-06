/*
 * Reading the setup form back off the wire.
 *
 * No Zephyr headers, so the suite runs real bodies through it on the host.
 *
 * A browser posts `application/x-www-form-urlencoded`: fields joined by `&`,
 * key and value split by `=`, spaces written as `+`, and anything else that is
 * not alphanumeric written as `%` and two hex digits. A Wi-Fi password is
 * exactly the kind of string that exercises all of it.
 *
 * Strictness is the point. A malformed escape is rejected rather than passed
 * through as a literal `%`, because the alternative is a password that differs
 * from what somebody typed and a device that then reports the network as
 * unreachable. The same rule as qdb: reject at the edge, so nothing downstream
 * has to wonder.
 */

#pragma once

#include <stdint.h>

namespace tk
{

/** IEEE 802.11 caps an SSID at 32 bytes; the NUL makes 33. */
constexpr uint16_t kSsidBufSize = 33;

/** WPA2 passphrases run to 63 characters, or 64 hex digits for a raw PSK. */
constexpr uint16_t kPskBufSize = 65;

/**
 * Percent-decode one field of a form body.
 *
 * Matches whole keys only, so `ssid` does not match `xssid` or `ssid_hidden`.
 * The value is NUL-terminated on success and untouched on failure.
 *
 * @param out_size  including room for the terminator
 * @return decoded length, or -1 when the key is absent, the body is malformed,
 *         or the decoded value would not fit.
 */
int form_field(const char *body, uint16_t len, const char *key, char *out, uint16_t out_size);

/**
 * Percent-decode a whole value.
 *
 * Exposed for its own sake because it holds the escape rules, and a test that
 * drives it directly says more about a bad `%` than one that has to build a
 * form body around it first.
 *
 * @return decoded length, or -1 on a malformed escape or a value too long.
 */
int form_decode(const char *value, uint16_t len, char *out, uint16_t out_size);

} // namespace tk
