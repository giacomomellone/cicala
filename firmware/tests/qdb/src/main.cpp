
#include <string.h>

#include <zephyr/ztest.h>

#include "qdb.hpp"

using namespace kveld;

namespace
{

const uint8_t en_bundle[] = {
#include "en_qdb.inc"
};

const uint8_t de_bundle[] = {
#include "de_qdb.inc"
};

constexpr uint8_t kWildDeck = 5;
constexpr uint8_t kPlaybackDepth = CONFIG_KVELD_PLAYBACK_DEPTH_MAX;

struct Rng {
    uint32_t state;
};

uint32_t next_random(void *ctx)
{
    Rng *rng = static_cast<Rng *>(ctx);

    rng->state = rng->state * 1103515245u + 12345u;

    return rng->state >> 8;
}

bool text_equals(const Question &q, const char *literal)
{
    size_t i = 0;

    for (; i < q.len; i++) {
        if (literal[i] == '\0' || literal[i] != q.text[i]) {
            return false;
        }
    }

    return literal[i] == '\0';
}

Bag::State bag_state;
Rng rng;

} // namespace

ZTEST_SUITE(kveld_qdb, NULL, NULL, NULL, NULL, NULL);

ZTEST(kveld_qdb, test_the_shipped_bundles_open)
{
    Qdb en;
    Qdb de;

    zassert_true(en.open(en_bundle, sizeof(en_bundle)), "the English bundle must parse");
    zassert_true(de.open(de_bundle, sizeof(de_bundle)), "the German bundle must parse");

    zassert_equal(en.language_len(), 2);
    zassert_equal(en.language()[0], 'e');
    zassert_equal(en.language()[1], 'n');

    zassert_equal(de.language()[0], 'd');
    zassert_equal(de.language()[1], 'e');

    zassert_true(en.count() > 0);
    zassert_true(de.count() > 0);
    zassert_not_equal(en.fingerprint(), de.fingerprint(), "two corpora must not look alike");
}

ZTEST(kveld_qdb, test_every_question_is_readable_and_within_the_buffer)
{
    Qdb qdb;

    const uint8_t *const bundles[] = {en_bundle, de_bundle};
    const size_t sizes[] = {sizeof(en_bundle), sizeof(de_bundle)};

    for (size_t b = 0; b < ARRAY_SIZE(bundles); b++) {
        const uint8_t *bundle = bundles[b];
        const size_t size = sizes[b];

        zassert_true(qdb.open(bundle, size));

        for (uint16_t i = 0; i < qdb.count(); i++) {
            Question q;

            zassert_true(qdb.at(i, q), "question %u must be readable", i);
            zassert_true(q.len > 0, "question %u is empty", i);
            zassert_true(q.len <= CONFIG_KVELD_MAX_QUESTION_BYTES,
                         "question %u is %u bytes, over the %d the device renders", i, q.len,
                         CONFIG_KVELD_MAX_QUESTION_BYTES);
            zassert_true(q.depth >= 1 && q.depth <= 3, "question %u has depth %u", i, q.depth);
            zassert_true(q.deck_mask != 0, "question %u belongs to no deck", i);
            zassert_true((q.deck_mask & 0xC0) == 0, "question %u sets a reserved deck bit", i);
            zassert_true((q.forms & 0xE0) == 0, "question %u sets a reserved form bit", i);
        }
    }
}

ZTEST(kveld_qdb, test_tone_flags_stay_in_the_wild_deck)
{
    Qdb qdb;

    zassert_true(qdb.open(en_bundle, sizeof(en_bundle)));

    for (uint16_t i = 0; i < qdb.count(); i++) {
        Question q;

        zassert_true(qdb.at(i, q));

        if (!q.spicy && !q.dark) {
            continue;
        }

        zassert_equal(q.deck_mask, 1u << kWildDeck,
                      "question %u carries a tone flag outside Wild (mask %02x)", i, q.deck_mask);
    }
}

