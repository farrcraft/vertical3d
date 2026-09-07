/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <string_view>

namespace v3d::ui::component::menu {
enum class ItemType {
    Unknown,
    Action,
    Submenu,
    Check,
    Radio,
    Input,
    NumericInput,
    KeyInput
};

constexpr ItemType stringToType(const std::string_view& typeName) {
    if (typeName == "action") {
        return ItemType::Action;
    }
    if (typeName == "submenu") {
        return ItemType::Submenu;
    }
    if (typeName == "check") {
        return ItemType::Check;
    }
    if (typeName == "radio") {
        return ItemType::Radio;
    }
    if (typeName == "input") {
        return ItemType::Input;
    }
    if (typeName == "numeric_input") {
        return ItemType::NumericInput;
    }
    if (typeName == "key_input") {
        return ItemType::KeyInput;
    }
    return  ItemType::Unknown;
}
};  // namespace v3d::ui::component::menu
