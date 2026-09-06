/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include "Event.h"

#include <glm/glm.hpp>

namespace v3d::event {

/**
 * The cursor moved. Carries where it moved to and how far it moved, both in window
 * coordinates with the origin at the top left.
 **/
class MouseMotion final : public Event {
 public:
    /**
     **/
    MouseMotion(const glm::vec2& position, const glm::vec2& motion, const boost::shared_ptr<Context>& context) noexcept;

    /**
     * @return the cursor position the move ended at
     **/
    glm::vec2 position() const noexcept;

    /**
     * @return how far the cursor moved
     **/
    glm::vec2 motion() const noexcept;

 private:
    glm::vec2 position_;
    glm::vec2 motion_;
};
};  // namespace v3d::event
