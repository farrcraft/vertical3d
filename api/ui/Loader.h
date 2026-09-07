/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <string>
#include <vector>

#include "Layout.h"

#include "../event/Engine.h"
#include "../event/Event.h"
#include "../log/Logger.h"

#include <boost/json/object.hpp>
#include <boost/shared_ptr.hpp>
#include <entt/entt.hpp>

namespace v3d::ui {

class Component;
class Container;
class Style;

namespace style {
class Property;
class Theme;
};  // namespace style

namespace component {
class Bar;
class Box;
class Button;
class CheckBox;
class Icon;
class Label;
class Menu;
class MenuBar;
class Panel;
class Scrollbar;
class SelectList;
class Toolbar;
};  // namespace component

/**
 * Builds containers and themes out of a ui config document.
 *
 * Separate from Engine, which is what the built ui is: a loader is a thing that runs once
 * and a ui is a thing that is asked questions for as long as the app lives, and keeping
 * them together put twenty-five parsing methods and boost::json into a header every app
 * that draws a ui compiles.
 *
 * Nothing here is kept. `load()` fills the two collections and hands them over.
 **/
class Loader final {
 public:
    /**
     * @param eventEngine where a command's context is looked up
     * @param dispatcher what a built menu, toolbar or button sends its event to
     * @param logger where a document that does not parse says so
     **/
    Loader(const boost::shared_ptr<v3d::event::Engine>& eventEngine,
        const boost::shared_ptr<entt::dispatcher>& dispatcher,
        const boost::shared_ptr<v3d::log::Logger>& logger);

    /**
     * Read the themes and the containers a document names.
     * @return false when the document is not one, leaving what was read incomplete
     **/
    bool load(const boost::json::object& doc);

    /**
     * @return the containers, in the order the document listed them
     **/
    std::vector<boost::shared_ptr<Container>>& containers() noexcept;

    /**
     * @return the themes, in the order the document listed them
     **/
    std::vector<boost::shared_ptr<style::Theme>>& themes() noexcept;

    /**
     * @return the name of the theme the document said starts active, empty when it named
     *      none - in which case the first one loaded is
     **/
    const std::string& active() const noexcept;

 private:
    /**
     * Read the themes array, and the name of the one that starts active.
     **/
    bool loadThemes(const boost::json::object& doc);

    /**
     * Read one theme and the styles in it. A theme holding no styles is legal and draws in
     * the defaults.
     **/
    bool loadTheme(const boost::json::object& entry);

    /**
     * Read one container and the components in it.
     **/
    bool loadContainer(const boost::json::object& entry);

    /**
     * Build one component from the type it names, read what it holds, and hand it back for
     * whatever is holding it.
     *
     * @return the component, or null when the entry names a type there is no loader for
     **/
    boost::shared_ptr<Component> loadComponent(const boost::json::object& entry);

    /**
     * Read a component's "children" array, if it has one, into the component.
     **/
    bool loadChildren(const boost::json::object& entry, const boost::shared_ptr<Component>& component);

    /**
     * Build the component one config entry names, before anything every component has -
     * its name, its box, what it holds - has been read onto it.
     *
     * @return the component, or null for a type the loader does not know
     **/
    boost::shared_ptr<Component> buildComponent(const std::string& componentType,
        const boost::json::object& entry);

    boost::shared_ptr<component::Menu> loadMenu(const boost::json::object& entry);
    boost::shared_ptr<component::MenuBar> loadMenuBar(const boost::json::object& entry);
    boost::shared_ptr<component::Toolbar> loadToolbar(const boost::json::object& entry);
    boost::shared_ptr<component::Button> loadButton(const boost::json::object& entry);
    boost::shared_ptr<component::Label> loadLabel(const boost::json::object& entry);
    boost::shared_ptr<component::Icon> loadIcon(const boost::json::object& entry);
    boost::shared_ptr<component::Panel> loadPanel(const boost::json::object& entry);
    boost::shared_ptr<component::Bar> loadBar(const boost::json::object& entry);
    boost::shared_ptr<component::Scrollbar> loadScrollbar(const boost::json::object& entry);
    boost::shared_ptr<component::SelectList> loadSelectList(const boost::json::object& entry);

    /**
     * Read a check box, or the radio button that is one with a group. Which of the two is
     * built is the caller's, because the type is what tells them apart.
     **/
    void loadCheckBox(const boost::json::object& entry, const boost::shared_ptr<component::CheckBox>& box);

    /**
     * Read what a flow box carries beyond an ordinary component - the gap between its
     * children, and whether they are widened to it.
     **/
    void loadBox(const boost::json::object& entry, const boost::shared_ptr<component::Box>& box);

    /**
     * Read one style and everything in it into a theme.
     * @return false when the style names a class it cannot be built as
     **/
    bool loadStyle(const boost::json::object& entry, const boost::shared_ptr<style::Theme>& theme);

    /**
     * Read the four kinds of property a style may hold - colours, numbers, fonts and images
     * - each from its own array.
     **/
    bool loadProperties(const boost::json::object& entry, const boost::shared_ptr<Style>& target);

    /**
     * Build one style property as the class of the array it was written in.
     *
     * @param section which of the four arrays the property came out of
     * @param propertyClass what the built property is filed under, which is the singular of
     *        the section
     * @return the property, or null when it does not carry what its class needs
     **/
    boost::shared_ptr<style::Property> loadProperty(const std::string& section,
        const boost::json::object& property, const std::string& name, std::string* propertyClass);

    /**
     * Read what every component may carry whatever its type: where it is, how big it is,
     * which style draws it, whether it is drawn at all, whether it answers the cursor, and
     * what it is drawn in front of.
     **/
    void loadAttributes(const boost::json::object& entry, const boost::shared_ptr<Component>& component);

    /**
     * Read a component's box - "position" and "size" as two lengths each, and the corner of
     * the parent they are measured from.
     *
     * A number is pixels and a string ending in % is a fraction of the parent, so
     * "position": [10, "50%"] is ten pixels in and half way down. A component naming neither
     * is left Auto, which is the size it makes of itself where it has one.
     **/
    void loadLayout(const boost::json::object& entry, Layout* layout);

    /**
     * Read the "context" and "command" pair a menu item or a toolbar button names, and
     * resolve the context.
     * @return the event, or one with no context when the config gave neither
     **/
    v3d::event::Event loadCommand(const boost::json::object& entry);

    boost::shared_ptr<v3d::log::Logger> logger_;
    boost::shared_ptr<v3d::event::Engine> eventEngine_;
    boost::shared_ptr<entt::dispatcher> dispatcher_;
    std::vector<boost::shared_ptr<Container>> containers_;
    std::vector<boost::shared_ptr<style::Theme>> themes_;
    std::string active_;
};

};  // namespace v3d::ui
