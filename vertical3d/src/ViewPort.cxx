/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "ViewPort.h"

#include <string>

#include <boost/make_shared.hpp>

namespace v3d::editor {

    /**
     **/
    ViewPort::ViewPort(const std::string& name, const v3d::type::CameraProfile& profile) :
        name_(name),
        region_(0.0f, 0.0f, 0.0f, 0.0f),
        showFlags_(SHOW_GRID | SHOW_MESH) {
        camera_ = boost::make_shared<v3d::type::Camera>(profile);
    }

    /**
     **/
    boost::shared_ptr<v3d::type::Camera> ViewPort::camera() const {
        return camera_;
    }

    /**
     **/
    const std::string& ViewPort::name() const noexcept {
        return name_;
    }

    /**
     **/
    void ViewPort::resize(const glm::vec4& region) {
        region_ = region;

        const unsigned int width = static_cast<unsigned int>(region.z > 0.0f ? region.z : 0.0f);
        const unsigned int height = static_cast<unsigned int>(region.w > 0.0f ? region.w : 0.0f);
        camera_->profile().size(width, height);
        if (height > 0) {
            camera_->profile().pixelAspect(static_cast<float>(width) / static_cast<float>(height));
        }
    }

    /**
     **/
    const glm::vec4& ViewPort::region() const noexcept {
        return region_;
    }

    /**
     **/
    void ViewPort::show(unsigned int flags) noexcept {
        showFlags_ = flags;
    }

    /**
     **/
    unsigned int ViewPort::show() const noexcept {
        return showFlags_;
    }

    /**
     **/
    void ViewPort::show(VisibleFilter filter, bool visible) noexcept {
        if (visible) {
            showFlags_ |= static_cast<unsigned int>(filter);
        } else {
            showFlags_ &= ~static_cast<unsigned int>(filter);
        }
    }

    /**
     **/
    bool ViewPort::shows(VisibleFilter filter) const noexcept {
        return (showFlags_ & static_cast<unsigned int>(filter)) != 0;
    }

    /**
     **/
    ConstructionPlane& ViewPort::grid() noexcept {
        return grid_;
    }

    /**
     **/
    void ViewPort::draw(v3d::render::realtime::LineCanvas* canvas) {
        if (canvas == nullptr) {
            return;
        }
        canvas->clear();

        // the matrices are rebuilt once a frame rather than on every camera move, so a
        // frame sees one consistent view however many moves went into it
        camera_->createProjection();
        camera_->createView();

        if (shows(SHOW_GRID)) {
            grid_.draw(*camera_, canvas);
        }
    }

};  // namespace v3d::editor
