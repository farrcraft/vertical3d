/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <vector>

#include "../../Component.h"
#include "MenuItem.h"

#include <boost/shared_ptr.hpp>
#include <entt/entt.hpp>

namespace v3d::ui::component {
/**
 * A Menu is a container for an ordered set of MenuItems. It does not dictate how the
 * items are rendered. 
 */
class Menu : public Component {
 public:
     /**
      * @param dispatcher the dispatcher activated menu items send their bound event to
      **/
     explicit Menu(const boost::shared_ptr<entt::dispatcher>& dispatcher);

    /**
        * Make the next item in the menu active.
        * If the last item in the menu is already active, it will wrap around
        * to the first item.
        * @return false on wrap around
        */
    bool next();
    /**
        * Make the previous item in the menu active.
        * If the first item in the menu is already active, it will wrap around
        * to the last item.
        * @return false on wrap around
        */
    bool previous();
    /**
        * Make the active item's parent menu active.
        * @return whether the menu level was successfully changed
        */
    bool up();
    /**
        * Make the active item's submenu active.
        * @return whether the menu level was successfully changed
        */
    bool down();
    /**
        * Get the number of items in this menu.
        * @return the item count
        */
    size_t size() const;

    // use a [] operator to get individual items instead
    boost::shared_ptr<MenuItem> & operator[](size_t i);

    /**
      * Get the active MenuItem from this menu
      * This is always the active item within this menu level
      * @return the active MenuItem
      */
    boost::shared_ptr<MenuItem> active() const;
    /**
      * Set the active MenuItem from this menu.
      * Negative or out of bounds (>= size) will result in a null active pointer
      * being set.
      * @param idx the index of new the active item.
      */
    void active(int idx);
    /**
      * Activate the active item of the current menu level.
      *
      * A submenu item descends a level and an action item dispatches its bound event. An
      * input item begins capturing instead: navigation stops moving, what is fed to
      * capture() becomes the item's value, and the item's event is sent carrying it.
      *
      * An activation arriving while a capture is open is what ends the capture, so this
      * is both the verb that starts one and the verb that finishes one.
      */
    void activate();

    /**
     * Give a capture in progress its value.
     *
     * A key input is finished by the first value it is given - a binding is one key, so
     * there is nothing to wait for and the event goes out here. The other input types
     * hold what they were last given and wait for the activation that ends them, because
     * a string or a number is built up rather than pressed.
     *
     * What a value means is the app's: this takes whatever it is fed and puts it on the
     * item, which is what the item's event carries as its data. For a key input that is a
     * key name, per api/input/Keyboard.cpp's table, because a key name is what a binding
     * document holds.
     *
     * @return whether a capture took it, which is false when none is open
     **/
    bool capture(const v3d::event::EventData& value);

    /**
     * @return whether an input item is capturing rather than the menu navigating
     **/
    bool capturing() const;

    /**
     * Abandon a capture without sending anything, leaving the item's value as it was.
     **/
    void cancel();

    void addItem(boost::shared_ptr<MenuItem> item);

    /**
     * Get the currently active menu level.
     * There may be a hierarchy of active menus and menu items that leads
     * all the way to the lowest active submenu.  Level points to that menu.
     **/
    boost::shared_ptr<Menu> level() const;
    void level(boost::weak_ptr<Menu> m);

    /**
     * Set this menu's parent if it is a submenu 
     **/
    void parent(boost::weak_ptr<Menu> p);

    /**
     * Send a menu item's bound event, carrying its value as event data when it has one.
     * Public because a menu bar activates an item the cursor is on rather than the one
     * navigation left active.
     * @return false when the item has no event bound to it
     **/
    bool dispatch(const boost::shared_ptr<MenuItem>& item) const;

    /**
     * Check to see if this is a submenu.
     * @return true if this is a submenu.
     **/
    bool hasParent() const;

 private:
    boost::shared_ptr<entt::dispatcher> dispatcher_;
    std::vector< boost::shared_ptr<MenuItem> > items_;
    int active_;  // the active item in this menu, or -1 when there is none
    boost::weak_ptr<Menu> level_;
    boost::weak_ptr<Menu> parent_;  // if this is a submenu it will have a parent menu
    // the item being captured into, held here rather than on the level it belongs to
    // because level_ is, and both are state of the menu as a whole rather than of one of
    // its levels
    boost::shared_ptr<MenuItem> capture_;
};

};  // namespace v3d::ui::component
