/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <string_view>

namespace v3d::ui::menu {
    enum class ItemType {
        Unknown,
        Action,
        Submenu,
        Input,
        NumericInput,
        KeyInput
    };

    constexpr ItemType stringToType(const std::string_view& typeName) {
        if (typeName == "action") {
            return ItemType::Action;
        } else if (typeName == "submenu") {
            return ItemType::Submenu;
        } else if (typeName == "input") {
            return ItemType::Input;
        } else if (typeName == "numeric_input") {
            return ItemType::NumericInput;
        } else if (typeName == "key_input") {
            return ItemType::KeyInput;
        }
        return  ItemType::Unknown;
    }
};  // namespace v3d::ui::menu
