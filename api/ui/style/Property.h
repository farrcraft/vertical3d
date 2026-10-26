/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <string>
#include <string_view>

namespace v3d::ui::style {
/**
 * A style property.
 * This is a base property class that specific types can derive from.
 */
class Property {
 public:
    explicit Property(const std::string& str);
    virtual ~Property();

    /**
     * Get the property name.
     * @return the property name
     */
    std::string_view name() const;

 private:
    std::string name_;
};

};  // end namespace v3d::ui::style
