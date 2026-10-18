/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <string>
#include <string_view>
#include <vector>

#include <glm/glm.hpp>

namespace v3d::input {

/**
 * The cursor position and which buttons are held, built from every motion and button event
 * the mouse has handled. It is the mouse counterpart of KeyState.
 *
 * A button is named, not numbered, as a key is. The names are the ones a binding config uses:
 * "left", "middle", "right", "x1" and "x2". A caller that checks the left button therefore
 * needs no SDL header.
 **/
class MouseState final {
 public:
    MouseState();

    /**
     * Is a button currently held?
     *
     * @param button the button's name
     *
     * @return bool
     **/
    bool held(std::string_view button) const;

    /**
     * Did a button go down since the last flush?
     *
     * @param button the button's name
     **/
    bool pressed(std::string_view button) const;

    /**
     * Did a button come up since the last flush?
     *
     * @param button the button's name
     **/
    bool released(std::string_view button) const;

    /**
     * Forget this frame's edges, leaving what is held and where the cursor is alone. Called
     * once per frame by the loop, after everything that reads them has run.
     **/
    void flush();

    /**
     * Toggle the state of a button
     *
     * @param button the name of the button being toggled
     *
     * @return bool true if the resulting state is a held button
     **/
    bool operator() (const std::string& button);

    /**
     * Move the cursor
     *
     * @param cursor the new cursor position
     *
     * @return the previous position
     **/
    glm::vec2 operator() (const glm::vec2& cursor);

    /**
     * @return the current cursor position
     **/
    glm::vec2 position() const;

    /**
     * Turn the wheel.
     *
     * Added to the total, not replacing it. A wheel sends one event per notch, and several
     * can arrive in one frame. A flick that turned three notches reads as three.
     *
     * @param notches how far it turned, away from the reader first
     **/
    void wheel(float notches);

    /**
     * How far the wheel turned since the last flush, away from the reader first.
     *
     * An edge rather than a position, because a wheel has no position. It is cleared with
     * the button edges, so a frame that reads it after the flush reads zero.
     **/
    float wheel() const noexcept;

 private:
    std::vector<std::string> buttons_;
    /**< what went down since the last flush, and what came up **/
    std::vector<std::string> pressed_;
    std::vector<std::string> released_;
    glm::vec2 position_;
    /**< notches turned since the last flush, which is an edge and not a position **/
    float wheel_;
};

};  // namespace v3d::input
