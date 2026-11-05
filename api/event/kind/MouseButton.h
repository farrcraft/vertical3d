/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/event/Event.h>

#include <cstdint>
#include <string>
#include <string_view>

#include <glm/glm.hpp>

namespace v3d::event::kind {

/**
 * A mouse button changed edge. Carries where the cursor was when it did, in window
 * coordinates with the origin at the top left, the same frame MouseMotion reports in.
 *
 * The button is named, not numbered, so a listener needs no SDL header to tell which one it
 * was. A key arrives as "escape", not as a keycode, for the same reason. The button's name is
 * the event's name, as a key's is, so a binding names the left button "left".
 **/
class MouseButton : public Event {
 public:
    /**
     * Where the cursor was, which way the button went, how many presses in quick succession the
     * edge belongs to, and the id of the window the cursor was in, as Window::id() gives it. The
     * platform counts clicks: 1 for a single click, 2 for the second of a double click.
     **/
    struct Edge {
        glm::vec2 position{0.0F};
        bool pressed = false;
        unsigned int clicks = 1;
        std::uint32_t window = 0;
    };

    /**
     * @param button the button's name, one of the names a binding config uses
     * @param edge where the cursor was, which way the button went, the clicks and the window
     * @param context the context the event belongs to, the mouse's
     **/
    // not noexcept: the base takes the event name as a std::string, which allocates
    MouseButton(const std::string& button, const Edge& edge, const boost::shared_ptr<Context>& context);

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

    /**
     * @return how many presses in quick succession this edge belongs to
     **/
    unsigned int clicks() const noexcept;

    /**
     * @return the id of the window the cursor was in, whose coordinates the position is in
     **/
    std::uint32_t window() const noexcept;

 private:
    glm::vec2 position_;
    bool pressed_;
    unsigned int clicks_;
    std::uint32_t window_;
};
};  // namespace v3d::event::kind
