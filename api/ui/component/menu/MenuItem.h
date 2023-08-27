/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include "../../Component.h"

#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>

#include <string>

namespace v3d::ui {
    class Menu;

    /**
     * A single menu item. It consists of a label and points to an optional submenu.
     * There are several different types of menu items:
     * - action - activating an action menu will dispatch the bound action (scope::command param)
     * - submenu - activating this type of menu item will replace the currently active menu with the submenu
     * - input - this type of menu item is the same as an action item type except that it accepts a single input 
     *		when activated before it dispatches the action. the input value is used as the param in this case.
     * - numeric_input - the same as an input type except restricted to number input types only
     * - key_input - an input type that uses a key name as the param
     *
     * When using the input type items, input is terminated by the ui::selectMenu key binding.
     * Also for input type items, the param value is displayed within the menu item text along with the label
     * in the form 'label : param'. 
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
        MenuItem(const std::string & type, const std::string & label, 
                    const std::string & cmd, const std::string & scope, const std::string & param);

        /**
          * Draw the menu item
          */
        void draw() const;
        /**
          * Set the menu item text label 
          * @param str the new label text
          */
        void label(const std::string & str);
        /**
          * Set the submenu
          * @param sub the new sub menu
          */
        void submenu(boost::shared_ptr<Menu> sub);
        /**
          * Set the parent owner menu
          * @param m the new parent
          */
        void menu(boost::weak_ptr<Menu> m);
        /**
          * Get the current menu item label text
          * @return the label text
          */
        std::string label() const;
        /**
          * Get the submenu
          * @return the child menu
          */
        boost::shared_ptr<Menu> submenu() const;
        /**
          * Get the command name bound to the menu item
          * @return the command name
          */
        std::string command() const;
        /**
          * Get the owning menu this item belongs to.
          * @return a pointer to the parent menu
          **/
        boost::weak_ptr<Menu> menu();

        std::string scope() const;
        std::string param() const;
        std::string type() const;

     private:
        std::string label_;
        boost::shared_ptr<Menu> submenu_;
        boost::weak_ptr<Menu> menu_; // owning menu
        std::string command_;
        std::string scope_;
        std::string param_;
        std::string type_;
    };

};  // namespace v3d::ui
