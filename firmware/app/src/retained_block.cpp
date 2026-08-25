/* The retained block, placed in RTC slow memory. */

#include "retained_block.hpp"

namespace
{

#ifdef CONFIG_SOC_SERIES_ESP32S3
#define CICALA_RTC_NOINIT __attribute__((section(".rtc_noinit")))
#else
#define CICALA_RTC_NOINIT
#endif

CICALA_RTC_NOINIT cicala::Retained block;

bool checked;
bool survived;

void check_once()
{
    if (checked) {
        return;
    }

    checked = true;
    survived = cicala::retained_load(block);
}

} // namespace

cicala::Retained &cicala_retained()
{
    check_once();

    return block;
}

bool cicala_retained_survived()
{
    check_once();

    return survived;
}

void cicala_retained_seal()
{
    /* Validate first even here. */
    check_once();

    cicala::retained_seal(block);
}
