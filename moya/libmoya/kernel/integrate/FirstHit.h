/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/kernel/film/Combine.h>

#include <cstdint>

namespace v3d::moya::kernel::integrate {

/**
 * A value a camera ray reads where it first meets the scene. A ray that meets nothing reads an
 * infinite depth, an instance and a face of -1, and zero for the others.
 *
 * - Depth is the hit's depth along the camera's view.
 * - Position is the hit's point in the world.
 * - Normal is the shading normal in the world, on the side its geometric normal faces.
 * - Texture is the `st` primvar, or the hit's place on its primitive where there is none.
 * - Instance is the innermost instance that placed the hit, or -1 for a surface no instance
 *   places.
 * - Face is the face or curve of the mesh the hit's primitive was cut from.
 * - Primvar is a primvar's value by its number, zero where the surface has none.
 *
 * A light's shape the ray meets has its depth and position, and reads zero or -1 for the others.
 **/
enum class Reading : std::uint8_t {
    Depth,
    Position,
    Normal,
    Texture,
    Instance,
    Face,
    Primvar,
};

/** A channel the path tracer fills at each camera ray's first hit, and what it reads there. **/
struct FirstHit {
    /** Where the channel's first float sits in a pixel, and its components, from 1 to 4. **/
    std::uint32_t offset { 0 };
    std::uint32_t components { 1 };

    /** The number of the primvar it reads, for a Primvar. **/
    std::uint32_t primvar { 0 };

    Reading reading { Reading::Depth };
    film::Combine combine { film::Combine::Mean };
};

};  // namespace v3d::moya::kernel::integrate
