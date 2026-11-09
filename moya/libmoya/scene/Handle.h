/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <cstdint>

namespace v3d::moya::scene {

/**
 * An object of the scene. A handle holds its object's kind, its index among the scene's objects,
 * and the generation of that index. An index is used again once its object is removed, with the
 * next generation, so the handle of a removed object matches nothing.
 **/
struct Handle {
    std::uint32_t kind { 0 };
    std::uint32_t index { 0 };

    /** Zero for a handle no scene gave. A scene's generations start at one. **/
    std::uint32_t generation { 0 };

    bool valid() const noexcept;
    bool operator==(const Handle & other) const = default;
};

};  // namespace v3d::moya::scene
