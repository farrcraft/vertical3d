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
 * A style is keyed by the pair rather than by either half, so a theme can carry a default
 * for every button and an override for one of them. The properties themselves are
 * typed - a colour, a number, a font, an image - and are read back through the free
 * functions below rather than by casting at the call site.
 */
/**
 * What a style applies to: a component's own style name and its class, such as "button".
 */
struct Selector {
    std::string name;
    std::string className;
};

class Style {
 public:
    explicit Style(const Selector& selector);
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
    void addProperty(const boost::shared_ptr<Property>& prop, const std::string& class_name);
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
 * name it. Both the retained components and the immediate layer read a theme this way.
 **/
void readColour(const boost::shared_ptr<Style>& target, const std::string& name, glm::vec4* into);

/**
 * Read a number property out of a style, leaving what is there when the style does not
 * name it.
 **/
void readMetric(const boost::shared_ptr<Style>& target, const std::string& name, float* into);

/**
 * The values a theme wrote over a dressing, and what each held before.
 *
 * A theme names only some values. The next theme starts from what was there before any theme
 * rather than from what the last one left, so a value the new theme is silent on goes back to
 * its default, and no theme at all puts every value back.
 *
 * It records the field rather than its address, so a dressing that is copied keeps working.
 * A value written over a themed one by the app is put back with it.
 *
 * @param D the dressing the values are in
 **/
template <typename D>
class Overlay final {
 public:
    /**
     * Read a colour as readColour() does, and remember what it replaced.
     **/
    void colour(const boost::shared_ptr<Style>& target, const std::string& name, D* dressing, glm::vec4 D::* field) {
        const glm::vec4 was = dressing->*field;
        readColour(target, name, &(dressing->*field));
        if (dressing->*field != was) {
            colours_.emplace_back(field, was);
        }
    }

    /**
     * Read a number as readMetric() does, and remember what it replaced.
     **/
    void metric(const boost::shared_ptr<Style>& target, const std::string& name, D* dressing, float D::* field) {
        const float was = dressing->*field;
        readMetric(target, name, &(dressing->*field));
        if (dressing->*field != was) {
            metrics_.emplace_back(field, was);
        }
    }

    /**
     * Put back every value read since the last restore, and forget them.
     **/
    void restore(D* dressing) {
        for (auto it = colours_.rbegin(); it != colours_.rend(); ++it) {
            dressing->*(it->first) = it->second;
        }
        for (auto it = metrics_.rbegin(); it != metrics_.rend(); ++it) {
            dressing->*(it->first) = it->second;
        }
        colours_.clear();
        metrics_.clear();
    }

 private:
    std::vector<std::pair<glm::vec4 D::*, glm::vec4>> colours_;
    std::vector<std::pair<float D::*, float>> metrics_;
};

};  // namespace v3d::ui::style
