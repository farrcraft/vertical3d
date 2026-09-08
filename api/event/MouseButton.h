/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include "Event.h"

#include <string>

#include <glm/glm.hpp>

namespace v3d::event {

/**
 * A mouse button changed edge. Carries where the cursor was when it did, in window
 * coordinates with the origin at the top left, the same frame MouseMotion reports in.
 **/
class MouseButton : public Event {
 public:
    /**
     **/
    // not noexcept: the base takes the event name as a std::string, which allocates
    MouseButton(unsigned int button, const glm::vec2& position, const boost::shared_ptr<Context>& context, bool pressed);

    /**
     **/
    unsigned int button() const noexcept;

    /**
     * @return where the cursor was when the button changed edge
     **/
    glm::vec2 position() const noexcept;

    /**
     **/
    bool pressed() const noexcept;

 private:
    unsigned int button_;
    glm::vec2 position_;
    bool pressed_;
};
};  // namespace v3d::event
