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
 * A turn is an edge and not a position - there is no such thing as where a wheel is - so
 * this says how many notches went by since the last one of these, away from the reader
 * first. That is the sign ui::Immediate::Input takes.
 *
 * Not a bindable source event, for the reason a motion is not: it has no discrete name for
 * a binding to hang a command on.
 **/
class MouseWheel final : public Event {
 public:
    /**
     **/
    // not noexcept: the base takes the event name as a std::string, which allocates
    MouseWheel(const glm::vec2& notches, const glm::vec2& position, const boost::shared_ptr<Context>& context);

    /**
     * @return how far it turned - y is the wheel itself, x a horizontal one where there
     *         is one, and away from the reader is positive in both
     **/
    glm::vec2 notches() const noexcept;

    /**
     * @return where the cursor was as it turned, which is what says what was scrolled
     **/
    glm::vec2 position() const noexcept;

 private:
    glm::vec2 notches_;
    glm::vec2 position_;
};
};  // namespace v3d::event::kind
