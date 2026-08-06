#include "qdb.hpp"

namespace tk
{

namespace
{

/*
 * FNV-1a. Not a checksum of the corpus — just enough to notice that the
 * bundle is a different one, which is all the retained bag state cares about.
 */
constexpr uint32_t kFnvOffset = 2166136261u;
constexpr uint32_t kFnvPrime = 16777619u;

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

    if (data == nullptr || size < 4) {
        return false;
    }

    if (data[0] != 'Q' || data[1] != 'D' || data[2] != 'B' || data[3] != '2') {
        return false;
    }

    size_t pos = 4;

    // Two u8-length-prefixed strings: release version, then language code.
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
        // The bag keeps one bit per question per deck in RTC memory, so a
        // corpus it cannot track is refused rather than half-tracked.
        return false;
    }

    // Walk every record now: a bundle that runs off the end is worth finding
    // here, once, rather than on whichever draw first reaches the bad offset.
    size_t walk = pos;

    for (uint16_t i = 0; i < count; i++) {
        if (walk + 4 > size) {
            return false;
        }

        const uint16_t text_len = read_u16(data + walk + 2);

        walk += 4;

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

    uint32_t hash = fnv1a(kFnvOffset, _version, _version_len);

    hash = fnv1a(hash, _language, _language_len);
    hash = fnv1a(hash, &_count, sizeof(_count));
    _fingerprint = hash;

    return true;
}

bool Qdb::at(uint16_t index, Question &out) const
{
    if (!is_open() || index >= _count) {
        return false;
    }

    // Sequential walk rather than an offset table. With 240 questions this is
    // microseconds once per draw, and it costs no RAM on a device whose whole
    // budget is the reason the corpus lives in flash to begin with.
    size_t pos = _first;

    for (uint16_t i = 0; i < index; i++) {
        pos += 4 + read_u16(_data + pos + 2);
    }

    const uint8_t mask = _data[pos];
    const uint8_t metadata = _data[pos + 1];
    const uint16_t text_len = read_u16(_data + pos + 2);

    out.deck_mask = mask;
    out.depth = static_cast<uint8_t>((metadata & 0x03) + 1);
    out.spicy = (metadata & (1 << 2)) != 0;
    out.dark = (metadata & (1 << 3)) != 0;
    out.text = reinterpret_cast<const char *>(_data + pos + 4);
    out.len = text_len;

    return true;
}

bool Qdb::is_eligible(uint16_t index, uint8_t deck, uint8_t max_depth) const
{
    Question q;

    if (deck >= kDeckCount || !at(index, q)) {
        return false;
    }

    return (q.deck_mask & (1u << deck)) != 0 && q.depth <= max_depth;
}

uint16_t Qdb::eligible_count(uint8_t deck, uint8_t max_depth) const
{
    if (!is_open() || deck >= kDeckCount) {
        return 0;
    }

    uint16_t total = 0;
    size_t pos = _first;

    for (uint16_t i = 0; i < _count; i++) {
        const uint8_t mask = _data[pos];
        const uint8_t depth = static_cast<uint8_t>((_data[pos + 1] & 0x03) + 1);

        if ((mask & (1u << deck)) != 0 && depth <= max_depth) {
            total++;
        }

        pos += 4 + read_u16(_data + pos + 2);
    }

    return total;
}

// ------------------------------------------------------------------------ bag

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

    for (uint8_t deck = 0; deck < kDeckCount; deck++) {
        clear_deck(deck);
    }
}

void Bag::clear_deck(uint8_t deck)
{
    for (uint16_t w = 0; w < kBitmapWords; w++) {
        _state.drawn[deck][w] = 0;
    }
}

bool Bag::is_drawn(uint8_t deck, uint16_t index) const
{
    return (_state.drawn[deck][index / 32] & (1u << (index % 32))) != 0;
}

void Bag::mark_drawn(uint8_t deck, uint16_t index)
{
    _state.drawn[deck][index / 32] |= 1u << (index % 32);
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

uint16_t Bag::drawn_count(uint8_t deck) const
{
    if (deck >= kDeckCount) {
        return 0;
    }

    uint16_t total = 0;

    for (uint16_t w = 0; w < kBitmapWords; w++) {
        uint32_t word = _state.drawn[deck][w];

        while (word != 0) {
            total += (word & 1u);
            word >>= 1;
        }
    }

    return total;
}

bool Bag::pick(const Qdb &qdb, uint8_t deck, uint8_t max_depth, bool honour_recent,
               uint16_t &index) const
{
    // Two passes rather than a candidate list: counting first means no buffer
    // proportional to the corpus, which matters more here than the second walk
    // costs.
    uint16_t candidates = 0;

    for (uint16_t i = 0; i < qdb.count(); i++) {
        if (!qdb.is_eligible(i, deck, max_depth) || is_drawn(deck, i)) {
            continue;
        }

        if (honour_recent && is_recent(i)) {
            continue;
        }

        candidates++;
    }

    if (candidates == 0) {
        return false;
    }

    uint16_t target = static_cast<uint16_t>(_rand(_ctx) % candidates);

    for (uint16_t i = 0; i < qdb.count(); i++) {
        if (!qdb.is_eligible(i, deck, max_depth) || is_drawn(deck, i)) {
            continue;
        }

        if (honour_recent && is_recent(i)) {
            continue;
        }

        if (target == 0) {
            index = i;
            return true;
        }

        target--;
    }

    return false;
}

bool Bag::draw(const Qdb &qdb, uint8_t deck, uint8_t max_depth, uint16_t &index, Question &out)
{
    if (!qdb.is_open() || deck >= kDeckCount) {
        return false;
    }

    bool found = pick(qdb, deck, max_depth, true, index);

    if (!found) {
        // The ring is wider than what this deck has left. Dropping it beats
        // refusing to answer a press.
        found = pick(qdb, deck, max_depth, false, index);
    }

    if (!found) {
        // Every eligible question has been seen: that is a full cycle, and a
        // new one starts. The ring still applies, so the first question of the
        // new cycle is not one of the last few of the old.
        clear_deck(deck);
        found = pick(qdb, deck, max_depth, true, index);

        if (!found) {
            found = pick(qdb, deck, max_depth, false, index);
        }
    }

    if (!found) {
        return false;
    }

    mark_drawn(deck, index);
    push_recent(index);

    return qdb.at(index, out);
}

} // namespace tk