ZTEST(kveld_qdb, test_the_worked_example_from_the_spec_decodes)
{
    static const uint8_t bundle[] = {
        'Q',  'D', 'B', '3',  0x09, '2',  '0',  '2',  '6',  '.',  '0', '7', '.', '2',
        0x02, 'e', 'n', 0x01, 0x00, 0x03, 0x01, 0x00, 0x20, 0x00, 'W', 'h', 'e', 'n',
        ' ',  'd', 'i', 'd',  ' ',  'y',  'o',  'u',  ' ',  'l',  'a', 's', 't', ' ',
        's',  'i', 'n', 'g',  ' ',  'o',  'u',  't',  ' ',  'l',  'o', 'u', 'd', '?',
    };

    Qdb qdb;

    zassert_true(qdb.open(bundle, sizeof(bundle)));
    zassert_equal(qdb.count(), 1);

    Question q;

    zassert_true(qdb.at(0, q));
    zassert_true(text_equals(q, "When did you last sing out loud?"));
    zassert_equal(q.deck_mask, 0x03, "new_people and close");
    zassert_equal(q.depth, 2);
    zassert_equal(q.forms, 0, "the spec example carries no form tag");
    zassert_false(q.spicy);
    zassert_false(q.dark);
}

/* A minimal QDB3 writer for corpus-independent bag cases. */
struct SynQ {
    uint8_t mask;
    uint8_t depth;
    uint8_t forms;
    const char *text;
};

static size_t build_synthetic(uint8_t *out, const SynQ *qs, uint16_t n)
{
    size_t len = 0;
    const char *version = "t.1";
    const char *lang = "en";

    out[len++] = 'Q';
    out[len++] = 'D';
    out[len++] = 'B';
    out[len++] = '3';
    out[len++] = 3;
    memcpy(out + len, version, 3);
    len += 3;
    out[len++] = 2;
    memcpy(out + len, lang, 2);
    len += 2;
    out[len++] = n & 0xFF;
    out[len++] = n >> 8;

    for (uint16_t i = 0; i < n; i++) {
        const uint16_t tl = strlen(qs[i].text);

        out[len++] = qs[i].mask;
        out[len++] = qs[i].depth - 1;
        out[len++] = qs[i].forms;
        out[len++] = tl & 0xFF;
        out[len++] = tl >> 8;
        memcpy(out + len, qs[i].text, tl);
        len += tl;
    }

    return len;
}

static uint8_t synthetic_buf[1024];

static void open_synthetic(Qdb &qdb, const SynQ *qs, uint16_t n)
{
    zassert_true(qdb.open(synthetic_buf, build_synthetic(synthetic_buf, qs, n)));
}

ZTEST(kveld_qdb, test_a_damaged_bundle_is_refused)
{
    Qdb qdb;

    zassert_false(qdb.open(nullptr, 0));
    zassert_false(qdb.open(en_bundle, 3), "shorter than the magic");

    static uint8_t wrong_magic[sizeof(en_bundle)];

    for (size_t i = 0; i < sizeof(en_bundle); i++) {
        wrong_magic[i] = en_bundle[i];
    }

    wrong_magic[3] = '1';
    zassert_false(qdb.open(wrong_magic, sizeof(wrong_magic)), "QDB1 is not this format");

    zassert_false(qdb.open(en_bundle, sizeof(en_bundle) - 1),
                  "a truncated record runs off the end");
    zassert_false(qdb.is_open(), "a refused bundle must not stay half-open");
}

ZTEST(kveld_qdb, test_trailing_bytes_are_refused)
{
    static uint8_t padded[sizeof(en_bundle) + 1];

    for (size_t i = 0; i < sizeof(en_bundle); i++) {
        padded[i] = en_bundle[i];
    }

    padded[sizeof(en_bundle)] = 0xFF;

    Qdb qdb;

    zassert_false(qdb.open(padded, sizeof(padded)), "a bundle with a tail is not intact");
}

