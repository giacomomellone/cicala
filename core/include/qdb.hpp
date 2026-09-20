/* QDB4 reader and no-repeat shuffle bag. */

#pragma once

#include <stddef.h>
#include <stdint.h>
#include "config.hpp"

namespace cicala
{

/** Independent permissions, off on a fresh start. */
constexpr uint8_t kAllowDark = 1 << 0;
constexpr uint8_t kAllowSexual = 1 << 1;
constexpr uint8_t kAllowHeavy = 1 << 2;
constexpr uint8_t kPermissionsValid = 7;

constexpr uint16_t kBitmapWords = (kMaxQuestions + 31) / 32;

/** Form bits in schema tag order minus the tone flags sexual and dark. */
constexpr uint8_t kFormIcebreaker = 1 << 0;
constexpr uint8_t kFormReflective = 1 << 1;
constexpr uint8_t kFormHypothetical = 1 << 2;
constexpr uint8_t kFormMemory = 1 << 3;
constexpr uint8_t kFormWouldYouRather = 1 << 4;
constexpr uint8_t kFormBitsValid = static_cast<uint8_t>(
    kFormIcebreaker | kFormReflective | kFormHypothetical | kFormMemory | kFormWouldYouRather);

/** The bag's texture banding: little public exposure versus a personal
    construction or more. */
constexpr uint8_t depth_band(uint8_t depth)
{
    return depth >= 2 ? 2 : 1;
}

struct Question {
    /** Points into the bundle buffer. Not NUL-terminated. */
    const char *text;
    uint16_t len;
    /** Editorial depth, 1..3. */
    uint8_t depth;
    /** Form bitmask; zero when the question carries no form tag. */
    uint8_t forms;
    bool sexual;
    bool dark;
};

class Qdb
{
public:
    /** Open a decompressed QDB4 image after validating all bounds and records. */
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

    /** True when every restriction has its corresponding permission. */
    bool is_eligible(uint16_t index, uint8_t permissions) const;

    /** How many questions are currently allowed. */
    uint16_t eligible_count(uint8_t permissions) const;

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
        /** Questions seen most recently, for this language. */
        uint16_t recent[kRecentRing];
        uint8_t recent_len;
        uint8_t recent_next;
        /** Exact depth and form mask of the question served last, shared
            in this language; zero when nothing has been served. */
        uint8_t last_depth;
        uint8_t last_forms;
        /** One bit per question: already drawn this cycle. */
        uint32_t drawn[kBitmapWords];
    };

    Bag(State &state, RandFn rand, void *ctx) : _state(state), _rand(rand), _ctx(ctx) {}

    /** Bind to a bundle. Return true when retained state was compatible. */
    bool bind(const Qdb &qdb);

    /** Draw the next question, or return false when the allowed pool is empty. */
    bool draw(const Qdb &qdb, uint8_t permissions, uint16_t &index, Question &out);

    /** Forget every cycle and the ring. */
    void reset();

    /** Questions marked seen in the current cycle. */
    uint16_t drawn_count() const;

private:
    bool pick(const Qdb &qdb, uint8_t permissions, uint8_t recency, uint16_t &index) const;
    /** Cost of serving a question now: one for the same depth band as the
        last serve, one for sharing a form with it. */
    uint8_t texture_cost(const Question &q) const;
    bool is_recent(uint16_t index) const;
    bool is_drawn(uint16_t index) const;
    void mark_drawn(uint16_t index);
    void push_recent(uint16_t index);
    void clear_eligible(const Qdb &qdb, uint8_t permissions);

    State &_state;
    RandFn _rand;
    void *_ctx;
};

} // namespace cicala
