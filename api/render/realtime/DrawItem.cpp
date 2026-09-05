/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "DrawItem.h"

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
    return (static_cast<uint64_t>(layer) << 48) |
        (static_cast<uint64_t>(pipeline) << 32) |
        (static_cast<uint64_t>(material) << 16) |
        static_cast<uint64_t>(depth);
}

/**
 **/
bool SortKey::operator<(const SortKey& other) const noexcept {
    return packed() < other.packed();
}

/**
 **/
DrawItem::DrawItem() noexcept :
    vertexBuffer(VK_NULL_HANDLE),
    vertexBufferOffset(0),
    indexBuffer(VK_NULL_HANDLE),
    indexBufferOffset(0),
    indexType(VK_INDEX_TYPE_UINT32),
push{},
pushSize(0),
vertices(0),
firstVertex(0),
indices(0),
firstIndex(0),
instances(1),
firstInstance(0) {
}

};  // namespace v3d::render::realtime
