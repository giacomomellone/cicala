#include "portal_form.hpp"

namespace kveld
{

namespace
{

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

uint16_t key_length(const char *key)
{
    uint16_t n = 0;

    while (key[n] != '\0') {
        n++;
    }

    return n;
}

bool same(const char *a, const char *b, uint16_t len)
{
    for (uint16_t i = 0; i < len; i++) {
        if (a[i] != b[i]) {
            return false;
        }
    }

    return true;
}

} // namespace

int form_decode(const char *value, uint16_t len, char *out, uint16_t out_size)
{
    if (value == nullptr || out == nullptr || out_size == 0) {
        return -1;
    }

    uint16_t written = 0;

    for (uint16_t i = 0; i < len; i++) {
        char c = value[i];

        if (c == '+') {
            c = ' ';
        } else if (c == '%') {
            if ((uint32_t) i + 2 >= (uint32_t) len) {
                return -1;
            }

            const int hi = hex_value(value[i + 1]);
            const int lo = hex_value(value[i + 2]);

            if (hi < 0 || lo < 0) {
                return -1;
            }

            c = (char) (hi << 4 | lo);
            i += 2;
        }

        // Reserve one byte for the terminator.
        if (written + 1 >= out_size) {
            return -1;
        }

        out[written] = c;
        written++;
    }

    out[written] = '\0';

    return written;
}

int form_field(const char *body, uint16_t len, const char *key, char *out, uint16_t out_size)
{
    if (body == nullptr || key == nullptr) {
        return -1;
    }

    const uint16_t key_len = key_length(key);

    if (key_len == 0) {
        return -1;
    }

    uint16_t at = 0;

    while (at < len) {
        uint16_t end = at;

        while (end < len && body[end] != '&') {
            end++;
        }

        // A field without '=' has no value.
        uint16_t split = at;

        while (split < end && body[split] != '=') {
            split++;
        }

        if (split < end && split - at == key_len && same(&body[at], key, key_len)) {
            return form_decode(&body[split + 1], (uint16_t) (end - split - 1), out, out_size);
        }

        at = (uint16_t) (end + 1);
    }

    return -1;
}

} // namespace kveld
