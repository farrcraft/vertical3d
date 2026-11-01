/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/ui/style/Style.h>

#include <string>
#include <vector>

#include <boost/shared_ptr.hpp>

namespace v3d::ui::style {
/**
 * A named collection of styles.
 */
class Theme {
 public:
    explicit Theme(const std::string & str);
    ~Theme();

    /**
        * Get the name of the theme.
        * @return theme name
        */
    std::string_view name() const;
    /**
        * Add a style to the theme
        * @param style the style to add
        */
    void addStyle(const boost::shared_ptr<Style>& style);
    /**
        * Get a set of styles with the matching name and class.
        * If no name or class is specified then no filtering of that parameter will be done. This allows getting the
        * subsets that consist of all styles, all of the same name, and all of the same class.
        * @param name the name of the styles to find.
        * @param class_name the class of the styles to get
        * @return a collection of styles
        */
    std::vector< boost::shared_ptr<Style> > getStyleSet(const std::string & name, const std::string & class_name) const;

    /**
     * Put another theme beneath this one. A style of this theme fills in what it does not name
     * from the other's style of the same class and name, and of the same state for a button.
     * The other's styles with no such match are added after this theme's own, so a component
     * that names no style is still dressed by this theme's first style of its class.
     *
     * Each style added is a copy of the other's, of the same kind and holding the same
     * properties, so filling in this theme's copy later leaves the other theme's style alone.
     *
     * @param lower the theme beneath this one
     **/
    void beneath(const Theme& lower);

 private:
    std::string name_;
    std::vector< boost::shared_ptr<Style> > styles_;
};

};  // end namespace v3d::ui::style
