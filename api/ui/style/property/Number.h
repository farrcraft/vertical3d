/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/ui/style/Property.h>

#include <string>

namespace v3d::ui::style::property {

/**
 * A style property that defines a single number - a height, a padding, a width. Metrics
 * come from the theme the same way colours do.
 */
class Number : public Property {
 public:
    /**
     * @param name the property name, which a style is asked for by - "bar-height"
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
