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
 *   shutter, and its transformation from camera space to world space over the shutter. Camera
 *   space is RI's: x to the right, y up, looking down z.
 * - **options**: the picture's resolution and aspect, the crop window, samples per pixel and the
 *   noise they may stop at, and the pixel filter. Also the frame number the sampler is seeded
 *   by, and the camera the render looks through.
 * - **channel**: what a film channel holds, and how many floats.
 * - **output**: a file, its format, the channels it writes beside the beauty, their precision and
 *   their compression.
 * - **mesh**, **subdivision**, **curves** and **points**: geometry prototypes, each holding its
 *   points over the shutter, its topology and its primvars. A subdivision mesh adds its scheme,
 *   creases, corners, holes and boundary rule. Curves add their type, basis, wrap, widths and
 *   profile, and points add their widths and profile.
 * - **sphere**, **cone**, **cylinder**, **hyperboloid**, **paraboloid**, **disk** and **torus**:
 *   RI's quadrics, each with RI's dimensions, which may move over the shutter. RI gives them no
 *   defaults, so each is a unit shape until a client sets it.
 * - **group**: a prototype made of the instances that name it as their group.
 * - **instance**: a prototype placed by a transformation over the shutter, with its visibility to
 *   camera rays, shadow rays and indirect rays. An instance in no group is in the world. An
 *   instance of a group places every instance in it, so instancing nests. It names its material,
 *   and one in a group that names none takes the material of the instance that places the group.
 * - **shader**, **connection** and **material**: a material's network. A shader node names its
 *   shader and its language, and declares its inputs as primvars of constant interpolation. A
 *   connection feeds one node's output to another's input. A material names the node and output
 *   for its surface and for its displacement.
 * - **rectLight**, **diskLight**, **sphereLight**, **cylinderLight**, **meshLight**,
 *   **distantLight** and **domeLight**: UsdLux's lights, with UsdLux's parameters and defaults.
 *   Each lights every instance while its includeRoot is 1, and otherwise only the instances it
 *   names, which may be none. Every instance casts its shadows while its shadowIncludeRoot is 1,
 *   and otherwise only the instances its shadowInstances names. A mesh light takes its shape
 *   from a geometry prototype.
 *
 * Every default is RI's where RI names one. A parameter whose RI default depends on another, as
 * the screen window depends on the frame's aspect, is empty or zero until a client sets it, and is
 * then derived as RI derives it.
 **/
Schema kinds();

};  // namespace v3d::moya::scene
