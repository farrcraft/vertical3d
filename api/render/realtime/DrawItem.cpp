/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "DrawItem.h"

namespace v3d::render::realtime {

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
firstInstance(0),
scissored(false),
scissor{} {
}

};  // namespace v3d::render::realtime
