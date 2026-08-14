/* QDB2 reader and no-repeat shuffle bag. */

#pragma once

#include <stddef.h>
#include <stdint.h>

namespace tk
{

/** Deck count from questions/schema.json. */
constexpr uint8_t kDeckCount = 6;

#ifdef CONFIG_TK_MAX_QUESTIONS
constexpr uint16_t kMaxQuestions = CONFIG_TK_MAX_QUESTIONS;
#else
constexpr uint16_t kMaxQuestions = 512;
#endif

#ifdef CONFIG_TK_RECENT_RING
constexpr uint8_t kRecentRing = CONFIG_TK_RECENT_RING;
#else
constexpr uint8_t kRecentRing = 20;
#endif

constexpr uint16_t kBitmapWords = (kMaxQuestions + 31) / 32;

struct Question {
    /** Points into the bundle buffer. Not NUL-terminated. */
    const char *text;
    uint16_t len;
    /** Bit n set means eligible for deck n. */
    uint8_t deck_mask;
    /** Editorial depth, 1..3. */
    uint8_t depth;
    bool spicy;
    bool dark;
};

class Qdb
{
public:
    /** Open a decompressed QDB2 image after validating all bounds and records. */
    bool open(const uint8_t *data, size_t size);

    bool is_open() const { return _data != nullptr; }

    uint16_t count() const { return _count; }

    const char *version() const { return _version; }
    uint8_t version_len() const { return _version_len; }

    const char *language() const { return _language; }
    uint8_t language_len() const { return _language_len; }

    /** Identity used to invalidate retained bag state after a corpus change. */
    uint32_t fingerprint() const { return _fingerprint; }

    /** Question at `index`, in bundle order. */
    bool at(uint16_t index, Question &out) const;

    /** True when `index` may be drawn for `deck` at up to `max_depth`. */
    bool is_eligible(uint16_t index, uint8_t deck, uint8_t max_depth) const;

    /** How many questions `deck` can currently yield. */
    uint16_t eligible_count(uint8_t deck, uint8_t max_depth) const;

private:
    const uint8_t *_data = nullptr;
    size_t _size = 0;

    /** Offset of the first question record. */
    size_t _first = 0;
    uint16_t _count = 0;

    const char *_version = nullptr;
    uint8_t _version_len = 0;
    const char *_language = nullptr;
    uint8_t _language_len = 0;

    uint32_t _fingerprint = 0;
};

/** Draw without repeats. State is plain data suitable for RTC memory. */
class Bag
{
public:
    /** Injectable uniform random source. */
    using RandFn = uint32_t (*)(void *ctx);

    struct State {
        uint32_t fingerprint;
        /** Questions seen most recently, shared across decks. */
        uint16_t recent[kRecentRing];
        uint8_t recent_len;
        uint8_t recent_next;
        /** One bit per question, per deck: already drawn this cycle. */
        uint32_t drawn[kDeckCount][kBitmapWords];
    };

    Bag(State &state, RandFn rand, void *ctx) : _state(state), _rand(rand), _ctx(ctx) {}

    /** Bind to a bundle. Return true when retained state was compatible. */
    bool bind(const Qdb &qdb);

    /** Draw the next question, or return false when the deck has none. */
    bool draw(const Qdb &qdb, uint8_t deck, uint8_t max_depth, uint16_t &index, Question &out);

    /** Forget every cycle and the ring. */
    void reset();

    /** Questions drawn so far in `deck`'s current cycle. */
    uint16_t drawn_count(uint8_t deck) const;

private:
    bool pick(const Qdb &qdb, uint8_t deck, uint8_t max_depth, bool honour_recent,
              uint16_t &index) const;
    bool is_recent(uint16_t index) const;
    bool is_drawn(uint8_t deck, uint16_t index) const;
    void mark_drawn(uint8_t deck, uint16_t index);
    void push_recent(uint16_t index);
    void clear_deck(uint8_t deck);

    State &_state;
    RandFn _rand;
    void *_ctx;
};

} // namespace tk