ZTEST(kveld_qdb, test_depth_three_is_out_of_normal_playback)
{
    Qdb qdb;

    zassert_true(qdb.open(en_bundle, sizeof(en_bundle)));

    bool any_depth_three = false;

    for (uint16_t i = 0; i < qdb.count(); i++) {
        Question q;

        zassert_true(qdb.at(i, q));

        if (q.depth != 3) {
            continue;
        }

        any_depth_three = true;

        for (uint8_t deck = 0; deck < kDeckCount; deck++) {
            zassert_false(qdb.is_eligible(i, deck, kPlaybackDepth),
                          "depth 3 must never be drawn automatically");
        }
    }

    zassert_true(any_depth_three, "the corpus should still contain depth 3 for this to mean much");

    for (uint8_t deck = 0; deck < kDeckCount; deck++) {
        zassert_true(qdb.eligible_count(deck, kPlaybackDepth) <= qdb.eligible_count(deck, 3));
    }
}

ZTEST(kveld_qdb, test_a_cycle_never_repeats)
{
    Qdb qdb;

    zassert_true(qdb.open(en_bundle, sizeof(en_bundle)));

    for (uint8_t deck = 0; deck < kDeckCount; deck++) {
        bag_state = {};
        rng = {.state = 12345u + deck};

        Bag bag(bag_state, next_random, &rng);

        bag.bind(qdb);

        const uint16_t eligible = qdb.eligible_count(deck, kPlaybackDepth);

        zassert_true(eligible > 0, "deck %u yields nothing at all", deck);

        static bool seen[kMaxQuestions];

        for (uint16_t i = 0; i < kMaxQuestions; i++) {
            seen[i] = false;
        }

        for (uint16_t i = 0; i < eligible; i++) {
            uint16_t index = 0;
            Question q;

            zassert_true(bag.draw(qdb, deck, kPlaybackDepth, index, q),
                         "deck %u ran dry after %u of %u", deck, i, eligible);
            zassert_false(seen[index], "deck %u repeated question %u within one cycle", deck,
                          index);
            zassert_true(qdb.is_eligible(index, deck, kPlaybackDepth),
                         "deck %u drew an ineligible question", deck);

            seen[index] = true;
        }

        zassert_equal(bag.drawn_count(deck), eligible, "the cycle should be exactly full");
    }
}

ZTEST(kveld_qdb, test_a_new_cycle_starts_once_the_deck_is_used_up)
{
    Qdb qdb;

    zassert_true(qdb.open(en_bundle, sizeof(en_bundle)));

    bag_state = {};
    rng = {.state = 7u};

    Bag bag(bag_state, next_random, &rng);

    bag.bind(qdb);

    constexpr uint8_t deck = 3;
    const uint16_t eligible = qdb.eligible_count(deck, kPlaybackDepth);

    for (uint16_t i = 0; i < eligible; i++) {
        uint16_t index = 0;
        Question q;

        zassert_true(bag.draw(qdb, deck, kPlaybackDepth, index, q));
    }

    uint16_t index = 0;
    Question q;

    zassert_true(bag.draw(qdb, deck, kPlaybackDepth, index, q), "the bag must refill, not stop");
    zassert_equal(bag.drawn_count(deck), 1, "the new cycle holds only the question just drawn");
}

ZTEST(kveld_qdb, test_the_smallest_deck_still_draws_despite_the_ring)
{
    Qdb qdb;

    zassert_true(qdb.open(de_bundle, sizeof(de_bundle)));

    bag_state = {};
    rng = {.state = 99u};

    Bag bag(bag_state, next_random, &rng);

    bag.bind(qdb);

    constexpr uint8_t here = 4;

    zassert_true(qdb.eligible_count(here, kPlaybackDepth) <= kRecentRing,
                 "this case is only interesting while the deck is no bigger than the ring");

    for (int i = 0; i < 50; i++) {
        uint16_t index = 0;
        Question q;

        zassert_true(bag.draw(qdb, here, kPlaybackDepth, index, q),
                     "the ring must not starve a small deck (draw %d)", i);
    }
}

