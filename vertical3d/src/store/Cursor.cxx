/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Cursor.h"

#include <cstddef>
#include <span>

#include <glm/vec3.hpp>

namespace v3d::editor {

/**
 **/
Cursor::Cursor(std::span<const std::byte> bytes) :
    bytes_(bytes) {
}

/**
 **/
glm::vec3 Cursor::vector() {
    const float x = next<float>();
    const float y = next<float>();
    const float z = next<float>();
    return glm::vec3(x, y, z);
}

};  // namespace v3d::editor
