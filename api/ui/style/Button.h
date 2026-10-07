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
     * Separate from component::Button::ButtonState. Three of these looks are the transient
     * state the cursor writes. The fourth is a disabled component, which the cursor never
     * changes.
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
