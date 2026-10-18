/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

namespace v3d::render::offline {

/** Pi, the radians in half a turn. **/
constexpr float PI = 3.14159265358979323846f;

/** The degrees in half a turn. An angle in degrees times PI over this is the angle in radians. **/
constexpr float HALF_TURN_DEGREES = 180.0f;

/** The byte that holds a channel value of one in an 8-bit image. **/
constexpr float BYTE_SCALE = 255.0f;

};  // namespace v3d::render::offline
