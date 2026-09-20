#include "snapshot.hpp"

#include <string.h>

namespace cicala
{
namespace
{
uint32_t hash(const uint8_t *data, size_t size)
{
    uint32_t value = 2166136261u;
    while (size--)
        value = (value ^ *data++) * 16777619u;
    return value;
}
void put(uint8_t *&p, uint32_t value, unsigned bytes)
{
    while (bytes--) {
        *p++ = static_cast<uint8_t>(value);
        value >>= 8;
    }
}
uint32_t get(const uint8_t *&p, unsigned bytes)
{
    uint32_t value = 0;
    for (unsigned i = 0; i < bytes; ++i)
        value |= uint32_t(*p++) << (8 * i);
    return value;
}
bool valid(const Snapshot &s)
{
    if ((s.play.permissions | s.play.draft | s.play.restrictions) & ~kPermissionsValid)
        return false;
    if (s.play.cursor > 3 || s.play.kind > static_cast<uint8_t>(ViewKind::Empty) ||
        s.play.len > kMaxQuestionBytes || s.bag.recent_len > kRecentRing ||
        s.bag.recent_next >= kRecentRing || s.bag.last_depth > 3 ||
        (s.bag.last_forms & ~kFormBitsValid))
        return false;
    for (unsigned i = 0; i < s.bag.recent_len; ++i)
        if (s.bag.recent[i] >= kMaxQuestions)
            return false;
    return true;
}
} // namespace

bool generation_newer(uint32_t a, uint32_t b)
{
    return a != b && uint32_t(a - b) < 0x80000000u;
}

bool snapshot_encode(const Snapshot &s, uint8_t *out, size_t capacity)
{
    if (!out || capacity < kSnapshotBytes || !valid(s))
        return false;
    uint8_t *p = out;
    memcpy(p, "CSN1", 4);
    p += 4;
    put(p, s.generation, 4);
    put(p, kMaxQuestions, 2);
    put(p, kRecentRing, 1);
    put(p, kMaxQuestionBytes, 2);
    put(p, s.play.permissions, 1);
    put(p, s.play.draft, 1);
    put(p, s.play.cursor, 1);
    put(p, s.play.menu, 1);
    put(p, s.play.kind, 1);
    put(p, s.play.restrictions, 1);
    put(p, s.play.len, 2);
    memset(p, 0, kMaxQuestionBytes);
    memcpy(p, s.play.text, s.play.len);
    p += kMaxQuestionBytes;
    put(p, s.bag.fingerprint, 4);
    for (auto index : s.bag.recent)
        put(p, index, 2);
    put(p, s.bag.recent_len, 1);
    put(p, s.bag.recent_next, 1);
    put(p, s.bag.last_depth, 1);
    put(p, s.bag.last_forms, 1);
    for (auto word : s.bag.drawn)
        put(p, word, 4);
    put(p, hash(out, kSnapshotBytes - 4), 4);
    return true;
}

bool snapshot_decode(const uint8_t *data, size_t size, Snapshot &out)
{
    if (!data || size != kSnapshotBytes || memcmp(data, "CSN1", 4))
        return false;
    const uint8_t *p = data + size - 4;
    if (get(p, 4) != hash(data, size - 4))
        return false;
    p = data + 4;
    Snapshot s{};
    s.generation = get(p, 4);
    if (get(p, 2) != kMaxQuestions || get(p, 1) != kRecentRing || get(p, 2) != kMaxQuestionBytes)
        return false;
    s.play.permissions = get(p, 1);
    s.play.draft = get(p, 1);
    s.play.cursor = get(p, 1);
    const auto menu = get(p, 1);
    if (menu > 1)
        return false;
    s.play.menu = menu;
    s.play.kind = get(p, 1);
    s.play.restrictions = get(p, 1);
    s.play.len = get(p, 2);
    memcpy(s.play.text, p, kMaxQuestionBytes);
    p += kMaxQuestionBytes;
    s.bag.fingerprint = get(p, 4);
    for (auto &index : s.bag.recent)
        index = get(p, 2);
    s.bag.recent_len = get(p, 1);
    s.bag.recent_next = get(p, 1);
    s.bag.last_depth = get(p, 1);
    s.bag.last_forms = get(p, 1);
    for (auto &word : s.bag.drawn)
        word = get(p, 4);
    if (!valid(s))
        return false;
    out = s;
    return true;
}
} // namespace cicala
