/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/ui/style/Property.h>

#include <string>

namespace v3d::ui::style::property {

/**
 * A style property that defines a single number, such as a height, a padding or a width.
 * A theme supplies these metrics the same way it supplies colours.
 */
class Number : public Property {
 public:
    /**
     * @param name the name a style looks the property up by, such as "bar-height"
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
