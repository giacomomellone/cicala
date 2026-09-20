/* Validated state stored in ESP32-S3 RTC slow memory across deep sleep. */

#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "session.hpp"

namespace cicala
{

/** State preserved across a deep-sleep wake. */
struct Retained {
    // `hash` covers every byte after this header.
    uint32_t magic;
    uint16_t version;
    uint16_t size;
    uint32_t hash;

    /** Draw-without-repeats state. See lib/qdb. */
    Bag::State bags[3];

    /** Partial refreshes since the last full one. See app/src/panel.cpp. */
    uint16_t partial_since_full;

    PlayState play;

    /** `seq` of the last question published, so a stale render is detectable. */
    uint32_t seq;
};

/** Validate `block`. Zero it and return false when its stamp is invalid. */
bool retained_load(Retained &block);

/** Update the header and payload hash. */
void retained_seal(Retained &block);

/** True when `block` currently carries a valid stamp. Does not modify it. */
bool retained_sealed(const Retained &block);

} // namespace cicala
