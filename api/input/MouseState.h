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
 * MouseState keeps track of the cursor position and of which buttons are currently held,
 * based on all of the motion and button events we've seen. It is the mouse counterpart of
 * KeyState.
 *
 * A button is named rather than numbered, the way a key is. The names are the ones a
 * binding config uses - "left", "middle", "right", "x1", "x2" - so a consumer asking
 * whether the left button is down needs no SDL header to say which one that is.
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
     * @param p the new cursor position
     *
     * @return the previous position
     **/
    glm::vec2 operator() (const glm::vec2& p);

    /**
     * @return the current cursor position
     **/
    glm::vec2 position() const;

    /**
     * Turn the wheel.
     *
     * Accumulated rather than replaced, because a wheel sends one event per notch and
     * several can land in one frame - a flick that turned three notches has to read as
     * three, not as the last one.
     *
     * @param notches how far it turned, away from the reader first
     **/
    void wheel(float notches);

    /**
     * How far the wheel turned since the last flush, away from the reader first.
     *
     * An edge rather than a position: there is no such thing as where a wheel is, so this
     * is cleared with the button edges and a frame that reads it late reads zero. That is
     * the sign convention ui::Immediate::Input takes.
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
