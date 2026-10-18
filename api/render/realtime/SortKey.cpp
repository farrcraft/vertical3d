/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "SortKey.h"

#include <cstdint>

namespace v3d::render::realtime {

/**
 **/
SortKey::SortKey() noexcept :
    layer(0),
    pipeline(0),
    material(0),
    depth(0) {
}

/**
 **/
uint64_t SortKey::packed() const noexcept {
    // four fields of equal width, the layer most significant
    constexpr int fieldBits = 16;
    return (static_cast<uint64_t>(layer) << (3 * fieldBits)) |
        (static_cast<uint64_t>(pipeline) << (2 * fieldBits)) |
        (static_cast<uint64_t>(material) << fieldBits) |
        static_cast<uint64_t>(depth);
}

/**
 **/
bool SortKey::operator<(const SortKey& other) const noexcept {
    return packed() < other.packed();
}

};  // namespace v3d::render::realtime
