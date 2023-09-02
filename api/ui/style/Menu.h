/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <string>

#include "../Style.h"

namespace v3d::ui::style {

    class Menu : public Style {
     public:
        explicit Menu(const std::string & str);
        ~Menu();

     private:
    };


};  // end namespace v3d::ui::style
