/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/event/Event.h>

#include <glm/glm.hpp>

namespace v3d::event::kind {

/**
 * The wheel turned. Carries how far, and where the cursor was as it did.
 *
 * A wheel has no position, only movement. notches() is how far the wheel turned since the
 * previous MouseWheel. A turn away from the user is positive, the sign ui::Immediate::Input
 * takes.
 *
 * A MouseWheel is not a Source, so no binding can map it to a command. MouseMotion is not a
 * Source either.
 **/
class MouseWheel final : public Event {
 public:
    /** How far the wheel turned on each axis, and where the cursor was. **/
    struct Turned {
        glm::vec2 notches{0.0F};
        glm::vec2 position{0.0F};
    };

    // not noexcept: the base takes the event name as a std::string, which allocates
    MouseWheel(const Turned& turned, const boost::shared_ptr<Context>& context);

    /**
     * @return how far it turned - y is the wheel itself, x a horizontal one where there
     *         is one, and away from the reader is positive in both
     **/
    glm::vec2 notches() const noexcept;

    /**
     * @return where the cursor was as it turned, which identifies what was scrolled
     **/
    glm::vec2 position() const noexcept;

 private:
    glm::vec2 notches_;
    glm::vec2 position_;
};
};  // namespace v3d::event::kind
