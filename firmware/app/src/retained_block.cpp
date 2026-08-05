/*
 * The retained block, placed in RTC slow memory.
 *
 * Two things make this work, and both are properties of the section rather
 * than of any code here:
 *
 *   - `.rtc_noinit` lands in `rtc_slow_seg`, the 8 KB the ESP32-S3 keeps
 *     powered through deep sleep. Ordinary `.bss` is in SRAM, which is not.
 *   - It is a NOLOAD section that nothing zeroes at startup. `.rtc.bss` would
 *     also survive the sleep and then be cleared on the way back up, which
 *     retains the memory and discards the point of it.
 *
 * Everywhere else the section does not exist — qemu, native_sim, any host
 * build — the block is ordinary zeroed `.bss`. Every boot then looks like a
 * cold one, which is what those platforms should report: they have no RTC
 * domain to survive in. The suite covers the surviving case directly instead,
 * by sealing a block and reading it back.
 */

#include "retained_block.hpp"

namespace
{

#ifdef CONFIG_SOC_SERIES_ESP32S3
#define TK_RTC_NOINIT __attribute__((section(".rtc_noinit")))
#else
#define TK_RTC_NOINIT
#endif

TK_RTC_NOINIT tk::Retained block;

bool checked;
bool survived;

void check_once()
{
    if (checked) {
        return;
    }

    checked = true;
    survived = tk::retained_load(block);
}

} // namespace

tk::Retained &tk_retained()
{
    check_once();

    return block;
}

bool tk_retained_survived()
{
    check_once();

    return survived;
}

void tk_retained_seal()
{
    /*
     * Validate first even here. Sealing a block nobody has checked would stamp
     * whatever garbage a cold boot left in RTC memory, turning it into state
     * the next boot trusts — the one outcome the stamp exists to prevent.
     */
    check_once();

    tk::retained_seal(block);
}