ZTEST(kveld_qdb, test_the_ring_is_shared_across_decks)
{
    Qdb qdb;

    zassert_true(qdb.open(en_bundle, sizeof(en_bundle)));

    bag_state = {};
    rng = {.state = 4242u};

    Bag bag(bag_state, next_random, &rng);

    bag.bind(qdb);

    uint16_t recent[kRecentRing];
    uint8_t recent_len = 0;

    for (int i = 0; i < kRecentRing; i++) {
        const uint8_t deck = (i % 2 == 0) ? 0 : 1;
        uint16_t index = 0;
        Question q;

        zassert_true(bag.draw(qdb, deck, kPlaybackDepth, index, q));

        for (uint8_t j = 0; j < recent_len; j++) {
            zassert_not_equal(recent[j], index, "question %u came back while still in the ring",
                              index);
        }

        recent[recent_len++] = index;
    }
}

ZTEST(kveld_qdb, test_a_replaced_corpus_discards_retained_state)
{
    Qdb en;
    Qdb de;

    zassert_true(en.open(en_bundle, sizeof(en_bundle)));
    zassert_true(de.open(de_bundle, sizeof(de_bundle)));

    bag_state = {};
    rng = {.state = 5u};

    Bag bag(bag_state, next_random, &rng);

    zassert_false(bag.bind(en), "there is nothing retained the first time");

    uint16_t index = 0;
    Question q;

    zassert_true(bag.draw(en, 1, kPlaybackDepth, index, q));
    zassert_true(bag.drawn_count(1) > 0);

    zassert_true(bag.bind(en), "the same bundle keeps its cycle across a wake");
    zassert_true(bag.drawn_count(1) > 0);

    zassert_false(bag.bind(de), "a synced bundle invalidates bitmaps that index the old one");
    zassert_equal(bag.drawn_count(1), 0);
}

ZTEST(kveld_qdb, test_an_unopened_bundle_draws_nothing)
{
    Qdb qdb;

    bag_state = {};
    rng = {.state = 1u};

    Bag bag(bag_state, next_random, &rng);

    uint16_t index = 0;
    Question q;

    zassert_false(bag.draw(qdb, 0, kPlaybackDepth, index, q));

    zassert_true(qdb.open(en_bundle, sizeof(en_bundle)));
    zassert_false(bag.draw(qdb, kDeckCount, kPlaybackDepth, index, q), "there is no seventh deck");
}

ZTEST(kveld_qdb, test_texture_alternates_depth_bands_when_the_pool_allows)
{
    static const SynQ qs[] = {
        {0x01, 1, 0, "One?"},  {0x01, 2, 0, "Two?"},  {0x01, 1, 0, "Three?"},
        {0x01, 2, 0, "Four?"}, {0x01, 1, 0, "Five?"}, {0x01, 2, 0, "Six?"},
    };
    Qdb qdb;

    open_synthetic(qdb, qs, ARRAY_SIZE(qs));

    bag_state = {};
    rng = {.state = 17u};

    Bag bag(bag_state, next_random, &rng);

    bag.bind(qdb);

    uint8_t prev_band = 0;

    for (int i = 0; i < 6; i++) {
        uint16_t index = 0;
        Question q;

        zassert_true(bag.draw(qdb, 0, kPlaybackDepth, index, q));

        const uint8_t band = q.depth >= 2 ? 2 : 1;

        if (prev_band != 0) {
            zassert_not_equal(band, prev_band, "draw %d repeated the depth band", i);
        }

        prev_band = band;
    }
}

