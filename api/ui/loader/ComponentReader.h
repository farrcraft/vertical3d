/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/event/Event.h>
#include <api/event/Router.h>
#include <api/log/Logger.h>
#include <api/ui/Layout.h>
#include <api/ui/Length.h>

#include <string>
#include <vector>

#include <boost/json/object.hpp>
#include <boost/shared_ptr.hpp>
#include <entt/entt.hpp>

namespace v3d::ui {
class Component;
class Container;
};  // namespace v3d::ui

namespace v3d::ui::component {
class Bar;
class Box;
class Button;
class CheckBox;
class Icon;
class Label;
class Panel;
class Scrollbar;
class Slider;
class RadioButton;
class SelectList;
class TabPage;
class TextBox;
class Toolbar;
};  // namespace v3d::ui::component

namespace v3d::ui::component::menu {
class Bar;
class Menu;
};  // namespace v3d::ui::component::menu

namespace v3d::ui::loader {

/**
 * Reads the containers of a ui document and the components in them.
 *
 * It reads by the rule Members.h states. A container's name, visible flag and components, a
 * component's type and name, a menu bar's menus, a menu's label and items, a menu item's
 * label and type, a toolbar's buttons and an icon's source are required. Everything else is
 * optional.
 **/
class ComponentReader final {
 public:
    /**
     * @param router where a command's context is looked up
     * @param dispatcher what a built menu or toolbar sends its events to
     * @param logger where a document that does not read is reported
     **/
    ComponentReader(const boost::shared_ptr<v3d::event::Router>& router,
        const boost::shared_ptr<entt::dispatcher>& dispatcher,
        const boost::shared_ptr<v3d::log::Logger>& logger);

    /**
     * Read the "containers" array of a document.
     * @return false when the document is refused
     **/
    bool read(const boost::json::object& doc);

    /**
     * @return the containers, in the order the document listed them
     **/
    std::vector<boost::shared_ptr<Container>>& containers() noexcept;

 private:
    /**
     * Read one container and the components in it.
     **/
    bool readContainer(const boost::json::object& entry);

    /**
     * Build one component from the type it names, read what it holds, and hand it back for
     * whatever is holding it.
     *
     * @return the component, or null when the document is refused
     **/
    boost::shared_ptr<Component> readComponent(const boost::json::object& entry);

    /**
     * Read a component's "children" array, if it has one, into the component.
     **/
    bool readChildren(const boost::json::object& entry, const boost::shared_ptr<Component>& component);

    /**
     * Build the component one entry names, before anything every component has - its name,
     * its box, what it holds - has been read onto it.
     *
     * @return the component, or null for a type the reader does not know
     **/
    boost::shared_ptr<Component> buildComponent(const std::string& componentType,
        const boost::json::object& entry);

    boost::shared_ptr<component::menu::Menu> readMenu(const boost::json::object& entry);
    boost::shared_ptr<component::menu::Bar> readMenuBar(const boost::json::object& entry);
    boost::shared_ptr<component::Toolbar> readToolbar(const boost::json::object& entry);
    boost::shared_ptr<component::Button> readButton(const boost::json::object& entry);
    boost::shared_ptr<component::Label> readLabel(const boost::json::object& entry);
    boost::shared_ptr<component::Icon> readIcon(const boost::json::object& entry);
    boost::shared_ptr<component::Bar> readBar(const boost::json::object& entry);
    boost::shared_ptr<component::Scrollbar> readScrollbar(const boost::json::object& entry);
    boost::shared_ptr<component::Slider> readSlider(const boost::json::object& entry);
    boost::shared_ptr<component::SelectList> readSelectList(const boost::json::object& entry);
    boost::shared_ptr<component::TextBox> readTextBox(const boost::json::object& entry);
    boost::shared_ptr<component::TabPage> readTabPage(const boost::json::object& entry);
    boost::shared_ptr<component::RadioButton> readRadioButton(const boost::json::object& entry);

    /**
     * Read a flow box, whose direction is the only thing the two kinds differ by.
     **/
    boost::shared_ptr<component::Box> readFlowBox(const std::string& componentType,
        const boost::json::object& entry);

    /**
     * Read a check box, or a radio button, which is a check box with a group. The caller
     * builds the right one, because the type tells them apart.
     **/
    void readCheckBox(const boost::json::object& entry, const boost::shared_ptr<component::CheckBox>& box);

    /**
     * Read what a flow box carries beyond an ordinary component: the gap between its
     * children, whether they are widened to it, and whether they wrap.
     **/
    void readBox(const boost::json::object& entry, const boost::shared_ptr<component::Box>& box);

    /**
     * Read what any component may carry, whatever its type: its style, whether it is
     * visible, enabled, pickable, focusable or clipped, and its depth. Its position and size
     * are read by readLayout(), which is not called for a menu or a menu bar.
     **/
    void readAttributes(const boost::json::object& entry, const boost::shared_ptr<Component>& component);

    /**
     * Read a component's box - "position" and "size" as two lengths each, and the corner of
     * the parent they are measured from.
     *
     * A number is pixels and a string ending in % is a fraction of the parent, so
     * "position": [10, "50%"] is ten pixels in and half way down. A component naming neither
     * is left Auto, which is its natural size where it has one.
     **/
    void readLayout(const boost::json::object& entry, Layout* layout);

    /**
     * Read a pair of lengths out of a two element array - a position or a size. A member
     * that is not two lengths is ignored, as is either length that is not one.
     **/
    void readLengths(const boost::json::object& entry, const std::string& key, Length* first, Length* second);

    /**
     * Read the "context" and "command" pair that a component carrying a command names, and
     * resolve the context.
     * @return the event, or one with no context when the entry omits either of the two
     **/
    v3d::event::Event readCommand(const boost::json::object& entry);

    boost::shared_ptr<v3d::log::Logger> logger_;
    boost::shared_ptr<v3d::event::Router> router_;
    boost::shared_ptr<entt::dispatcher> dispatcher_;
    std::vector<boost::shared_ptr<Container>> containers_;
};

};  // namespace v3d::ui::loader
