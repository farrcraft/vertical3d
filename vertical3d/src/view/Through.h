/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/type/camera/Profile.h>
#include <vertical3d/src/scene/item/Camera.h>
#include <vertical3d/src/scene/Scene.h>

#include <string_view>

#include "ViewPort.h"

#include <boost/shared_ptr.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

namespace v3d::editor::view {

/**
 * What a layout names for a viewport's camera to have the view follow the scene's camera. Until
 * the scene has one, the view is a free perspective view.
 **/
constexpr std::string_view SCENE_CAMERA = "scene-camera";

/**
 * What a view shows when it looks through a scene camera: the profile it takes, and the part of
 * the view the render's picture fills.
 **/
struct Framed final {
    v3d::type::camera::Profile profile;
    glm::vec4 frame{ 0.0F };  /**< x, y, width and height, measured from the view's top left corner **/
};

/**
 * Frame a scene camera in a view, as the render's picture.
 *
 * The frame is the largest box of the picture's aspect that fits the view, centred in it. The
 * camera's lens covers the frame, so the profile widens the lens to the whole view: a view taller
 * than the frame sees further above and below it.
 *
 * @param lens the camera's lens, as an evaluation of the scene found it
 * @param world the camera's world matrix
 * @param view the view's own profile, whose size and pixel aspect the profile keeps
 * @param picture the render's picture size, in pixels
 **/
Framed framed(const scene::item::Camera::Lens& lens, const glm::mat4& world, const v3d::type::camera::Profile& view, const glm::uvec2& picture);

/**
 * @return the scene camera a view looks through: the one it names, or for a view that follows
 *         the scene's camera, the first selected camera, else the first camera in the scene.
 *         Nothing for a free view, or when that camera is not in the scene
 **/
boost::shared_ptr<scene::item::Camera> lookedThrough(const scene::Scene& scene, const ViewPort::Through& through);

/**
 * Set a view from the scene camera it looks through, framed to the render's picture, and named
 * for the camera. A view that names a camera the scene no longer holds is freed where it is. A
 * view that follows the scene's camera, in a scene with none, keeps its own camera until one is
 * made.
 **/
void follow(const scene::Scene& scene, ViewPort* view);

};  // namespace v3d::editor::view
