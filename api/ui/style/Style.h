/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <string>
#include <map>
#include <utility>
#include <vector>

#include <boost/shared_ptr.hpp>
#include <glm/vec4.hpp>

namespace v3d::ui::style {

class Property;

/**
 * One entry of a theme: the properties a component of a given class and name is drawn with.
 *
 * A style is keyed by the pair rather than by either half, which is what lets a theme carry
 * a default for every button and an override for one of them. The properties themselves are
 * typed - a colour, a number, a font, an image - and are read back through the free
 * functions below rather than by casting at the call site.
 */
class Style {
 public:
    Style(const std::string& str, const std::string& class_name);
    virtual ~Style();

    /**
     * Get the name of the style.
     * @return the style name
     */
    std::string_view name() const;
    /**
     * Get the class name of the style.
     * @return the style's class name
     */
    std::string_view className() const;
    /**
     * Get a style property.
     * @param name the property name
     * @param class_name the property class
     * @return a pointer to the named style property
     */
    boost::shared_ptr<Property> property(const std::string& name, const std::string& class_name) const;
    /**
     * Add a style property to the style
     * @param prop the new property to be added
     * @param class_name the class of the new property
     */
    void addProperty(boost::shared_ptr<Property> prop, const std::string& class_name);
    /**
     * Get a collection of style properties
     * If no name or class is specified then no filtering of that parameter will be done. This allows getting the
     * subsets that consist of all styles, all of the same name, and all of the same class.
     * @param name the name of the properties to get
     * @param class_name the class of the properties to get
     * @return a collection of matching properties
     */
    std::vector< boost::shared_ptr<Property>  > getPropertySet(const std::string& name, const std::string& class_name) const;

 protected:
    void className(const std::string& str);

 private:
    std::string name_;
    std::string className_;
    std::map <std::pair<std::string, std::string>, boost::shared_ptr<Property> > properties_;  // key is pair<name, class>
};

/**
 * Read a colour property out of a style, leaving what is there when the style does not
 * name it. Both ways of writing a ui dress themselves from a theme this way, per ADR-0020.
 **/
void readColour(const boost::shared_ptr<Style>& target, const std::string& name, glm::vec4* into);

/**
 * Read a number property out of a style, leaving what is there when the style does not
 * name it.
 **/
void readMetric(const boost::shared_ptr<Style>& target, const std::string& name, float* into);

};  // namespace v3d::ui::style
