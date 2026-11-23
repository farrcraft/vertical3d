/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <cstdint>
#include <optional>
#include <string_view>

namespace v3d::editor::view {

/**
 * How a view shows a mesh's surface. Flat lights it with each face's own normal, so the faces
 * show, and smooth with a normal averaged at each vertex, so the surface looks curved. Rendered
 * shows moya's picture of the scene from the view's camera, which a render session of the view's
 * own makes as the view is used.
 **/
enum class Shading : std::uint8_t { Flat, Smooth, Rendered };

/**
 * @return the name a viewport's state gives a shading, "flat", "smooth" or "rendered"
 **/
std::string_view name(Shading shading) noexcept;

/**
 * @return the shading a name gives, or nothing for a name that is neither
 **/
std::optional<Shading> shading(std::string_view name) noexcept;

};  // namespace v3d::editor::view
