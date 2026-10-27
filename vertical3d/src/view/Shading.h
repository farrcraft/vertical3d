/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <cstdint>

namespace v3d::editor {

/**
 * How a shaded view lights a mesh's surface: with each face's own normal, so the faces show,
 * or with a normal averaged at each vertex, so the surface looks curved.
 **/
enum class Shading : std::uint8_t { Flat, Smooth };

};  // namespace v3d::editor
