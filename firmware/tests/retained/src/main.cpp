/*
 * The stamp that tells a wake from a cold boot.
 *
 * None of these cases can be produced on a board on purpose. A cold boot means
 * pulling the cell, a corrupted block means catching a brownout mid-write, and
 * a layout change means flashing two firmwares in sequence and hoping the
 * struct moved the way you expected. On the host they are all just bytes.
 *
 * What makes it worth testing at all is the consequence of getting it wrong.
 * `Bag::State` carries `recent_len` and `recent_next`, which index fixed
 * arrays with no bounds checks of their own — see lib/qdb — so a block of
 * garbage accepted as state is an out-of-bounds read and an out-of-bounds
 * write, not a wrong question.
 */

#include <string.h>

#include <zephyr/ztest.h>

#include "retained.hpp"

namespace
{

/** A block with something recognisable in every field the stamp covers. */
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
    /* What .bss looks like on a platform with no RTC domain, and what RTC
     * memory looks like after the cell has been out. */
    tk::Retained block{};

    zassert_false(tk::retained_load(block), "an unstamped block is a cold boot");
    zassert_true(is_zeroed(block));
}

ZTEST(tk_retained, test_garbage_is_not_mistaken_for_a_wake)
{
    /*
     * The case that matters most: uninitialised RTC memory is not zeroed, it
     * is whatever was there. Accepting it would hand lib/qdb a recent_len of
     * 0xAA against a 20-entry array.
     */
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
    /* The device seals after every render, so the stamp has to be a function
     * of the contents rather than of how many times it has been applied. */
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

    /* One bit, in the middle of the bag — a brownout halfway through a write
     * looks like this. */
    block.bag.drawn[3][1] ^= 1u;

    zassert_false(tk::retained_sealed(block), "the stamp must not still match");
    zassert_false(tk::retained_load(block));
    zassert_true(is_zeroed(block));
}

ZTEST(tk_retained, test_the_last_byte_is_covered)
{
    /*
     * A hash that stopped one field short would pass everything above. `seq`
     * is last in the struct, so this is the case that catches an off-by-one in
     * the length the stamp covers.
     */
    tk::Retained block = populated();

    tk::retained_seal(block);
    block.seq++;

    zassert_false(tk::retained_sealed(block));
}

ZTEST(tk_retained, test_a_different_layout_is_rejected)
{
    tk::Retained block = populated();

    tk::retained_seal(block);

    /* What a firmware update that added a field to the struct leaves behind. */
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
    /*
     * 8 KB of RTC slow memory, shared with whatever the SoC and the bootloader
     * keep there — the build reported 36 bytes in use before this block
     * existed. This is a long way from the ceiling and should stay that way;
     * the assertion is here so that adding a field is a decision rather than
     * an accident.
     */
    zassert_true(sizeof(tk::Retained) <= 1024, "the retained block has grown to %zu bytes",
                 sizeof(tk::Retained));
}
