/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "MouseMotion.h"

#include <api/event/Chord.h>
#include <api/event/Context.h>
#include <api/event/Event.h>

#include <cstdint>

namespace v3d::event::kind {
/**
 **/
MouseMotion::MouseMotion(const Moved& moved, const boost::shared_ptr<Context>& context) :
    Event("motion", context),
    position_(moved.position),
    motion_(moved.motion),
    modifiers_(moved.modifiers),
    window_(moved.window) {
}

/**
 **/
glm::vec2 MouseMotion::position() const noexcept {
    return position_;
}

/**
 **/
glm::vec2 MouseMotion::motion() const noexcept {
    return motion_;
}

/**
 **/
const Modifiers& MouseMotion::modifiers() const noexcept {
    return modifiers_;
}

/**
 **/
std::uint32_t MouseMotion::window() const noexcept {
    return window_;
}

};  // namespace v3d::event::kind
