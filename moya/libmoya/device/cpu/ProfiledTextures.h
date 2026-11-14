/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/device/cpu/Tally.h>
#include <moya/libmoya/device/cpu/Textures.h>
#include <moya/libmoya/kernel/texture/Catalog.h>
#include <moya/libmoya/kernel/texture/Texels.h>

#include <cstdint>

#include <glm/vec4.hpp>

namespace v3d::moya::device::cpu {

/**
 * The CPU's texture access, adding the ticks each material's program and each texture lookup
 * takes to one pixel's tally, as Profiled adds its traversal's.
 *
 * Only the CPU's libraries define started() and finished().
 **/
struct ProfiledTextures {
    kernel::texture::Catalog catalog;
    Textures textures;
    Tally * tally { nullptr };

    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the texture, its level, then the texel
    glm::vec4 texel(std::uint32_t texture, std::uint32_t level, std::uint32_t column, std::uint32_t row) const {
        return textures.texel(texture, level, column, row);
    }

    std::uint64_t started() const;
    void finished(kernel::texture::Work work, std::uint64_t mark) const;
};

};  // namespace v3d::moya::device::cpu
