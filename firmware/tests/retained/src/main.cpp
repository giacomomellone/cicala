/* Reject corrupt RTC state before its lengths can index fixed arrays. */

#include <string.h>

#include <zephyr/ztest.h>

#include "retained.hpp"

namespace
{

tk::Retained populated()
{
    tk::Retained block{};

    block.bag.fingerprint = 0xABCD1234u;
    block.bag.recent_len = 3;
    block.bag.recent_next = 4;
    block.bag.recent[0] = 11;
    block.bag.recent[1] = 22;
    block.bag.recent[2] = 33;
    block.bag.drawn[2][0] = 0x0000'0005u;
    block.partial_since_full = 17;
    block.deck = 4;
    block.showing_question = true;
    block.seq = 99;

    return block;
}

bool is_zeroed(const tk::Retained &block)
{
    const uint8_t *bytes = reinterpret_cast<const uint8_t *>(&block);

    for (size_t i = 0; i < sizeof(block); i++) {
        if (bytes[i] != 0) {
            return false;
        }
    }

    return true;
}

} // namespace

ZTEST_SUITE(tk_retained, NULL, NULL, NULL, NULL, NULL);

ZTEST(tk_retained, test_a_cold_boot_is_not_mistaken_for_a_wake)
{
    tk::Retained block{};

    zassert_false(tk::retained_load(block), "an unstamped block is a cold boot");
    zassert_true(is_zeroed(block));
}

ZTEST(tk_retained, test_garbage_is_not_mistaken_for_a_wake)
{
    tk::Retained block;

    memset(&block, 0xAA, sizeof(block));

    zassert_false(tk::retained_load(block), "garbage must not validate");
    zassert_true(is_zeroed(block), "and it must be cleared, not left to be read");
    zassert_equal(block.bag.recent_len, 0);
    zassert_equal(block.bag.recent_next, 0);
}

ZTEST(tk_retained, test_a_sealed_block_survives_with_its_contents)
{
    tk::Retained block = populated();

    tk::retained_seal(block);

    const tk::Retained before = block;

    zassert_true(tk::retained_load(block), "a sealed block is a wake");
    zassert_mem_equal(&block, &before, sizeof(block), "and load() must not touch it");

    zassert_equal(block.bag.fingerprint, 0xABCD1234u);
    zassert_equal(block.bag.recent_len, 3);
    zassert_equal(block.bag.recent[1], 22);
    zassert_equal(block.bag.drawn[2][0], 0x0000'0005u);
    zassert_equal(block.partial_since_full, 17);
    zassert_equal(block.deck, 4);
    zassert_true(block.showing_question);
    zassert_equal(block.seq, 99);
}

ZTEST(tk_retained, test_sealing_twice_is_stable)
{
    tk::Retained block = populated();

    tk::retained_seal(block);

    const uint32_t first = block.hash;

    tk::retained_seal(block);

    zassert_equal(block.hash, first);
}

ZTEST(tk_retained, test_a_changed_payload_is_rejected)
{
    tk::Retained block = populated();

    tk::retained_seal(block);

    block.bag.drawn[3][1] ^= 1u;

    zassert_false(tk::retained_sealed(block), "the stamp must not still match");
    zassert_false(tk::retained_load(block));
    zassert_true(is_zeroed(block));
}

ZTEST(tk_retained, test_the_last_byte_is_covered)
{
    tk::Retained block = populated();

    tk::retained_seal(block);
    /* seq is the final field in the retained payload. */
    block.seq++;

    zassert_false(tk::retained_sealed(block));
}

ZTEST(tk_retained, test_a_different_layout_is_rejected)
{
    tk::Retained block = populated();

    tk::retained_seal(block);

    block.size = sizeof(block) - 4;

    zassert_false(tk::retained_load(block), "a block of the wrong shape is not state");
    zassert_true(is_zeroed(block));

    block = populated();
    tk::retained_seal(block);
    block.version++;

    zassert_false(tk::retained_load(block), "nor is one whose fields changed meaning");
    zassert_true(is_zeroed(block));
}

ZTEST(tk_retained, test_a_wrong_magic_is_rejected)
{
    tk::Retained block = populated();

    tk::retained_seal(block);
    block.magic ^= 1u;

    zassert_false(tk::retained_load(block));
    zassert_true(is_zeroed(block));
}

ZTEST(tk_retained, test_the_block_fits_the_rtc_budget)
{
    /* Leave headroom in the ESP32-S3's 8 KB RTC slow-memory region. */
    zassert_true(sizeof(tk::Retained) <= 1024, "the retained block has grown to %zu bytes",
                 sizeof(tk::Retained));
}
