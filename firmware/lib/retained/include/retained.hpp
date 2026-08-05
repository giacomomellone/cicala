/*
 * State that has to survive the reboot deep sleep really is.
 *
 * On the ESP32-S3 deep sleep does not preserve SRAM, so every wake runs
 * `main()` from the top with `.bss` freshly zeroed. Anything the device is
 * supposed to remember between presses therefore lives in RTC slow memory,
 * which keeps its contents across sleep and loses them when the cell is
 * removed or goes flat.
 *
 * That leaves one question this header exists to answer: *did it survive?*
 * RTC memory holds whatever it held, including uninitialised garbage on a
 * cold power-on, and the difference is not observable from the contents
 * themselves. Reading garbage as state is not a cosmetic problem — `Bag::State`
 * carries `recent_len` and `recent_next`, which index fixed arrays without
 * their own bounds checks, so a garbage value reads and writes past the end of
 * the ring.
 *
 * The block is therefore stamped: a magic number, a layout version, its own
 * size, and a hash of everything after the header. All four have to agree
 * before the contents are used, and `load()` zeroes the block when they do
 * not. A cold boot, a firmware whose struct changed shape, and a brownout
 * halfway through a write all land in the same safe place.
 *
 * No Zephyr headers, so the suite exercises every one of those cases on the
 * host. Where the block physically lives is app/src/retained.cpp.
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "qdb.hpp"

namespace tk
{

/**
 * Everything the device remembers across a wake.
 *
 * Deliberately small: it competes for 8 KB of RTC slow memory with whatever
 * the SoC and the bootloader keep there. `Bag::State` dominates it at about
 * 430 bytes.
 */
struct Retained {
    /*
     * Header. `hash` covers every byte after it; `magic`, `version` and `size`
     * are checked against their compile-time values instead, which is a
     * stronger test than hashing them would be.
     */
    uint32_t magic;
    uint16_t version;
    uint16_t size;
    uint32_t hash;

    /** Draw-without-repeats state. See lib/qdb. */
    Bag::State bag;

    /** Partial refreshes since the last full one. See app/src/panel.cpp. */
    uint16_t partial_since_full;

    /** The deck the question on the glass was drawn from. */
    uint8_t deck;

    /**
     * The deck Category has advanced to.
     *
     * A button has no position to read, so unlike the rotary selector it
     * replaced, the active deck has to be remembered. On a cold boot the
     * zeroed block makes that New People, which is the documented default.
     */
    uint8_t active_deck;

    /**
     * What is on the glass.
     *
     * A deck name is not a question: waking to one means the user turned the
     * selector and never pressed, so the panel is mid-conversation with them
     * and the name has to stay. `AppIo::retained_matches()` reports only a
     * question, because only a question makes a boot free.
     */
    bool showing_question;

    /** `seq` of the last question published, so a stale render is detectable. */
    uint32_t seq;
};

/**
 * Validate `block`, or zero it.
 *
 * @return true when the block survived intact — a wake. False means a cold
 *         boot, a layout change or a corrupted write, and the block has been
 *         zeroed, which is the state a first boot starts from anyway.
 */
bool retained_load(Retained &block);

/**
 * Stamp `block` so the next boot accepts it.
 *
 * Every field above is written by ordinary code that knows nothing about the
 * header, so the stamp has to be reapplied afterwards. Sealing late rather
 * than on every write is deliberate: a block left unsealed by a crash is
 * discarded on the next boot, which is the direction that cannot corrupt
 * anything.
 */
void retained_seal(Retained &block);

/** True when `block` currently carries a valid stamp. Does not modify it. */
bool retained_sealed(const Retained &block);

} // namespace tk
