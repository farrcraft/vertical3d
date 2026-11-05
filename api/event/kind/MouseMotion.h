/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/event/Chord.h>
#include <api/event/Event.h>

#include <cstdint>

#include <glm/glm.hpp>

namespace v3d::event::kind {

/**
 * The cursor moved. Carries where it moved to and how far it moved, both in window
 * coordinates with the origin at the top left.
 **/
class MouseMotion final : public Event {
 public:
    /**
     * Where the cursor is, how far it moved, the modifier keys held as it did, and the id of the
     * window it moved in, as Window::id() gives it.
     **/
    struct Moved {
        glm::vec2 position{0.0F};
        glm::vec2 motion{0.0F};
        Modifiers modifiers;
        std::uint32_t window = 0;
    };

    // not noexcept: the base takes the event name as a std::string, which allocates
    MouseMotion(const Moved& moved, const boost::shared_ptr<Context>& context);

    /**
     * @return the cursor position the move ended at
     **/
    glm::vec2 position() const noexcept;

    /**
     * @return how far the cursor moved
     **/
    glm::vec2 motion() const noexcept;

    /**
     * @return the modifier keys held as the cursor moved
     **/
    const Modifiers& modifiers() const noexcept;

    /**
     * @return the id of the window the cursor moved in, whose coordinates the position is in
     **/
    std::uint32_t window() const noexcept;

 private:
    glm::vec2 position_;
    glm::vec2 motion_;
    Modifiers modifiers_;
    std::uint32_t window_;
};
};  // namespace v3d::event::kind
