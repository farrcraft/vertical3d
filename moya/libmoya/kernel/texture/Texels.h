/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/kernel/Qualifiers.h>
#include <moya/libmoya/kernel/texture/Catalog.h>

#include <cstdint>
#include <type_traits>

#include <glm/vec4.hpp>

namespace v3d::moya::kernel::texture {

/**
 * A device's access to texture files: the catalog of what a scene's programs read, and one
 * texel of a texture's level, by its column and row from the level's top left. The column and row
 * lie within the level. A texel is four channels, as expanded() gives them. A texel's value never
 * depends on what the device holds in memory when it is read.
 **/
template <typename X>
concept Texels = requires(const X & texels, std::uint32_t texture, std::uint32_t level, std::uint32_t column, std::uint32_t row) {
    requires std::is_same_v<std::remove_cvref_t<decltype(texels.catalog)>, Catalog>;
    requires std::is_same_v<decltype(texels.texel(texture, level, column, row)), glm::vec4>;
};

/** The work a device's texture access may time, for a profiled render. **/
enum class Work : std::uint8_t {
    /** A material's program, with the texture lookups it makes. **/
    Program,
    /** One texture lookup. **/
    Lookup,
};

/**
 * A mark at the start of some work, from a texture access that times its work, or zero from one
 * that does not. finished() is given the mark back when the work ends.
 **/
template <Texels X>
MOYA_KERNEL std::uint64_t started(const X & texels) {
    if constexpr (requires { texels.started(); }) {
        return texels.started();
    } else {
        return 0;
    }
}

/** Tells a texture access that times its work that some work begun at a mark has ended. **/
template <Texels X>
MOYA_KERNEL void finished(const X & texels, Work work, std::uint64_t mark) {
    if constexpr (requires { texels.finished(work, mark); }) {
        texels.finished(work, mark);
    }
}

/**
 * A texel of one to four channels as four: grey as red, green and blue, and an alpha of one where
 * it has none. Two channels are grey and alpha.
 **/
MOYA_KERNEL inline glm::vec4 expanded(const float * values, std::uint32_t channels) {
    switch (channels) {
        case 1U:
            return glm::vec4 { values[0], values[0], values[0], 1.0F };
        case 2U:
            return glm::vec4 { values[0], values[0], values[0], values[1] };
        case 3U:
            return glm::vec4 { values[0], values[1], values[2], 1.0F };
        default:
            return glm::vec4 { values[0], values[1], values[2], values[3] };
    }
}

};  // namespace v3d::moya::kernel::texture
