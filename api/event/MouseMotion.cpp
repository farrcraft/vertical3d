/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "MouseMotion.h"

namespace v3d::event {
    /**
     **/
    MouseMotion::MouseMotion(const glm::vec2& position, const glm::vec2& motion, const boost::shared_ptr<Context>& context) noexcept :
        Event("motion", context),
        position_(position),
        motion_(motion) {
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

};  // namespace v3d::event
