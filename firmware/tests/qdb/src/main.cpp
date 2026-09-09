#include <string.h>
#include <zephyr/ztest.h>
#include "qdb.hpp"
using namespace cicala;
namespace
{
const uint8_t en_bundle[] = {
#include "en_qdb.inc"
};
const uint8_t de_bundle[] = {
#include "de_qdb.inc"
};
const uint8_t en_shipped[] = {
#include "en_shipped_qdb.inc"
};
const uint8_t de_shipped[] = {
#include "de_shipped_qdb.inc"
};
uint32_t zero(void *)
{
    return 0;
}
// Metadata-only variants of an existing protocol example; no new question copy.
struct Fixture {
    uint8_t bytes[512] = {'Q', 'D', 'B', '4', 1, 'v', 2, 'e', 'n', 0, 0};
    size_t size = 11;
    uint8_t count = 0;
    void add(uint8_t depth, uint8_t permissions = 0, uint8_t forms = 0)
    {
        const char *text = "When did you last sing out loud?";
        bytes[size++] =
            (depth - 1) | (permissions & kAllowSexual ? 4 : 0) | (permissions & kAllowDark ? 8 : 0);
        bytes[size++] = forms;
        bytes[size++] = strlen(text);
        bytes[size++] = 0;
        memcpy(bytes + size, text, strlen(text));
        size += strlen(text);
        bytes[9] = ++count;
    }
};
} // namespace
ZTEST_SUITE(cicala_qdb, NULL, NULL, NULL, NULL, NULL);
ZTEST(cicala_qdb, test_all_bundles_open_and_questions_fit)
{
    const uint8_t *bundles[] = {en_bundle, de_bundle, en_shipped, de_shipped};
    size_t sizes[] = {sizeof(en_bundle), sizeof(de_bundle), sizeof(en_shipped), sizeof(de_shipped)};
    for (int b = 0; b < 4; b++) {
        Qdb qdb;
        zassert_true(qdb.open(bundles[b], sizes[b]));
        for (uint16_t i = 0; i < qdb.count(); i++) {
            Question q;
            zassert_true(qdb.at(i, q));
            zassert_between_inclusive(q.depth, 1, 3);
            zassert_true(q.len <= CONFIG_CICALA_MAX_QUESTION_BYTES);
            zassert_equal(q.forms & ~kFormBitsValid, 0);
        }
    }
}
ZTEST(cicala_qdb, test_every_permission_combination_is_independent)
{
    Fixture f;
    for (int required = 0; required < 8; required++)
        f.add(required & 4 ? 3 : 2, required);
    Qdb qdb;
    zassert_true(qdb.open(f.bytes, f.size));
    for (int allowed = 0; allowed < 8; allowed++) {
        for (int required = 0; required < 8; required++)
            zassert_equal(qdb.is_eligible(required, allowed), (required & ~allowed) == 0);
    }
}
ZTEST(cicala_qdb, test_rejects_old_format_truncation_and_reserved_bits)
{
    Fixture f;
    f.add(2, kAllowSexual | kAllowDark, kFormMemory);
    Qdb qdb;
    zassert_true(qdb.open(f.bytes, f.size));
    Question q;
    zassert_true(qdb.at(0, q));
    zassert_true(q.sexual && q.dark);
    zassert_equal(q.forms, kFormMemory);
    zassert_equal(q.depth, 2);
    for (size_t size = 0; size < f.size; size++)
        zassert_false(qdb.open(f.bytes, size));
    f.bytes[3] = '3';
    zassert_false(qdb.open(f.bytes, f.size));
    f.bytes[3] = '4';
    f.bytes[11] = 3;
    zassert_false(qdb.open(f.bytes, f.size));
    f.bytes[11] = 0x10;
    zassert_false(qdb.open(f.bytes, f.size));
    f.bytes[11] = 0;
    f.bytes[12] = 0x80;
    zassert_false(qdb.open(f.bytes, f.size));
    zassert_false(qdb.open(nullptr, 0));
}
ZTEST(cicala_qdb, test_cycles_never_repeat_or_escape_permissions)
{
    Qdb qdb;
    zassert_true(qdb.open(en_bundle, sizeof(en_bundle)));
    for (uint8_t p = 0; p < 8; p++) {
        Bag::State state{};
        Bag bag(state, zero, nullptr);
        bag.bind(qdb);
        uint16_t eligible = qdb.eligible_count(p), index = 0;
        Question q;
        zassert_true(eligible > 0);
        for (int cycle = 0; cycle < 2; cycle++) {
            bool seen[kMaxQuestions] = {};
            for (uint16_t i = 0; i < eligible; i++) {
                zassert_true(bag.draw(qdb, p, index, q));
                zassert_true(qdb.is_eligible(index, p));
                zassert_false(seen[index]);
                seen[index] = true;
            }
        }
    }
}
ZTEST(cicala_qdb, test_recent_window_and_current_are_avoided)
{
    Qdb qdb;
    zassert_true(qdb.open(en_bundle, sizeof(en_bundle)));
    zassert_true(qdb.eligible_count(0) > kRecentRing);
    Bag::State state{};
    Bag bag(state, zero, nullptr);
    bag.bind(qdb);
    uint16_t index;
    Question q;
    for (int i = 0; i < 150; i++) {
        Bag::State before = state;
        zassert_true(bag.draw(qdb, 0, index, q));
        for (int j = 0; j < before.recent_len; j++)
            zassert_not_equal(index, before.recent[j]);
    }
}
ZTEST(cicala_qdb, test_filter_changes_preserve_seen_history)
{
    Fixture f;
    f.add(1, kAllowSexual);
    f.add(1);
    f.add(1);
    Qdb qdb;
    zassert_true(qdb.open(f.bytes, f.size));
    Bag::State state{};
    Bag bag(state, zero, nullptr);
    bag.bind(qdb);
    uint16_t index;
    Question q;
    zassert_true(bag.draw(qdb, kAllowSexual, index, q));
    zassert_equal(index, 0);
    zassert_true(bag.draw(qdb, 0, index, q));
    zassert_equal(index, 1);
    zassert_true(bag.draw(qdb, kAllowSexual, index, q));
    zassert_equal(index, 2);
}
ZTEST(cicala_qdb, test_breather_does_not_force_early_repeat)
{
    Fixture f;
    f.add(3, 0, kFormMemory);
    f.add(3, 0, kFormReflective);
    f.add(2, 0, kFormMemory);
    Qdb qdb;
    zassert_true(qdb.open(f.bytes, f.size));
    Bag::State state{};
    Bag bag(state, zero, nullptr);
    bag.bind(qdb);
    uint16_t index;
    Question q;
    zassert_true(bag.draw(qdb, 7, index, q));
    zassert_equal(index, 0);
    zassert_true(bag.draw(qdb, 7, index, q));
    zassert_equal(index, 2);
    state.last_depth = 3;
    zassert_true(bag.draw(qdb, 7, index, q));
    zassert_equal(index, 1);
}
ZTEST(cicala_qdb, test_singleton_relaxes_and_empty_does_not_mutate)
{
    Fixture f;
    f.add(3, kAllowDark | kAllowSexual);
    Qdb qdb;
    zassert_true(qdb.open(f.bytes, f.size));
    Bag::State state{};
    Bag bag(state, zero, nullptr);
    bag.bind(qdb);
    uint16_t index;
    Question q;
    Bag::State before = state;
    zassert_false(bag.draw(qdb, 0, index, q));
    zassert_mem_equal(&state, &before, sizeof(state));
    for (int i = 0; i < 10; i++) {
        zassert_true(bag.draw(qdb, 7, index, q));
        zassert_equal(index, 0);
    }
}
ZTEST(cicala_qdb, test_same_version_content_change_invalidates_indices)
{
    Fixture f;
    f.add(1);
    Qdb qdb;
    zassert_true(qdb.open(f.bytes, f.size));
    Bag::State state{};
    Bag bag(state, zero, nullptr);
    zassert_false(bag.bind(qdb));
    uint16_t index;
    Question q;
    bag.draw(qdb, 0, index, q);
    zassert_true(bag.bind(qdb));
    zassert_equal(bag.drawn_count(), 1);
    f.bytes[11] = 1;
    zassert_true(qdb.open(f.bytes, f.size));
    zassert_false(bag.bind(qdb));
    zassert_equal(bag.drawn_count(), 0);
    zassert_equal(state.last_depth, 0);
}
