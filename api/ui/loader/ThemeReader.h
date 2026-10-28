/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/log/Logger.h>

#include <string>
#include <vector>

#include <boost/json/object.hpp>
#include <boost/shared_ptr.hpp>

namespace v3d::ui::style {
class Property;
class Style;
class Theme;
};  // namespace v3d::ui::style

namespace v3d::ui::loader {

/**
 * Reads the themes of a ui document, and the name of the one that starts active.
 *
 * It reads by the rule Members.h states. A theme's name, a style's class and name, a
 * property's name and its value are required. A theme's styles, and a style's arrays of
 * properties, are optional.
 **/
class ThemeReader final {
 public:
    explicit ThemeReader(const boost::shared_ptr<v3d::log::Logger>& logger);

    /**
     * Read the "themes" array and the "theme" member of a document.
     * @return false when the document is refused
     **/
    bool read(const boost::json::object& doc);

    /**
     * @return the themes, in the order the document listed them
     **/
    std::vector<boost::shared_ptr<style::Theme>>& themes() noexcept;

    /**
     * @return the name of the theme the document makes active, or empty when it names none
     **/
    const std::string& active() const noexcept;

 private:
    /**
     * Read one theme and the styles in it. A theme holding no styles draws in the defaults.
     **/
    bool readTheme(const boost::json::object& entry);

    /**
     * Read one style and everything in it into a theme.
     **/
    bool readStyle(const boost::json::object& entry, const boost::shared_ptr<style::Theme>& theme);

    /**
     * Read the three kinds of property a style may hold - colours, numbers and images - each
     * from its own array. Any other member of the style is not read.
     **/
    bool readProperties(const boost::json::object& entry, const boost::shared_ptr<style::Style>& target);

    /**
     * Build one style property as the class of the array it was written in.
     *
     * @param section which of the three arrays the property came out of
     * @param propertyClass what the built property is filed under, which is the singular of
     *        the section
     * @return the property, or null when it does not carry what its class needs
     **/
    boost::shared_ptr<style::Property> readProperty(const std::string& section,
        const boost::json::object& property, const std::string& name, std::string* propertyClass);

    boost::shared_ptr<v3d::log::Logger> logger_;
    std::vector<boost::shared_ptr<style::Theme>> themes_;
    std::string active_;
};

};  // namespace v3d::ui::loader
