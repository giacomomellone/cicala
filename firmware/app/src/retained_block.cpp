/* The retained block, placed in RTC slow memory. */

#include "retained_block.hpp"

namespace
{

#ifdef CONFIG_SOC_SERIES_ESP32S3
#define KVELD_RTC_NOINIT __attribute__((section(".rtc_noinit")))
#else
#define KVELD_RTC_NOINIT
#endif

KVELD_RTC_NOINIT kveld::Retained block;

bool checked;
bool survived;

void check_once()
{
    if (checked) {
        return;
    }

    checked = true;
    survived = kveld::retained_load(block);
}

} // namespace

kveld::Retained &kveld_retained()
{
    check_once();

    return block;
}

bool kveld_retained_survived()
{
    check_once();

    return survived;
}

void kveld_retained_seal()
{
    /* Validate first even here. */
    check_once();

    kveld::retained_seal(block);
}
