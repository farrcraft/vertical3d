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

namespace v3d::render::realtime::vulkan::renderer {

/**
 * Cut an item down to a canvas's clip rectangle - min x, min y, max x, max y, in the pixels of
 * the image drawn into, the unit a scissor uses. What lies off the top or left
 * of the image is dropped, and a rectangle turned inside out cuts the draw to nothing.
 **/
inline void clip(DrawItem* item, const glm::vec4& rectangle) {
    const float left = std::max(rectangle.x, 0.0f);
    const float top = std::max(rectangle.y, 0.0f);
    item->scissored = true;
    item->scissor.offset.x = static_cast<int32_t>(left);
    item->scissor.offset.y = static_cast<int32_t>(top);
    item->scissor.extent.width = static_cast<uint32_t>(std::max(rectangle.z - left, 0.0f));
    item->scissor.extent.height = static_cast<uint32_t>(std::max(rectangle.w - top, 0.0f));
}

};  // namespace v3d::render::realtime::vulkan::renderer
