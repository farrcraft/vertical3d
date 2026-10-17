/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/event/Event.h>

#include <glm/glm.hpp>

namespace v3d::event::kind {

/**
 * The cursor moved. Carries where it moved to and how far it moved, both in window
 * coordinates with the origin at the top left.
 **/
class MouseMotion final : public Event {
 public:
    /**
     **/
    // not noexcept: the base takes the event name as a std::string, which allocates
    /** Where the cursor is, and how far it moved. **/
    struct Moved {
        glm::vec2 position{0.0f};
        glm::vec2 motion{0.0f};
    };

    MouseMotion(const Moved& moved, const boost::shared_ptr<Context>& context);

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
};  // namespace v3d::event::kind
