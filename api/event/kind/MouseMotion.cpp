/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "MouseMotion.h"

namespace v3d::event::kind {
/**
 **/
MouseMotion::MouseMotion(const Moved& moved, const boost::shared_ptr<Context>& context) :
    Event("motion", context),
    position_(moved.position),
    motion_(moved.motion) {
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

};  // namespace v3d::event::kind
