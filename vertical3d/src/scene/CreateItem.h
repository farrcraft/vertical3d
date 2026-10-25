/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/type/camera/Profile.h>

#include "Camera.h"
#include "Light.h"

#include <boost/shared_ptr.hpp>
#include <glm/ext/quaternion_float.hpp>
#include <glm/mat4x4.hpp>

namespace v3d::editor {

/**
 * The rotation that turns an item's own axes to a view's. An item made from a view then faces
 * the way the view does, with its +z where the view looks and its +y the view's up.
 **/
glm::quat facing(const v3d::type::camera::Profile& view);

/**
 * A camera at a view's eye, facing the way the view looks.
 *
 * It takes the view's angle of view, its clipping distances and, as its extent, the view's
 * orthographic zoom. When they do not make a valid lens, the camera has the default lens.
 *
 * @param projection what the camera is, whatever the view is
 **/
boost::shared_ptr<Camera> create_camera(const v3d::type::camera::Profile& view, Camera::Projection projection);

/**
 * The camera profile a scene camera is seen through: where it is in the world, its projection
 * and its lens. It is the inverse of create_camera(), so a camera made from a view gives back
 * that view's eye, axes and lens.
 *
 * @param world the camera's world matrix, from an evaluation of its scene. Its scale is
 *        ignored
 * @param picture a profile whose size and pixel aspect the picture takes, since a scene camera
 *        holds neither
 **/
v3d::type::camera::Profile profile_of(const Camera& camera, const glm::mat4& world,
    const v3d::type::camera::Profile& picture);

/**
 * A light at a view's eye, facing the way the view looks, so a distant or spot light lights
 * what the view shows. Its emission is the RI shader's default for its type.
 **/
boost::shared_ptr<Light> create_light(const v3d::type::camera::Profile& view, Light::Type type);

};  // namespace v3d::editor
