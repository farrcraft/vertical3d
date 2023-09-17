/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include "Menu.h"

#include <vector>

#include "../../Component.h"
#include "../../Navigation.h"
#include "MenuItem.h"

namespace v3d::ui::component {
    /**
     * A Menu is a container for an ordered set of MenuItems. It does not dictate how the
     * items are rendered. 
     */
    class Menu : public Component {
     public:
         Menu();

         /**
          * Navigate changes the currently active menu item.
          * @param Navigation direction of navigation
          * @param wrap whether navigation can "wrap around"
          * @return false when no navigation was possible
          **/
         bool navigate(Navigation direction, bool wrap);

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
          * Activate the currently active menu item.
          *
          */
        void activate();

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
         * Check to see if this is a submenu.
         * @return true if this is a submenu.
         **/
        bool hasParent() const;

     private:
        std::vector< boost::shared_ptr<MenuItem> > items_;
        std::vector< boost::shared_ptr<MenuItem> >::iterator active_;  // the active item in this menu
        boost::weak_ptr<Menu> level_;
        boost::weak_ptr<Menu> parent_;  // if this is a submenu it will have a parent menu
    };

};  // namespace v3d::ui::component
