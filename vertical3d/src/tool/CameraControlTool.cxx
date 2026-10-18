/**
 * Vertical3D
 * Copyright(c) 2021 Joshua Farr(josh@farrcraft.com)
**/

#include "CameraControlTool.h"

#include <api/type/camera/Camera.h>
#include <vertical3d/src/view/ViewPort.h>

#include <string>

namespace v3d::editor {

namespace {

/**
 * World units a perspective camera moves for each pixel the mouse is dragged.
 **/
constexpr float perspectiveFactor = 0.125F;

};  // namespace

/**
 **/
CameraControlTool::CameraControlTool() :
    last_(0.0F, 0.0F),
    mode_(CAMERA_MODE_NONE),
    dragging_(false) {
}

/**
 **/
void CameraControlTool::activate(const std::string& name) {
    if (name == "zoom") {
        mode_ = CAMERA_MODE_ZOOM;
    } else if (name == "truck") {
        mode_ = CAMERA_MODE_TRUCK;
    } else if (name == "pan") {
        mode_ = CAMERA_MODE_PAN;
    }
}

/**
 **/
void CameraControlTool::deactivate(const std::string& /* name */) {
    mode_ = CAMERA_MODE_NONE;
}

/**
 **/
void CameraControlTool::view(const boost::shared_ptr<ViewPort>& view) {
    view_ = view;
    if (view_) {
        arcball_.bounds(view_->region().z, view_->region().w);
    }
}

/**
 **/
boost::shared_ptr<ViewPort> CameraControlTool::view() const {
    return view_;
}

/**
 **/
CameraControlTool::CameraMode CameraControlTool::mode() const noexcept {
    return mode_;
}

/**
 **/
bool CameraControlTool::dragging() const noexcept {
    return dragging_;
}

/**
 **/
glm::vec2 CameraControlTool::local(const glm::vec2& position) const {
    if (!view_) {
        return position;
    }
    return glm::vec2(position.x - view_->region().x, position.y - view_->region().y);
}

/**
 **/
void CameraControlTool::motion(const glm::vec2& position) {
    const glm::vec2 point = local(position);

    if (dragging_) {
        if (mode_ == CAMERA_MODE_ZOOM) {
            zoom(point);
        } else if (mode_ == CAMERA_MODE_TRUCK) {
            truck(point);
        } else if (mode_ == CAMERA_MODE_PAN) {
            pan(point);
        }
    }
    // after the move, not before: zoom() and truck() measure the move from last_, and a
    // last_ written first would make every move zero
    last_ = point;
}

/**
 **/
void CameraControlTool::button(unsigned int button, bool pressed, const glm::vec2& position) {
    // only the primary button drives the camera
    if (button != 1) {
        return;
    }
    last_ = local(position);
    dragging_ = pressed;
    if (pressed) {
        // the arcball drags from the point it was clicked at
        arcball_.click(last_);
    }
}

/**
 **/
void CameraControlTool::zoom(const glm::vec2& position) {
    if (!view_) {
        return;
    }
    const boost::shared_ptr<v3d::type::camera::Camera> camera = view_->camera();
    if (!camera) {
        return;
    }
    // an orthographic camera zooms, and a perspective camera dollies
    float delta;
    // position is where the cursor is now, and last_ is where it was at the last event
    delta = static_cast<float>(position[0] - last_[0]);
    float factor;
    if (camera->orthographic()) {
        factor = camera->orthoFactorHorizontal();
        camera->zoom(-delta * factor);
    } else {
        factor = perspectiveFactor;
        camera->dolly(delta * factor);
    }
}

/**
 **/
void CameraControlTool::truck(const glm::vec2& position) {
    if (!view_) {
        return;
    }
    const boost::shared_ptr<v3d::type::camera::Camera> camera = view_->camera();
    if (!camera) {
        return;
    }
    float delta_x;
    float delta_y;
    delta_x = static_cast<float>(position[0] - last_[0]);
    delta_y = static_cast<float>(position[1] - last_[1]);
    float factor;
    if (delta_x != 0.0F) {
        if (camera->orthographic()) {
            factor = camera->orthoFactorHorizontal();
        } else {
            factor = perspectiveFactor;
        }
        camera->truck(delta_x * -factor);
    }
    if (delta_y != 0.0F) {
        // the vertical factor, because the pixel aspect ratio applies to the width alone
        if (camera->orthographic()) {
            factor = camera->orthoFactorVertical();
        } else {
            factor = perspectiveFactor;
        }
        camera->pedestal(delta_y * factor);
    }
}

/**
 **/
void CameraControlTool::pan(const glm::vec2& position) {
    if (!view_) {
        return;
    }
    const boost::shared_ptr<v3d::type::camera::Camera> camera = view_->camera();
    if (!camera) {
        return;
    }
    if (!camera->orthographic()) {
        glm::quat rot;
        // the point the drag has reached: motion() records last_ only after this returns
        rot = arcball_.drag(position);
        camera->rotate(rot);
    }
}

};  // namespace v3d::editor
