/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
**/

#pragma once

#include <cstdint>

namespace v3d::brep {

/**
 * How a mesh names one of its own vertices, half edges or faces.
 *
 * One type for all three, because they are all offsets into a BRep's arrays and a
 * conversion between two of them is never meaningful. It is 32 bits: a half edge holds
 * four of these and a mesh is mostly half edges.
 **/
using Index = uint32_t;

/**
 * The index a half edge, face or vertex reference carries when it points at nothing.
 *
 * The value is part of the project file format, which stores a reference that may be
 * absent as this sentinel. Changing it breaks every project file already written.
 **/
constexpr Index INVALID_ID = (1u << 31);

};  // namespace v3d::brep
