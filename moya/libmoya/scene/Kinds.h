/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/scene/Schema.h>

namespace v3d::moya::scene {

/**
 * The kinds of object moya renders, which a scene of moya's is made with:
 *
 * - **camera**: a projection, a field of view, a screen window, clipping, depth of field, a
 *   shutter, and its transformation from camera space to world space over the shutter.
 * - **options**: the picture's resolution and aspect, the crop window, samples per pixel, the
 *   pixel filter, the frame number the sampler is seeded by, and the camera the render looks
 *   through.
 * - **channel**: what a film channel holds, and how many floats.
 * - **output**: a file, its format, the channels it writes beside the beauty, their precision and
 *   their compression.
 *
 * Every default is RI's where RI names one. A parameter whose RI default depends on another, as
 * the screen window depends on the frame's aspect, is empty or zero until a client sets it, and is
 * then derived as RI derives it.
 **/
Schema kinds();

};  // namespace v3d::moya::scene
