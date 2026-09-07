/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "TabPage.h"

#include <string>

namespace v3d::ui::component {

TabPage::TabPage() :
    Component(Type::TAB_PAGE) {
}

void TabPage::label(const std::string& str) {
    label_ = str;
}

std::string_view TabPage::label() const {
    return label_;
}

};  // namespace v3d::ui::component
