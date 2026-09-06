/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "../Component.h"
#include "Button.h"

#include <boost/shared_ptr.hpp>
#include <entt/entt.hpp>
#include <glm/vec2.hpp>

namespace v3d::ui::component {

/**
 * A strip of buttons along one edge of the window.
 *
 * Which edge it is on decides its shape: a top toolbar is a row as wide as the canvas and
 * a left one is a column as tall as what is under the top strip. The buttons themselves
 * are the same either way.
 *
 * The strip answers the cursor out of the bounds a renderer left on its buttons, per
 * ADR-0019, so nothing is hit until something has been drawn.
 **/
class Toolbar : public Component {
 public:
    /**
     * Which edge of the window the strip runs along.
     **/
    enum class Edge {
        Top,
        Left
    };

    /**
     * @param dispatcher the dispatcher a pressed button sends its bound event to
     * @param edge which edge the strip runs along
     **/
    Toolbar(const boost::shared_ptr<entt::dispatcher>& dispatcher, Edge edge);

    /**
     * @return which edge the strip runs along
     **/
    Edge edge() const noexcept;

    /**
     * Add a button to the end of the strip.
     **/
    void add(const boost::shared_ptr<Button>& button);

    /**
     * @return how many buttons are in the strip
     **/
    std::size_t size() const noexcept;

    /**
     * @param index which button, which must be less than size()
     **/
    boost::shared_ptr<Button> button(std::size_t index) const;

    /**
     * The cursor moved. A button under it is left in its hover state and every other one
     * is put back to normal, which is what a renderer draws the highlight from.
     *
     * @param cursor where it is, in the pixels the strip was drawn in
     * @return whether it is over the strip
     **/
    bool motion(const glm::vec2& cursor);

    /**
     * The cursor is somewhere this strip cannot see - over a dropped menu panel, say -
     * so nothing on it is hovered.
     **/
    void leave();

    /**
     * The primary button went down. A press on a button sends that button's command.
     *
     * @param cursor where the cursor is
     * @return whether the strip took the press, which a press anywhere on it does
     **/
    bool press(const glm::vec2& cursor);

    /**
     * Find a button by the command it sends, so that whatever answers a command can mark
     * the button that names it.
     * @param command the button's event as Event::str() gives it - "context::name"
     * @return the button, or null when no button sends that command
     **/
    boost::shared_ptr<Button> find(const std::string& command) const;

 private:
    /**
     * @return the button the cursor is on, or null
     **/
    boost::shared_ptr<Button> buttonAt(const glm::vec2& cursor) const;

    boost::shared_ptr<entt::dispatcher> dispatcher_;
    std::vector<boost::shared_ptr<Button>> buttons_;
    Edge edge_;
};

};  // namespace v3d::ui::component
