#include "retained.hpp"

#include <stddef.h>
#include <string.h>

namespace tk
{

namespace
{

// "TKR1" as a little-endian integer.
constexpr uint32_t kMagic = 0x3152'4B54u;

// Bump when the payload layout or field meaning changes.
constexpr uint16_t kVersion = 1;

// FNV-1a detects accidental RTC-memory corruption.
constexpr uint32_t kFnvOffset = 2166136261u;
constexpr uint32_t kFnvPrime = 16777619u;

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
