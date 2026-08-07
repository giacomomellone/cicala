#include "manifest.hpp"

namespace tk
{

namespace
{

bool is_digit(char c)
{
    return c >= '0' && c <= '9';
}

size_t str_len(const char *s)
{
    size_t n = 0;

    while (s[n] != '\0') {
        n++;
    }

    return n;
}

/** One dotted component, and where the next one starts. */
uint32_t take_component(const char *s, size_t len, size_t &at)
{
    uint32_t value = 0;

    while (at < len && is_digit(s[at])) {
        /* Saturate rather than wrap. A version field long enough to overflow is
         * not a version, and wrapping could make an old release look new. */
        if (value < 100000000u) {
            value = value * 10 + (uint32_t) (s[at] - '0');
        }

        at++;
    }

    /* Skip whatever separated this component from the next — a dot normally,
     * but anything non-numeric is treated the same rather than rejected, since
     * this only has to order two strings the build produced. */
    while (at < len && !is_digit(s[at])) {
        at++;
    }

    return value;
}

/** Find `"key"` at the top level of `json`, returning the offset after the colon. */
bool find_key(const char *json, size_t len, const char *key, size_t from, size_t &value_at)
{
    const size_t key_len = str_len(key);

    for (size_t i = from; i + key_len + 2 < len; i++) {
        if (json[i] != '"') {
            continue;
        }

        bool match = true;

        for (size_t k = 0; k < key_len; k++) {
            if (json[i + 1 + k] != key[k]) {
                match = false;
                break;
            }
        }

        if (!match || json[i + 1 + key_len] != '"') {
            continue;
        }

        size_t at = i + key_len + 2;

        while (at < len &&
               (json[at] == ' ' || json[at] == '\n' || json[at] == '\r' || json[at] == '\t')) {
            at++;
        }

        if (at < len && json[at] == ':') {
            at++;

            while (at < len &&
                   (json[at] == ' ' || json[at] == '\n' || json[at] == '\r' || json[at] == '\t')) {
                at++;
            }

            value_at = at;

            return true;
        }
    }

    return false;
}

/** Copy a JSON string value into `out`, refusing anything that will not fit. */
bool take_string(const char *json, size_t len, size_t at, char *out, size_t out_size)
{
    if (at >= len || json[at] != '"') {
        return false;
    }

    at++;

    size_t written = 0;

    while (at < len && json[at] != '"') {
        /* No escape handling: nothing this reads — a URL, a version, a base64
         * signature — contains one, and a manifest that needs escapes is not
         * one this firmware should be acting on. */
        if (json[at] == '\\') {
            return false;
        }

        if (written + 1 >= out_size) {
            return false;
        }

        out[written] = json[at];
        written++;
        at++;
    }

    if (at >= len) {
        return false;
    }

    out[written] = '\0';

    return true;
}

bool take_uint(const char *json, size_t len, size_t at, uint32_t &out)
{
    if (at >= len || !is_digit(json[at])) {
        return false;
    }

    uint32_t value = 0;

    while (at < len && is_digit(json[at])) {
        if (value > 400000000u) {
            return false;
        }

        value = value * 10 + (uint32_t) (json[at] - '0');
        at++;
    }

    out = value;

    return true;
}

int hex_value(char c)
{
    if (c >= '0' && c <= '9') {
        return c - '0';
    }

    if (c >= 'a' && c <= 'f') {
        return c - 'a' + 10;
    }

    if (c >= 'A' && c <= 'F') {
        return c - 'A' + 10;
    }

    return -1;
}

/** Exactly `bytes` worth of lower- or upper-case hex, and no more. */
bool take_hex(const char *hex, uint8_t *out, size_t bytes)
{
    for (size_t i = 0; i < bytes; i++) {
        const int hi = hex_value(hex[i * 2]);
        const int lo = hex_value(hex[i * 2 + 1]);

        if (hi < 0 || lo < 0) {
            return false;
        }

        out[i] = (uint8_t) (hi << 4 | lo);
    }

    return hex[bytes * 2] == '\0';
}

int base64_value(char c)
{
    if (c >= 'A' && c <= 'Z') {
        return c - 'A';
    }

    if (c >= 'a' && c <= 'z') {
        return c - 'a' + 26;
    }

    if (c >= '0' && c <= '9') {
        return c - '0' + 52;
    }

    if (c == '+') {
        return 62;
    }

    if (c == '/') {
        return 63;
    }

    return -1;
}

/** Decode exactly `bytes` from base64, padding included. */
bool take_base64(const char *b64, uint8_t *out, size_t bytes)
{
    size_t written = 0;
    uint32_t acc = 0;
    int held = 0;

    for (size_t i = 0; b64[i] != '\0'; i++) {
        if (b64[i] == '=') {
            break;
        }

        const int v = base64_value(b64[i]);

        if (v < 0) {
            return false;
        }

        acc = (acc << 6) | (uint32_t) v;
        held += 6;

        if (held >= 8) {
            held -= 8;

            if (written >= bytes) {
                /* More data than the caller has room for. Refused rather than
                 * truncated: a 64-byte buffer holding the first 64 bytes of a
                 * longer signature is not a signature. */
                return false;
            }

            out[written] = (uint8_t) ((acc >> held) & 0xFF);
            written++;
        }
    }

    return written == bytes;
}

} // namespace

int version_compare(const char *a, const char *b)
{
    if (a == nullptr || b == nullptr) {
        return 0;
    }

    const size_t a_len = str_len(a);
    const size_t b_len = str_len(b);

    size_t a_at = 0;
    size_t b_at = 0;

    /* Component by component until one runs out, then the remaining components
     * of the longer one decide — a missing component counting as zero, so
     * 2026.08 is older than 2026.08.1 and equal to 2026.08.0. */
    while (a_at < a_len || b_at < b_len) {
        const uint32_t a_part = a_at < a_len ? take_component(a, a_len, a_at) : 0;
        const uint32_t b_part = b_at < b_len ? take_component(b, b_len, b_at) : 0;

        if (a_part != b_part) {
            return a_part < b_part ? -1 : 1;
        }
    }

    return 0;
}

bool manifest_parse(const char *json, size_t len, Manifest &out)
{
    if (json == nullptr || len == 0) {
        return false;
    }

    size_t at = 0;

    if (!find_key(json, len, "schema", 0, at) || !take_uint(json, len, at, out.schema)) {
        return false;
    }

    if (!find_key(json, len, "version", 0, at) ||
        !take_string(json, len, at, out.version, sizeof(out.version))) {
        return false;
    }

    if (!find_key(json, len, "min_fw", 0, at) ||
        !take_string(json, len, at, out.min_fw, sizeof(out.min_fw))) {
        return false;
    }

    return true;
}

bool manifest_entry(const char *json, size_t len, const char *language, ManifestEntry &out)
{
    if (json == nullptr || len == 0 || language == nullptr) {
        return false;
    }

    size_t languages_at = 0;

    if (!find_key(json, len, "languages", 0, languages_at)) {
        return false;
    }

    /*
     * The language key is data, so it is searched for rather than described.
     * Searching from the `languages` object means a top-level key with the same
     * name could not be mistaken for it.
     */
    size_t entry_at = 0;

    if (!find_key(json, len, language, languages_at, entry_at)) {
        return false;
    }

    size_t at = 0;

    if (!find_key(json, len, "url", entry_at, at) ||
        !take_string(json, len, at, out.url, sizeof(out.url))) {
        return false;
    }

    if (!find_key(json, len, "size", entry_at, at) || !take_uint(json, len, at, out.size)) {
        return false;
    }

    if (out.size == 0) {
        return false;
    }

    char scratch[kSignatureBytes * 2 + 8];

    if (!find_key(json, len, "sha256", entry_at, at) ||
        !take_string(json, len, at, scratch, sizeof(scratch)) ||
        !take_hex(scratch, out.sha256, sizeof(out.sha256))) {
        return false;
    }

    uint32_t count = 0;

    if (!find_key(json, len, "count", entry_at, at) || !take_uint(json, len, at, count) ||
        count > 0xFFFF) {
        return false;
    }

    out.count = (uint16_t) count;

    /*
     * `"sig": null` is a development bundle. Recorded rather than rejected
     * here, so the caller can refuse it with a message about what it is; a
     * parse failure would read as a corrupt manifest instead.
     */
    out.signed_ = false;

    for (size_t i = 0; i < sizeof(out.sig); i++) {
        out.sig[i] = 0;
    }

    if (!find_key(json, len, "sig", entry_at, at)) {
        return false;
    }

    if (at < len && json[at] == 'n') {
        return true;
    }

    if (!take_string(json, len, at, scratch, sizeof(scratch)) ||
        !take_base64(scratch, out.sig, sizeof(out.sig))) {
        return false;
    }

    out.signed_ = true;

    return true;
}

bool firmware_parse(const char *json, size_t len, FirmwareRelease &out)
{
    if (json == nullptr || len == 0) {
        return false;
    }

    size_t at = 0;

    if (!find_key(json, len, "schema", 0, at) || !take_uint(json, len, at, out.schema)) {
        return false;
    }

    if (!find_key(json, len, "version", 0, at) ||
        !take_string(json, len, at, out.version, sizeof(out.version))) {
        return false;
    }

    if (!find_key(json, len, "url", 0, at) ||
        !take_string(json, len, at, out.url, sizeof(out.url))) {
        return false;
    }

    if (!find_key(json, len, "size", 0, at) || !take_uint(json, len, at, out.size)) {
        return false;
    }

    if (out.size == 0) {
        return false;
    }

    char scratch[kSignatureBytes * 2 + 8];

    if (!find_key(json, len, "sha256", 0, at) ||
        !take_string(json, len, at, scratch, sizeof(scratch)) ||
        !take_hex(scratch, out.sha256, sizeof(out.sha256))) {
        return false;
    }

    out.signed_ = false;

    for (size_t i = 0; i < sizeof(out.sig); i++) {
        out.sig[i] = 0;
    }

    if (!find_key(json, len, "sig", 0, at)) {
        return false;
    }

    /* `"sig": null` — an image built without the manifest key. Parsed rather
     * than rejected, so ota.cpp can say what is wrong with it. */
    if (at < len && json[at] == 'n') {
        return true;
    }

    if (!take_string(json, len, at, scratch, sizeof(scratch)) ||
        !take_base64(scratch, out.sig, sizeof(out.sig))) {
        return false;
    }

    out.signed_ = true;

    return true;
}

} // namespace tk
