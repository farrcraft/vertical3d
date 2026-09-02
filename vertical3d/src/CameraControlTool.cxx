/**
 * Vertical3D
 * Copyright(c) 2021 Joshua Farr(josh@farrcraft.com)
**/

#include "CameraControlTool.h"

#include <string>

namespace v3d::editor {

    /**
     **/
    CameraControlTool::CameraControlTool() :
        last_(0.0f, 0.0f),
        mode_(CAMERA_MODE_NONE),
        dragging_(false) {
    }

    /**
     **/
    void CameraControlTool::activate(const std::string& name) {
        if (name == "zoomCamera") {
            mode_ = CAMERA_MODE_ZOOM;
        } else if (name == "truckCamera") {
            mode_ = CAMERA_MODE_TRUCK;
        } else if (name == "panCamera") {
            mode_ = CAMERA_MODE_PAN;
        }
    }

    /**
     **/
    void CameraControlTool::deactivate(const std::string& name) {
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
        // after the move, not before: pan() drags the arcball to the point the gesture has
        // reached, and a last_ written first would make every rotation one event stale
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
        boost::shared_ptr<v3d::type::Camera> camera = view_->camera();
        if (!camera) {
            return;
        }
        // do camera zooming/dollying - ortho cameras zoom, but perspective cameras dolly
        float delta;
        // event is current position and mouse is previous position
        delta = static_cast<float>(position[0] - last_[0]);
        float factor;
        if (camera->orthographic()) {
            // size is the viewport size (pixel dimensions)
            factor = camera->orthoFactorHorizontal();
            camera->zoom(-delta * factor);
        } else {
            factor = 0.125f;
            camera->dolly(delta * factor);
        }
    }

    /**
     **/
    void CameraControlTool::truck(const glm::vec2& position) {
        if (!view_) {
            return;
        }
        boost::shared_ptr<v3d::type::Camera> camera = view_->camera();
        if (!camera) {
            return;
        }
        float delta_x, delta_y;
        delta_x = static_cast<float>(position[0] - last_[0]);
        delta_y = static_cast<float>(position[1] - last_[1]);
        float factor;
        if (delta_x != 0.0f) {
            if (camera->orthographic()) {
                factor = camera->orthoFactorHorizontal();
            } else {
                factor = 0.125f;
            }
            camera->truck(delta_x * -factor);
        }
        if (delta_y != 0.0f) {
            // the vertical factor - the pixel aspect ratio belongs to width alone
            if (camera->orthographic()) {
                factor = camera->orthoFactorVertical();
            } else {
                factor = 0.125f;
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
        boost::shared_ptr<v3d::type::Camera> camera = view_->camera();
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
