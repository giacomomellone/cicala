#pragma once

#include "session.hpp"

namespace cicala
{
struct Snapshot {
    uint32_t generation;
    PlayState play;
    Bag::State bag;
};
// Header, configuration, play state, bag state, checksum. No C++ padding on disk.
constexpr size_t kSnapshotBytes =
    4 + 4 + 2 + 1 + 2 + 8 + kMaxQuestionBytes + 4 + 2 * kRecentRing + 4 + 4 * kBitmapWords + 4;
bool snapshot_encode(const Snapshot &snapshot, uint8_t *out, size_t capacity);
/** Leaves out untouched on a rejected record. */
bool snapshot_decode(const uint8_t *data, size_t size, Snapshot &out);
bool generation_newer(uint32_t a, uint32_t b);
} // namespace cicala
