/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/kernel/texture/Catalog.h>

#include <cstdint>

#include <glm/vec4.hpp>

namespace v3d::moya::device::cpu {

/**
 * The CPU's texture access: the catalog, and a host function that reads one texel through the
 * host's tile cache, loading the tile when the cache does not hold it. The host function never
 * throws. A texel it cannot read is zero, and the host reports it after the render.
 * libmoya fills both and passes them to a library's entry point. The function is libmoya's, so a
 * library calls it by its address and compiles none of it.
 **/
struct Textures {
    kernel::texture::Catalog catalog;

    /** What fetch reads through: the host's cache. **/
    void * cache { nullptr };

    /** Writes a texel's four channels to out, as kernel::texture::expanded() gives them. **/
    void (*fetch)(void * cache, std::uint32_t texture, std::uint32_t level, std::uint32_t column, std::uint32_t row, float * out) noexcept { nullptr };

    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the texture, its level, then the texel
    glm::vec4 texel(std::uint32_t texture, std::uint32_t level, std::uint32_t column, std::uint32_t row) const {
        float out[4] {};
        fetch(cache, texture, level, column, row, out);
        return glm::vec4 { out[0], out[1], out[2], out[3] };
    }
};

};  // namespace v3d::moya::device::cpu