ZTEST(kveld_qdb, test_texture_keeps_form_variety_when_the_band_cannot_change)
{
    /* One depth band only; the form preference must survive that relaxation. */
    static const SynQ qs[] = {
        {0x01, 2, 0x01, "One?"},  {0x01, 2, 0x02, "Two?"},  {0x01, 2, 0x01, "Three?"},
        {0x01, 2, 0x02, "Four?"}, {0x01, 2, 0x01, "Five?"}, {0x01, 2, 0x02, "Six?"},
    };
    Qdb qdb;

    open_synthetic(qdb, qs, ARRAY_SIZE(qs));

    bag_state = {};
    rng = {.state = 23u};

    Bag bag(bag_state, next_random, &rng);

    bag.bind(qdb);

    uint8_t prev_forms = 0;

    for (int i = 0; i < 6; i++) {
        uint16_t index = 0;
        Question q;

        zassert_true(bag.draw(qdb, 0, kPlaybackDepth, index, q));
        zassert_equal(q.forms & prev_forms, 0, "draw %d repeated a form", i);
        prev_forms = q.forms;
    }
}

ZTEST(kveld_qdb, test_texture_relaxes_when_every_candidate_repeats)
{
    static const SynQ qs[] = {
        {0x01, 2, 0x01, "One?"},
        {0x01, 2, 0x01, "Two?"},
        {0x01, 2, 0x01, "Three?"},
    };
    Qdb qdb;

    open_synthetic(qdb, qs, ARRAY_SIZE(qs));

    bag_state = {};
    rng = {.state = 31u};

    Bag bag(bag_state, next_random, &rng);

    bag.bind(qdb);

    for (int i = 0; i < 9; i++) {
        uint16_t index = 0;
        Question q;

        zassert_true(bag.draw(qdb, 0, kPlaybackDepth, index, q),
                     "texture must relax, not starve (draw %d)", i);
    }
}

ZTEST(kveld_qdb, test_texture_survives_a_cycle_reset)
{
    static const SynQ qs[] = {
        {0x01, 1, 0, "One?"},
        {0x01, 2, 0, "Two?"},
        {0x01, 1, 0, "Three?"},
        {0x01, 2, 0, "Four?"},
    };
    Qdb qdb;

    open_synthetic(qdb, qs, ARRAY_SIZE(qs));

    bag_state = {};
    rng = {.state = 41u};

    Bag bag(bag_state, next_random, &rng);

    bag.bind(qdb);

    uint8_t last_band = 0;

    for (int i = 0; i < 4; i++) {
        uint16_t index = 0;
        Question q;

        zassert_true(bag.draw(qdb, 0, kPlaybackDepth, index, q));
        last_band = q.depth >= 2 ? 2 : 1;
    }

    uint16_t index = 0;
    Question q;

    zassert_true(bag.draw(qdb, 0, kPlaybackDepth, index, q), "the bag must refill");
    zassert_equal(q.depth >= 2 ? 2 : 1, last_band == 1 ? 2 : 1,
                  "a new cycle still avoids repeating the last band shown");
}

ZTEST(kveld_qdb, test_texture_is_shared_across_decks)
{
    static const SynQ qs[] = {
        {0x01, 1, 0, "New people one?"},
        {0x02, 1, 0, "Close one?"},
        {0x02, 2, 0, "Close two?"},
    };
    Qdb qdb;

    open_synthetic(qdb, qs, ARRAY_SIZE(qs));

    bag_state = {};
    rng = {.state = 53u};

    Bag bag(bag_state, next_random, &rng);

    bag.bind(qdb);

    uint16_t index = 0;
    Question q;

    zassert_true(bag.draw(qdb, 0, kPlaybackDepth, index, q));
    zassert_equal(index, 0);

    zassert_true(bag.draw(qdb, 1, kPlaybackDepth, index, q));
    zassert_equal(index, 2, "the table just saw depth 1, so Close should offer depth 2");
}

ZTEST(kveld_qdb, test_form_bits_decode)
{
    /* icebreaker | hypothetical on Here, depth 1. */
    static const SynQ qs[] = {
        {0x10, 1, 0x05, "What can you see from here?"},
    };
    Qdb qdb;

    open_synthetic(qdb, qs, ARRAY_SIZE(qs));

    Question q;

    zassert_true(qdb.at(0, q));
    zassert_equal(q.forms, 0x05);
    zassert_equal(q.depth, 1);
    zassert_equal(q.deck_mask, 0x10);
}
