/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/type/camera/Camera.h>

#include <boost/shared_ptr.hpp>
#include <glm/vec4.hpp>

namespace v3d::editor {

/**
 * What a manipulator is seen through: a camera, and the rectangle of the window it draws into.
 *
 * The region is x, y, width and height in window pixels, as a view's region is. A view gives a
 * manipulator its own camera and region, so a manipulator needs nothing else of the view.
 **/
struct Screen {
    boost::shared_ptr<v3d::type::camera::Camera> camera;
    glm::vec4 region{ 0.0F };
};

};  // namespace v3d::editor
