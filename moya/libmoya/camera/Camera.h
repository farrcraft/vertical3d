/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/kernel/camera/Camera.h>
#include <moya/libmoya/scene/Scene.h>

/** The camera a render looks through, built from the scene for kernel code. **/
namespace v3d::moya::camera {

/**
 * The camera the scene's options name, as kernel code reads it, with RI's derivations.
 *
 * The frame's aspect is the resolution's, times the pixel aspect, while the options give none.
 * While the camera gives no screen window, a frame at least as wide as it is tall spans -aspect to
 * aspect across and -1 to 1 down. A taller one spans -1 to 1 across and -1 / aspect to 1 / aspect
 * down.
 *
 * The lens's radius is the focal length over twice the f-stop. It is zero, a pinhole, unless the
 * f-stop, the focal length and the focal distance are all positive and finite. The transformation
 * is its first sample.
 *
 * Options that name no camera take every camera parameter's fallback, which is RI's default
 * camera: orthographic, at the origin. A scene with no options takes their fallbacks too.
 **/
kernel::camera::Camera fromScene(const scene::Scene & scene);

/**
 * The camera a surface is measured from to be tessellated: the one the options name as their
 * dicing camera, with the options' resolution and frame, or the render camera while they name
 * none.
 **/
kernel::camera::Camera dicingFromScene(const scene::Scene & scene);

};  // namespace v3d::moya::camera
