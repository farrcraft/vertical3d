/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/type/Flags.h>

#include <cstdint>

namespace v3d::engine {
/**
 * The portions of the engine an app opts into.
 **/
enum class Feature : uint32_t {
    Window = (1 << 0),
    Config = (1 << 1),
    MouseInput = (1 << 2),
    KeyboardInput = (1 << 3)
};

using Features = v3d::type::Flags<Feature>;

constexpr Features operator|(Feature lhs, Feature rhs) noexcept {
    return Features(lhs) | rhs;
}

};  // namespace v3d::engine
