/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "MouseWheel.h"

namespace v3d::event::kind {
/**
 **/
MouseWheel::MouseWheel(const Turned& turned, const boost::shared_ptr<Context>& context) :
    Event("wheel", context),
    notches_(turned.notches),
    position_(turned.position) {
}

/**
 **/
glm::vec2 MouseWheel::notches() const noexcept {
    return notches_;
}

/**
 **/
glm::vec2 MouseWheel::position() const noexcept {
    return position_;
}

};  // namespace v3d::event::kind
