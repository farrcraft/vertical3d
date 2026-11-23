/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include "Shading.h"

#include <optional>
#include <string_view>

namespace v3d::editor::view {

/**
 **/
std::string_view name(Shading shading) noexcept {
    switch (shading) {
        case Shading::Smooth:
            return "smooth";
        case Shading::Rendered:
            return "rendered";
        case Shading::Flat:
            break;
    }
    return "flat";
}

/**
 **/
std::optional<Shading> shading(std::string_view name) noexcept {
    if (name == "flat") {
        return Shading::Flat;
    }
    if (name == "smooth") {
        return Shading::Smooth;
    }
    if (name == "rendered") {
        return Shading::Rendered;
    }
    return std::nullopt;
}

};  // namespace v3d::editor::view
