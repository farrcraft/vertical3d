/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/ui/style/Property.h>

#include <string>

#include <glm/vec4.hpp>

namespace v3d::ui::style::property {

/**
 * A vGUI style property that defines a single color.
 *
 * The colour carries its own alpha: the ui is drawn over whatever the app has already
 * drawn, so a panel that lets the scene through is a colour rather than a mode.
 */
class Color : public Property {
 public:
    /**
     * @param name the property name, which is what a style is asked for - "panel", "text"
     * @param value the colour, with alpha
     */
    Color(const std::string& name, const glm::vec4& value);
    ~Color();

    /**
     * @return the colour
     */
    glm::vec4 value() const noexcept;
    /**
     * @param v the new colour
     */
    void value(const glm::vec4& v) noexcept;

 private:
    glm::vec4 value_;
};

};  // namespace v3d::ui::style::property
