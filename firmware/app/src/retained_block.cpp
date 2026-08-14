/* The retained block, placed in RTC slow memory. */

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
    /* Validate first even here. */
    check_once();

    tk::retained_seal(block);
}
