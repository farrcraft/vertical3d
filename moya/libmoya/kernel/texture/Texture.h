/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/kernel/Qualifiers.h>

#include <cstdint>

namespace v3d::moya::kernel::texture {

/** The texture a lookup finds nowhere, as a missing UDIM tile is. **/
inline constexpr std::uint32_t kNoTexture = 0xFFFFFFFFU;

/**
 * One texture file: its first level's size and its count of levels. Each level is half the one
 * above along each side, rounded down and never below one texel.
 **/
struct Texture {
    std::uint32_t width { 0 };
    std::uint32_t height { 0 };
    std::uint32_t levels { 0 };

    /** A level's width. **/
    MOYA_KERNEL std::uint32_t widthAt(std::uint32_t level) const {
        const std::uint32_t side = level < 32U ? width >> level : 0U;
        return side > 0U ? side : 1U;
    }

    /** A level's height. **/
    MOYA_KERNEL std::uint32_t heightAt(std::uint32_t level) const {
        const std::uint32_t side = level < 32U ? height >> level : 0U;
        return side > 0U ? side : 1U;
    }
};

};  // namespace v3d::moya::kernel::texture
