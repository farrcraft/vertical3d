/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Bytes.h"

#include <api/brep/Index.h>

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <vector>

#include <glm/vec3.hpp>

namespace v3d::editor {

/**
 **/
void put(std::vector<std::byte>* bytes, const glm::vec3& value) {
    put(bytes, value.x);
    put(bytes, value.y);
    put(bytes, value.z);
}

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

/**
 **/
bool sized(const Blob& blob, std::size_t size, const char* role, std::string* problem) {
    if (blob.count < 0 || blob.count >= static_cast<std::int64_t>(v3d::brep::INVALID_ID) ||
        static_cast<std::size_t>(blob.count) * size != blob.data.size()) {
        *problem = std::string("its ") + role + " do not fill their blob exactly";
        return false;
    }
    return true;
}

/**
 **/
bool finite(const glm::vec3& value) {
    return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z);
}

};  // namespace v3d::editor
