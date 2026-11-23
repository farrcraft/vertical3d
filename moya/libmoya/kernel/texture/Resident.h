/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/kernel/Array.h>
#include <moya/libmoya/kernel/Qualifiers.h>
#include <moya/libmoya/kernel/texture/Catalog.h>
#include <moya/libmoya/kernel/texture/Texture.h>

#include <cstdint>

#include <glm/vec4.hpp>

namespace v3d::moya::kernel::texture {

/**
 * Textures held whole in device memory, every level of each. It is the oracle every device's
 * texel access is checked against.
 *
 * - texels holds each texture's levels, the first level first, each row after row from its top,
 *   every texel as its four channels.
 * - starts holds where each texture's first level begins among texels.
 **/
struct Resident {
    Catalog catalog;
    Array<glm::vec4> texels;
    Array<std::uint32_t> starts;

    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the texture, its level, then the texel
    MOYA_KERNEL glm::vec4 texel(std::uint32_t texture, std::uint32_t level, std::uint32_t column, std::uint32_t row) const {
        const Texture & held = catalog.textures[texture];
        std::uint32_t at = starts[texture];
        for (std::uint32_t above = 0; above < level; ++above) {
            at += held.widthAt(above) * held.heightAt(above);
        }
        return texels[at + (row * held.widthAt(level)) + column];
    }
};

};  // namespace v3d::moya::kernel::texture
