/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/ui/style/Style.h>

#include <cstdint>
#include <string>

namespace v3d::ui::style {

/**
 * A style for buttons.
 */
class Button : public Style {
 public:
    /**
     * Which look of a button the style applies to. A theme names it with "state".
     *
     * Separate from component::Button::ButtonState. Normal, Hover and Press match the
     * transient state the cursor writes. Disabled is for a disabled component, which the
     * cursor never changes.
     */
    enum class State : std::uint8_t {
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
