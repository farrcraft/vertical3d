/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/kernel/Qualifiers.h>
#include <moya/libmoya/kernel/texture/Catalog.h>

#include <cstdint>

#include <glm/vec4.hpp>

namespace v3d::moya::kernel::texture {

/** No textures: an empty catalog, so every lookup gives its default. **/
struct Untextured {
    Catalog catalog;

    MOYA_KERNEL static glm::vec4 texel(std::uint32_t /*texture*/, std::uint32_t /*level*/, std::uint32_t /*column*/, std::uint32_t /*row*/) {
        return glm::vec4 { 0.0F };
    }
};

};  // namespace v3d::moya::kernel::texture
