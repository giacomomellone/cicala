#include "retained.hpp"

#include <stddef.h>
#include <string.h>

namespace tk
{

namespace
{

/*
 * "TKR1" read as little-endian bytes. Any constant would do; a printable one
 * makes the block findable in a memory dump, which is the only way anyone will
 * ever look at it.
 */
constexpr uint32_t kMagic = 0x3152'4B54u;

/*
 * Bump whenever the layout of anything below the header changes. `size` catches
 * most of that on its own, but not a field that changed meaning while keeping
 * its width, which is exactly the change that would otherwise be read as valid.
 */
constexpr uint16_t kVersion = 1;

/* Same hash the bundle fingerprint uses, for the same reason: short, no table,
 * and strong enough for a corruption check that is not defending against an
 * attacker with write access to RTC memory. */
constexpr uint32_t kFnvOffset = 2166136261u;
constexpr uint32_t kFnvPrime = 16777619u;

/** Everything after the header, which is what the header describes. */
constexpr size_t kPayloadOffset = offsetof(Retained, bag);

uint32_t payload_hash(const Retained &block)
{
    const uint8_t *bytes = reinterpret_cast<const uint8_t *>(&block) + kPayloadOffset;
    uint32_t hash = kFnvOffset;

    for (size_t i = 0; i < sizeof(Retained) - kPayloadOffset; i++) {
        hash ^= bytes[i];
        hash *= kFnvPrime;
    }

    return hash;
}

} // namespace

bool retained_sealed(const Retained &block)
{
    if (block.magic != kMagic || block.version != kVersion || block.size != sizeof(Retained)) {
        return false;
    }

    return block.hash == payload_hash(block);
}

bool retained_load(Retained &block)
{
    if (retained_sealed(block)) {
        return true;
    }

    memset(&block, 0, sizeof(block));

    return false;
}

void retained_seal(Retained &block)
{
    block.magic = kMagic;
    block.version = kVersion;
    block.size = sizeof(Retained);
    block.hash = payload_hash(block);
}

} // namespace tk
