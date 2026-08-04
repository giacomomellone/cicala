/*
 * The question store: a reader for the TKB2 bundle format, and the shuffle bag
 * that draws from it without repeats.
 *
 * The format is specified in docs/sync_protocol.md and its executable
 * reference decoder is `parse_bundle()` in tools/build_bundle.py. Nothing here
 * includes a Zephyr header, so the suite runs the real English and German
 * bundles through it on the host.
 *
 * Reading is zero-copy: a Question points into the bundle buffer rather than
 * carrying its text. The bundle outlives every Question taken from it.
 */

#pragma once

#include <stddef.h>
#include <stdint.h>

namespace tk
{

/** Decks, in the order fixed by questions/schema.json x-tischkarte.decks. */
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
    /** 1..3. Normal playback excludes 3. */
    uint8_t depth;
    bool spicy;
    bool dark;
};

class Qdb
{
public:
    /**
     * Point the reader at a decompressed TKB2 image.
     *
     * Every length in the bundle is checked against `size`, so a truncated or
     * corrupt file is rejected here rather than read off the end later.
     *
     * @return false on bad magic, a short buffer, trailing bytes, or a corpus
     *         larger than kMaxQuestions.
     */
    bool open(const uint8_t *data, size_t size);

    bool is_open() const { return _data != nullptr; }

    uint16_t count() const { return _count; }

    const char *version() const { return _version; }
    uint8_t version_len() const { return _version_len; }

    const char *language() const { return _language; }
    uint8_t language_len() const { return _language_len; }

    /**
     * A cheap identity for this bundle.
     *
     * Bag state survives in RTC memory across deep sleep, but a sync can
     * replace the corpus underneath it, at which point the stored bitmaps
     * refer to questions that no longer exist. Comparing this catches that.
     */
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

/**
 * Draw without repeats until the deck is exhausted, then start a new cycle.
 *
 * State is a plain struct so it can live in RTC slow memory and survive the
 * reboot that deep sleep really is. Keeping it out of NVS means a Next press
 * costs no flash write.
 */
class Bag
{
public:
    /** Uniform random source. Injected so the suite can be deterministic. */
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

    /**
     * Attach to a bundle, discarding retained state if it is not the same one.
     *
     * @return true when the previous state was kept.
     */
    bool bind(const Qdb &qdb);

    /**
     * Next question for `deck`, or false when the deck yields nothing at all.
     *
     * Exclusions are applied in order of how much they matter. The recent ring
     * is a courtesy and is dropped first, because the smallest shipped deck is
     * no larger than the ring and honouring it there would leave nothing to
     * draw. The bag itself is only reset once every eligible question has been
     * seen — that is the no-repeats guarantee.
     */
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
