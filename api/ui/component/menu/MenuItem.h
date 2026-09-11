/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/event/Event.h>
#include <api/ui/Component.h>

#include <string>

#include "Type.h"

#include <boost/optional.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>

namespace v3d::ui::component {
class Menu;

/**
 * A single menu item. It consists of a label and points to an optional submenu.
 * There are several different types of menu items:
 * - action - activating an action item dispatches its bound event
 * - check - an action item that also shows a mark when it is checked
 * - radio - a check item that is one of a set, only one of which is marked
 * - submenu - activating this type of menu item will replace the currently active menu with the submenu
 * - input - this type of menu item is the same as an action item type except that it accepts a single input 
 *		when activated before it dispatches the event. the input becomes the item's value, which is sent as
 *		the event's data.
 * - numeric_input - the same as an input type except restricted to number input types only
 * - key_input - an input type that uses a key name as the value
 *
 * A check or radio item does not own the state it shows. Activating one dispatches its
 * event like an action item and marks nothing; whatever answers the command sets
 * checked(), so the mark cannot disagree with what the item reports.
 *
 * An input type item's value is appended to its label by text(), so a label ending in a separator
 * ("Rounds: ") reads as 'Rounds: 5'.
 *
 * Activating an input type item puts its menu into capture rather than dispatching: what
 * Menu::capture() is then given becomes the item's value, and the item's event carries it.
 * A key input ends at the first key it is given, because a binding is one key; the other two
 * end at the next activation, which the ui::selectMenu binding is what sends.
 **/
class MenuItem : public Component {
 public:
    /**
     * Construct a new menu item
     * @param type the type of menu item
     * @param label the label of the menu item
     * @param cmd the name of the command
     * @param scope the scope of the command
     * @param param the command paramater
     */
    MenuItem(menu::ItemType type, const std::string & label);

    /**
      * Set the menu item text label 
      * @param str the new label text
      */
    void label(const std::string & str);
    /**
      * Set the submenu
      * @param sub the new sub menu
      */
    void submenu(const boost::shared_ptr<Menu>& sub);
    /**
      * Set the parent owner menu
      * @param m the new parent
      */
    void menu(const boost::weak_ptr<Menu>& m);
    /**
      * Get the current menu item label text
      * @return the label text
      */
    std::string_view label() const;
    /**
      * Get the text to display for this item - the label, with the item's value appended
      * when it has one.
      * @return the display text
      */
    std::string text() const;
    /**
      * Set the item's value. Input type items carry the value they last captured, which is
      * sent as the data of the item's event when it is activated.
      * @param v the new value
      */
    void value(const v3d::event::EventData& v);
    /**
      * Get the item's value
      * @return the value, or none when the item has never been given one
      */
    boost::optional<v3d::event::EventData> value() const;
    /**
      * Get the submenu
      * @return the child menu
      */
    boost::shared_ptr<Menu> submenu() const;
    /**
     * Set the event that will be triggered by activating this menu item.
     * @param destination the event destination
     **/
    void event(const v3d::event::Event &destination);
    /**
      * Get the event bound to the menu item
      * @return the event
      */
    v3d::event::Event event() const;
    /**
      * Get the owning menu this item belongs to.
      * @return a pointer to the parent menu
      **/
    boost::weak_ptr<Menu> menu();

    menu::ItemType itemType() const;

    /**
      * Set whether a check or radio item draws its mark.
      * @param on whether the item is checked
      */
    void checked(bool on);
    /**
      * Get whether the item is checked. An item of any other type is never checked.
      * @return whether the item draws its mark
      */
    bool checked() const;

 private:
    std::string label_;
    boost::shared_ptr<Menu> submenu_;
    boost::weak_ptr<Menu> menu_;  // owning menu
    menu::ItemType type_;
    v3d::event::Event event_;
    bool checked_;
    bool hasValue_;
    v3d::event::EventData value_;
};

};  // namespace v3d::ui::component
