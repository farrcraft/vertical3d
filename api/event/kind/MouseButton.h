/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/event/Event.h>

#include <string>
#include <string_view>

#include <glm/glm.hpp>

namespace v3d::event::kind {

/**
 * A mouse button changed edge. Carries where the cursor was when it did, in window
 * coordinates with the origin at the top left, the same frame MouseMotion reports in.
 *
 * The button is named, not numbered, so a subscriber needs no SDL header to tell which
 * one it was - the same reason a key arrives as "escape" rather than as a keycode. The
 * name is the event's own, the way KeyDown's is, so binding one is binding "left".
 **/
class MouseButton : public Event {
 public:
    /**
     * @param button the button's name, one of the names a binding config uses
     **/
    // not noexcept: the base takes the event name as a std::string, which allocates
    MouseButton(const std::string& button, const glm::vec2& position, const boost::shared_ptr<Context>& context, bool pressed);

    /**
     * @return the button's name
     **/
    std::string_view button() const;

    /**
     * @return where the cursor was when the button changed edge
     **/
    glm::vec2 position() const noexcept;

    /**
     **/
    bool pressed() const noexcept;

 private:
    glm::vec2 position_;
    bool pressed_;
};
};  // namespace v3d::event::kind
