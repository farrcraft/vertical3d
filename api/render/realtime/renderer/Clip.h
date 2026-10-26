/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/render/realtime/DrawItem.h>

#include <vulkan/vulkan.h>

#include <algorithm>
#include <cstdint>

#include <glm/glm.hpp>

namespace v3d::render::realtime::renderer {

/**
 * Clip an item to a canvas's clip rectangle: min x, min y, max x, max y, in pixels of the image
 * drawn into, the unit a scissor uses. What lies off the top or left of the image is dropped,
 * and a rectangle whose max is below its min clips the draw to nothing. A NaN left or top edge
 * is taken as the image's edge, and a NaN right or bottom edge clips the draw to nothing. Every
 * edge is held within 2^24 pixels, far past any image, so each converts to an integer exactly.
 **/
inline void clip(DrawItem* item, const glm::vec4& rectangle) {
    const float limit = 16777216.0F;
    // std::max(a, b) returns a when a comparison with NaN is false, so naming the bound first
    // turns a NaN edge into the bound
    const float left = std::min(std::max(0.0F, rectangle.x), limit);
    const float top = std::min(std::max(0.0F, rectangle.y), limit);
    const float right = std::min(std::max(left, rectangle.z), limit);
    const float bottom = std::min(std::max(top, rectangle.w), limit);
    item->scissored = true;
    item->scissor.offset.x = static_cast<int32_t>(left);
    item->scissor.offset.y = static_cast<int32_t>(top);
    item->scissor.extent.width = static_cast<uint32_t>(right - left);
    item->scissor.extent.height = static_cast<uint32_t>(bottom - top);
}

};  // namespace v3d::render::realtime::renderer
