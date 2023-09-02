/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <string>

#include "../Component.h"

namespace v3d::ui::component {

    /**
     * A vGUI Label
     **/
    class Label : public Component {
     public:
        Label();
        ~Label();

        /**
         * Set the label text
         * @param txt the new text value
         */
        void text(const std::string& txt);
        /**
         * Get the current label text
         * @return the current text value
         */
        std::string_view text(void) const;

     private:
        std::string text_;
        //  std::string _font;
    };

};  // namespace v3d::ui::component
