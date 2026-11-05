/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/ui/style/Style.h>

#include <string>

namespace v3d::ui::style {

/**
 * A style of the "menu" class, under the name it is built with.
 **/
class Menu : public Style {
 public:
    explicit Menu(const std::string & str);
    ~Menu();

 private:
};


};  // end namespace v3d::ui::style
