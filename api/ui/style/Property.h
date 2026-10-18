/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <cstdint>
#include <string>

namespace v3d::ui::style {
/**
 * A style property.
 * This is a base property class that specific types can derive from.
 */
class Property {
 public:
    explicit Property(const std::string& str);
    virtual ~Property();

    typedef enum : std::uint8_t {
        TOP,
        BOTTOM,
        LEFT,
        RIGHT,
        TOP_LEFT,
        BOTTOM_LEFT,
        TOP_RIGHT,
        BOTTOM_RIGHT,
        CENTER,
        NULL_ALIGNMENT
    } Alignment;

    /**
     * Get alignment property value.
     * @return the alignment setting
     */
    Alignment align() const;
    /**
     * Set the property alignment.
     * @param alignment the new alignment
     */
    void align(Alignment alignment);
    /**
     * Get the property name.
     * @return the property name
     */
    std::string_view name() const;

 private:
    Alignment align_;
    std::string name_;
};

};  // end namespace v3d::ui::style
