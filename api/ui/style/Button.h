/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/ui/style/Style.h>

#include <string>

namespace v3d::ui::style {

/**
 * A style for buttons.
 */
class Button : public Style {
 public:
    /**
     * Which look the style dresses, which a theme names with "state".
     *
     * Its own rather than component::Button::ButtonState: three of these are the transient
     * state the cursor writes, and the fourth is a component being disabled, which nothing
     * about the cursor touches and lasts until something says otherwise - ADR-0059.
     */
    enum class State {
        Normal,
        Hover,
        Press,
        Disabled
    };

    Button(const std::string& str, State s);
    ~Button();

    /**
     * Get the look this style is used for.
     * @return the look
     */
    State state() const;

 private:
    State state_;
};

};  // end namespace v3d::ui::style
