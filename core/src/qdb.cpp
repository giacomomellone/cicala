#include "qdb.hpp"

#include <string.h>

namespace cicala
{

namespace
{

// FNV-1a identifies corpus changes in retained bag state.
constexpr uint32_t kFnvOffset = 2166136261u;
constexpr uint32_t kFnvPrime = 16777619u;

/** Magic bytes double as the format version. */
constexpr uint8_t kMagic[] = {'Q', 'D', 'B', '4'};

/** Per-question record: metadata, forms, u16 LE text length, text. */
constexpr size_t kRecordHeaderBytes = 4;
constexpr size_t kRecordMetadataOffset = 0;
constexpr size_t kRecordFormsOffset = 1;
constexpr size_t kRecordTextLenOffset = 2;

/** Metadata byte: bits 0–1 are depth - 1, then the tone flags. */
constexpr uint8_t kDepthBitsMask = 0x03;
constexpr uint8_t kSexualBit = 1 << 2;
constexpr uint8_t kDarkBit = 1 << 3;

/** Texture cost classes: band and form can each repeat, or neither, or both. */
constexpr uint8_t kTextureClasses = 6;

uint32_t fnv1a(uint32_t hash, const void *data, size_t len)
{
    const uint8_t *bytes = static_cast<const uint8_t *>(data);

    for (size_t i = 0; i < len; i++) {
        hash ^= bytes[i];
        hash *= kFnvPrime;
    }

    return hash;
}

uint16_t read_u16(const uint8_t *p)
{
    return static_cast<uint16_t>(p[0]) | static_cast<uint16_t>(p[1] << 8);
}

} // namespace

bool Qdb::open(const uint8_t *data, size_t size)
{
    _data = nullptr;
    _count = 0;

    if (data == nullptr || size < sizeof(kMagic)) {
        return false;
    }

    if (memcmp(data, kMagic, sizeof(kMagic)) != 0) {
        return false;
    }

    size_t pos = sizeof(kMagic);

    // The version and language are u8-length-prefixed strings.
    const char *strings[2] = {nullptr, nullptr};
    uint8_t lengths[2] = {0, 0};

    for (int i = 0; i < 2; i++) {
        if (pos >= size) {
            return false;
        }

        const uint8_t len = data[pos++];

        if (pos + len > size) {
            return false;
        }

        strings[i] = reinterpret_cast<const char *>(data + pos);
        lengths[i] = len;
        pos += len;
    }

    if (pos + 2 > size) {
        return false;
    }

    const uint16_t count = read_u16(data + pos);
    pos += 2;

    if (count > kMaxQuestions) {
        // Bag state has one bit per question.
        return false;
    }

    // Validate all record bounds while opening the bundle.
    size_t walk = pos;

    for (uint16_t i = 0; i < count; i++) {
        if (walk + kRecordHeaderBytes > size) {
            return false;
        }

        const uint8_t metadata = data[walk];
        if ((metadata & 0xf0) || (metadata & kDepthBitsMask) == 3 ||
            (data[walk + kRecordFormsOffset] & ~kFormBitsValid)) {
            return false;
        }
        const uint16_t text_len = read_u16(data + walk + kRecordTextLenOffset);

        walk += kRecordHeaderBytes;

        if (walk + text_len > size) {
            return false;
        }

        walk += text_len;
    }

    if (walk != size) {
        return false;
    }

    _data = data;
    _size = size;
    _first = pos;
    _count = count;
    _version = strings[0];
    _version_len = lengths[0];
    _language = strings[1];
    _language_len = lengths[1];

    _fingerprint = fnv1a(kFnvOffset, data, size);

    return true;
}

bool Qdb::at(uint16_t index, Question &out) const
{
    if (!is_open() || index >= _count) {
        return false;
    }

    // A sequential walk avoids a RAM index.
    size_t pos = _first;

    for (uint16_t i = 0; i < index; i++) {
        pos += kRecordHeaderBytes + read_u16(_data + pos + kRecordTextLenOffset);
    }

    const uint8_t metadata = _data[pos + kRecordMetadataOffset];
    const uint16_t text_len = read_u16(_data + pos + kRecordTextLenOffset);

    out.depth = static_cast<uint8_t>((metadata & kDepthBitsMask) + 1);
    out.forms = _data[pos + kRecordFormsOffset];
    out.sexual = (metadata & kSexualBit) != 0;
    out.dark = (metadata & kDarkBit) != 0;
    out.text = reinterpret_cast<const char *>(_data + pos + kRecordHeaderBytes);
    out.len = text_len;

    return true;
}

bool Qdb::is_eligible(uint16_t index, uint8_t permissions) const
{
    Question q;
    return !(permissions & ~kPermissionsValid) && at(index, q) &&
           (!q.dark || (permissions & kAllowDark)) && (!q.sexual || (permissions & kAllowSexual)) &&
           (q.depth <= 2 || (permissions & kAllowHeavy));
}

uint16_t Qdb::eligible_count(uint8_t permissions) const
{
    uint16_t total = 0;
    for (uint16_t i = 0; i < count(); i++) {
        total += is_eligible(i, permissions);
    }
    return total;
}

bool Bag::bind(const Qdb &qdb)
{
    if (_state.fingerprint == qdb.fingerprint()) {
        return true;
    }

    reset();
    _state.fingerprint = qdb.fingerprint();

    return false;
}

void Bag::reset()
{
    _state.recent_len = 0;
    _state.recent_next = 0;
    _state.last_depth = 0;
    _state.last_forms = 0;

    memset(_state.drawn, 0, sizeof(_state.drawn));
}

void Bag::clear_eligible(const Qdb &qdb, uint8_t permissions)
{
    for (uint16_t i = 0; i < qdb.count(); i++) {
        if (qdb.is_eligible(i, permissions)) {
            _state.drawn[i / 32] &= ~(1u << (i % 32));
        }
    }
}

bool Bag::is_drawn(uint16_t index) const
{
    return (_state.drawn[index / 32] & (1u << (index % 32))) != 0;
}

void Bag::mark_drawn(uint16_t index)
{
    _state.drawn[index / 32] |= 1u << (index % 32);
}

bool Bag::is_recent(uint16_t index) const
{
    for (uint8_t i = 0; i < _state.recent_len; i++) {
        if (_state.recent[i] == index) {
            return true;
        }
    }

    return false;
}

void Bag::push_recent(uint16_t index)
{
    if (kRecentRing == 0) {
        return;
    }

    _state.recent[_state.recent_next] = index;
    _state.recent_next = static_cast<uint8_t>((_state.recent_next + 1) % kRecentRing);

    if (_state.recent_len < kRecentRing) {
        _state.recent_len++;
    }
}

uint16_t Bag::drawn_count() const
{
    uint16_t total = 0;

    for (uint16_t w = 0; w < kBitmapWords; w++) {
        uint32_t word = _state.drawn[w];

        while (word != 0) {
            total += (word & 1u);
            word >>= 1;
        }
    }

    return total;
}

uint8_t Bag::texture_cost(const Question &q) const
{
    const uint8_t band = depth_band(q.depth);
    uint8_t cost = (_state.last_depth == 3 && q.depth == 3) ? 3 : 0;

    if (_state.last_depth != 0 && band == depth_band(_state.last_depth)) {
        cost++;
    }

    if (_state.last_forms != 0 && (q.forms & _state.last_forms) != 0) {
        cost++;
    }

    return cost;
}

bool Bag::pick(const Qdb &qdb, uint8_t permissions, uint8_t recency, uint16_t &index) const
{
    // Count first to avoid a candidate buffer. With texture on, the draw is
    // uniform within the cheapest cost class: a question that repeats both
    // the depth band and a form of the last serve only wins when nothing
    // cheaper remains.
    uint16_t class_count[kTextureClasses] = {0};
    uint16_t candidates = 0;
    uint8_t best = kTextureClasses; // unset: costs run 0..kTextureClasses-1

    for (uint16_t i = 0; i < qdb.count(); i++) {
        if (!qdb.is_eligible(i, permissions) || is_drawn(i)) {
            continue;
        }

        if ((recency == 2 && is_recent(i)) ||
            (recency >= 1 && _state.recent_len &&
             _state.recent[(_state.recent_next + kRecentRing - 1) % kRecentRing] == i)) {
            continue;
        }

        uint8_t cost = 0;

        if (kTexture) {
            Question q;

            if (qdb.at(i, q)) {
                cost = texture_cost(q);
            }
        }

        class_count[cost]++;
        candidates++;

        if (cost < best) {
            best = cost;
        }
    }

    if (candidates == 0) {
        return false;
    }

    const uint16_t pool = kTexture ? class_count[best] : candidates;
    uint16_t target = static_cast<uint16_t>(_rand(_ctx) % pool);

    for (uint16_t i = 0; i < qdb.count(); i++) {
        if (!qdb.is_eligible(i, permissions) || is_drawn(i)) {
            continue;
        }

        if ((recency == 2 && is_recent(i)) ||
            (recency >= 1 && _state.recent_len &&
             _state.recent[(_state.recent_next + kRecentRing - 1) % kRecentRing] == i)) {
            continue;
        }

        if (kTexture) {
            Question q;

            if (!qdb.at(i, q) || texture_cost(q) != best) {
                continue;
            }
        }

        if (target == 0) {
            index = i;
            return true;
        }

        target--;
    }

    return false;
}

bool Bag::draw(const Qdb &qdb, uint8_t permissions, uint16_t &index, Question &out)
{
    if (!qdb.is_open() || (permissions & ~kPermissionsValid) || !qdb.eligible_count(permissions)) {
        return false;
    }
    bool found = false;
    for (int cycle = 0; cycle < 2 && !found; cycle++) {
        for (int recency = 2; recency >= 0 && !found; recency--) {
            found = pick(qdb, permissions, recency, index);
        }
        if (!found && cycle == 0) {
            clear_eligible(qdb, permissions);
        }
    }

    if (!found) {
        return false;
    }

    mark_drawn(index);
    push_recent(index);

    if (!qdb.at(index, out)) {
        return false;
    }

    // Remember the exact depth and forms across filter and cycle changes.
    _state.last_depth = out.depth;
    _state.last_forms = out.forms;

    return true;
}

} // namespace cicala
