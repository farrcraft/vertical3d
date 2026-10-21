/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "ViewPort.h"

#include <api/render/realtime/canvas/Line.h>
#include <api/type/Checked.h>
#include <api/type/camera/Camera.h>
#include <api/type/camera/Profile.h>
#include <vertical3d/src/manipulator/Manipulator.h>
#include <vertical3d/src/manipulator/Target.h>
#include <vertical3d/src/scene/Scene.h>
#include <vertical3d/src/scene/WireframeVisitor.h>

#include "ConstructionPlane.h"

#include <cstdint>
#include <limits>
#include <string>

#include <boost/make_shared.hpp>

namespace v3d::editor {

/**
 **/
ViewPort::ViewPort(const std::string& name, const v3d::type::camera::Profile& profile) :
    name_(name),
    region_(0.0F, 0.0F, 0.0F, 0.0F),
    showFlags_(SHOW_GRID | SHOW_MESH | SHOW_HANDLE) {
    camera_ = boost::make_shared<v3d::type::camera::Camera>(profile);
}

/**
 **/
boost::shared_ptr<v3d::type::camera::Camera> ViewPort::camera() const {
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

    // a region that is negative, NaN or too large to count gives a size of zero
    const uint32_t width = v3d::type::toCount(region.z, std::numeric_limits<uint32_t>::max()).value_or(0U);
    const uint32_t height = v3d::type::toCount(region.w, std::numeric_limits<uint32_t>::max()).value_or(0U);
    camera_->profile().size(glm::uvec2(width, height));
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
void ViewPort::draw(const Scene& scene, const Manipulator* manipulator, const Target& target,
    v3d::render::realtime::canvas::Line* canvas, v3d::render::realtime::canvas::Line* handles) {
    if (canvas == nullptr) {
        return;
    }
    canvas->clear();
    if (handles != nullptr) {
        handles->clear();
    }

    // the matrices are rebuilt once a frame rather than on every camera move, so a
    // frame sees one consistent view however many moves went into it
    camera_->createProjection();
    camera_->createView();

    if (shows(SHOW_GRID)) {
        grid_.draw(*camera_, canvas);
    }

    if (shows(SHOW_MESH)) {
        WireframeVisitor wireframe(canvas);
        scene.accept(&wireframe);
    }

    if (shows(SHOW_HANDLE) && manipulator != nullptr && handles != nullptr) {
        manipulator->draw(target, *this, handles);
    }
}

};  // namespace v3d::editor
