/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include "../Component.h"

#include <string>
#include <utility>


namespace v3d::ui::component {
    /**
     * A vGUI Button
     */
    class Button : public Component {
    public:
        explicit Button();

        /**
            * button state enumeration
            */
        typedef enum {
            STATE_NORMAL,   /**< Normal Button State **/
            STATE_HOVER,    /**< Mouse is hovering over button **/
            STATE_PRESS,    /**< Button is being clicked **/
            STATE_INACTIVE  /**< unclickable/insensitive state **/
        } ButtonState;

        /**
          * Set the button's label
          * @param str the new label text
          */
        void label(const std::string& str);
        /**
         * Get the button's label
         * @return the current label text
         */
        std::string_view label() const;
        /**
         * Get the current button state
         * @return current state
         */
        ButtonState state() const;
        /**
         * Set the current button state
         * @param s the new state
         */
        void state(ButtonState s);

    private:
        std::string label_;
        ButtonState state_;
        std::pair<std::string, std::string> command_;  // <scope, command>
    };

};  // end namespace Luxa
