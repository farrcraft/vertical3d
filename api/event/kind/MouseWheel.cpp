/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "MouseWheel.h"

#include <api/event/Context.h>
#include <api/event/Event.h>

#include <cstdint>

namespace v3d::event::kind {
/**
 **/
MouseWheel::MouseWheel(const Turned& turned, const boost::shared_ptr<Context>& context) :
    Event("wheel", context),
    notches_(turned.notches),
    position_(turned.position),
    window_(turned.window) {
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

/**
 **/
std::uint32_t MouseWheel::window() const noexcept {
    return window_;
}

};  // namespace v3d::event::kind
