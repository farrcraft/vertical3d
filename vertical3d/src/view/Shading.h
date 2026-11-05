/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <cstdint>
#include <optional>
#include <string_view>

namespace v3d::editor::view {

/**
 * How a shaded view lights a mesh's surface: with each face's own normal, so the faces show,
 * or with a normal averaged at each vertex, so the surface looks curved.
 **/
enum class Shading : std::uint8_t { Flat, Smooth };

/**
 * @return the name a viewport's state gives a shading, "flat" or "smooth"
 **/
std::string_view name(Shading shading) noexcept;

/**
 * @return the shading a name gives, or nothing for a name that is neither
 **/
std::optional<Shading> shading(std::string_view name) noexcept;

};  // namespace v3d::editor::view
