/**
 * Vertical3D
 * Copyright(c) 2021 Joshua Farr(josh@farrcraft.com)
**/

#include "CameraControlTool.h"

#include <string>

CameraControlTool::CameraControlTool(const boost::shared_ptr<v3d::input::MouseDevice> & mouse, const boost::shared_ptr<v3d::ViewPort> & view) :
    mode_(CAMERA_MODE_NONE),
    view_(view),
    mouse_(mouse) {
}

// tool overrides
void CameraControlTool::activate(const std::string & name) {
    if (name == "zoom_camera_tool") {
        mode_ = CAMERA_MODE_ZOOM;
    } else if (name == "truck_camera_tool") {
        mode_ = CAMERA_MODE_TRUCK;
    } else if (name == "pan_camera_tool") {
        mode_ = CAMERA_MODE_PAN;
    }
    mouse_->addEventListener(this, "camera_control_tool");
}

void CameraControlTool::deactivate(const std::string & name) {
    mode_ = CAMERA_MODE_NONE;
    mouse_->removeEventListener("camera_control_tool");
}

// mouse event listener overrides
void CameraControlTool::motion(unsigned int x, unsigned int y) {
    glm::vec2 position(x, y);

    if (mode_ == CAMERA_MODE_ZOOM) {
        zoom(position);
    } else if (mode_ == CAMERA_MODE_TRUCK) {
        truck(position);
    } else if (mode_ == CAMERA_MODE_PAN) {
        pan(position);
    }
    last_ = position;
}

void CameraControlTool::buttonPressed(unsigned int button) {
}

void CameraControlTool::buttonReleased(unsigned int button) {
}

void CameraControlTool::zoom(const glm::vec2 & position) {
    if (!view_)
        return;
    boost::shared_ptr<v3d::type::Camera> camera = view_->camera();
    if (!camera)
        return;
    // do camera zooming/dollying - ortho cameras zoom, but perspective cameras dolly
    float delta;
    // event is current position and mouse is previous position
    delta = static_cast<float>(position[0] - last_[0]);
    float factor;
    if (camera->orthographic()) {
        // size is the viewport size (pixel dimensions)
        factor = camera->orthoFactor();
        camera->zoom(-delta * factor);
    } else {
        factor = 0.125;
        camera->dolly(delta * factor);
    }
    view_->invalidate();
}

void CameraControlTool::truck(const glm::vec2 & position) {
    if (!view_)
        return;
    boost::shared_ptr<v3d::type::Camera> camera = view_->camera();
    if (!camera)
        return;
    float delta_x, delta_y;
    delta_x = static_cast<float>(position[0] - last_[0]);
    delta_y = static_cast<float>(position[1] - last_[1]);
    float factor;
    if (delta_x != 0.0) {
        if (camera->orthographic())
            factor = camera->orthoFactor();
        else
            factor = 0.125;
        camera->truck(delta_x * -factor);
    }
    if (delta_y != 0.0)  {
        if (camera->orthographic())
            factor = camera->orthoFactor();
        else
            factor = 0.125;
        camera->pedestal(delta_y * factor);
    }
    view_->invalidate();
}

void CameraControlTool::pan(const glm::vec2 & position) {
    if (!view_)
        return;
    boost::shared_ptr<v3d::type::Camera> camera = view_->camera();
    if (!camera)
        return;
    if (!camera->orthographic()) {
        glm::quat rot;
        rot = arcball_.drag(glm::vec2(static_cast<float>(last_[0]), static_cast<float>(last_[1])));
        camera->rotate(rot);
    }
    view_->invalidate();
}
