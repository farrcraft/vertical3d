/**
 * Vertical3D
 * Copyright(c) 2021 Joshua Farr(josh@farrcraft.com)
**/

#include "CameraControl.h"

#include <api/type/camera/Camera.h>
#include <api/type/camera/Profile.h>
#include <vertical3d/src/view/ViewPort.h>

#include <string>

namespace v3d::editor::tool {

/**
 **/
CameraControl::CameraControl() :
    last_(0.0F, 0.0F),
    mode_(CAMERA_MODE_NONE),
    latched_(CAMERA_MODE_NONE),
    dragging_(false) {
}

/**
 **/
CameraControl::CameraMode CameraControl::named(const std::string& name) {
    if (name == "zoom") {
        return CameraControl::CAMERA_MODE_ZOOM;
    }
    if (name == "truck") {
        return CameraControl::CAMERA_MODE_TRUCK;
    }
    if (name == "pan") {
        return CameraControl::CAMERA_MODE_PAN;
    }
    if (name == "dolly") {
        return CameraControl::CAMERA_MODE_DOLLY;
    }
    if (name == "pitch") {
        return CameraControl::CAMERA_MODE_PITCH;
    }
    return CameraControl::CAMERA_MODE_NONE;
}

/**
 **/
void CameraControl::activate(const std::string& name) {
    const CameraMode mode = named(name);
    if (mode != CAMERA_MODE_NONE) {
        mode_ = mode;
    }
}

/**
 **/
void CameraControl::deactivate(const std::string& name) {
    // releasing a move that is not the one going leaves the one going alone
    if (named(name) == mode_) {
        mode_ = CAMERA_MODE_NONE;
    }
}

/**
 **/
void CameraControl::latch(const std::string& name) {
    const CameraMode mode = named(name);
    latched_ = mode == latched_ ? CAMERA_MODE_NONE : mode;
}

/**
 **/
void CameraControl::unlatch() noexcept {
    latched_ = CAMERA_MODE_NONE;
}

/**
 **/
CameraControl::CameraMode CameraControl::latched() const noexcept {
    return latched_;
}

/**
 **/
void CameraControl::view(const boost::shared_ptr<view::ViewPort>& view) {
    view_ = view;
    if (view_) {
        arcball_.bounds(view_->region().z, view_->region().w);
    }
}

/**
 **/
boost::shared_ptr<view::ViewPort> CameraControl::view() const {
    return view_;
}

/**
 **/
CameraControl::CameraMode CameraControl::mode() const noexcept {
    return mode_ != CAMERA_MODE_NONE ? mode_ : latched_;
}

/**
 **/
bool CameraControl::dragging() const noexcept {
    return dragging_;
}

/**
 **/
bool CameraControl::takes(const glm::vec2& /* position */) const {
    return mode() != CAMERA_MODE_NONE;
}

/**
 **/
glm::vec2 CameraControl::local(const glm::vec2& position) const {
    if (!view_) {
        return position;
    }
    return glm::vec2(position.x - view_->region().x, position.y - view_->region().y);
}

/**
 **/
void CameraControl::motion(const glm::vec2& position) {
    const glm::vec2 point = local(position);

    if (dragging_) {
        switch (mode()) {
            case CAMERA_MODE_ZOOM:
                zoom(point);
                break;
            case CAMERA_MODE_DOLLY:
                dolly(point);
                break;
            case CAMERA_MODE_TRUCK:
                truck(point);
                break;
            case CAMERA_MODE_PAN:
                pan(point);
                break;
            case CAMERA_MODE_PITCH:
                pitch(point);
                break;
            case CAMERA_MODE_NONE:
                break;
        }
    }
    // after the move, not before: zoom() and truck() measure the move from last_, and a
    // last_ written first would make every move zero
    last_ = point;
}

/**
 **/
void CameraControl::button(unsigned int button, bool pressed, const glm::vec2& position) {
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
void CameraControl::zoom(const glm::vec2& position) {
    if (!view_) {
        return;
    }
    const boost::shared_ptr<v3d::type::camera::Camera> camera = view_->camera();
    if (!camera) {
        return;
    }
    // position is where the cursor is now, and last_ is where it was at the last event. A
    // drag to the right zooms in, which narrows a perspective camera's angle of view
    const float delta = position[0] - last_[0];
    if (camera->orthographic()) {
        camera->zoom(-delta * camera->orthoFactorHorizontal());
        return;
    }
    v3d::type::camera::Profile& profile = camera->profile();
    profile.fov(glm::clamp(profile.fov() - (delta * fovFactor), narrowest, widest));
}

/**
 **/
void CameraControl::dolly(const glm::vec2& position) {
    if (!view_) {
        return;
    }
    const boost::shared_ptr<v3d::type::camera::Camera> camera = view_->camera();
    if (!camera) {
        return;
    }
    // an orthographic view shows the same picture from anywhere along its direction, so a
    // dolly there changes what it shows as a zoom does
    const float delta = position[0] - last_[0];
    if (camera->orthographic()) {
        camera->zoom(-delta * camera->orthoFactorHorizontal());
    } else {
        camera->dolly(delta * perspectiveFactor);
    }
}

/**
 **/
void CameraControl::truck(const glm::vec2& position) {
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
void CameraControl::pan(const glm::vec2& position) {
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

/**
 **/
void CameraControl::pitch(const glm::vec2& position) {
    if (!view_) {
        return;
    }
    const boost::shared_ptr<v3d::type::camera::Camera> camera = view_->camera();
    if (!camera || camera->orthographic()) {
        return;
    }
    // a drag up looks up
    camera->tilt((position[1] - last_[1]) * pitchFactor);
}

};  // namespace v3d::editor::tool
