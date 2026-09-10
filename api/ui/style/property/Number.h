/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/ui/style/Property.h>

#include <string>

namespace v3d::ui::style::property {

/**
 * A style property that defines a single number - a height, a padding, a width.
 *
 * Everything a component is drawn with comes out of the theme, and a metric is as much
 * part of a theme as a colour is: a strip that is too short for its font is the same
 * kind of wrong as one drawn in the wrong colour.
 */
class Number : public Property {
 public:
    /**
     * @param name the property name, which is what a style is asked for - "bar-height"
     * @param value the number
     */
    Number(const std::string& name, float value);
    ~Number();

    /**
     * @return the number
     */
    float value() const noexcept;
    /**
     * @param v the new number
     */
    void value(float v) noexcept;

 private:
    float value_;
};

};  // namespace v3d::ui::style::property
