/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <cstdint>
#include <string>

namespace v3d::editor::script {

/**
 * What a parameter of a command takes. An integer is one an int holds, and a frame is one. An
 * item is named by its name, and a part by the text form Part describes.
 **/
enum class Type : std::uint8_t { Boolean, Number, Integer, Text, Item, Items, Parts };

/**
 * One argument a command takes from a script, by name. Every parameter may be left out, and the
 * command then fills it from the editor's state, as it does for a menu or a key.
 **/
struct Parameter final {
    std::string name;
    Type type = Type::Boolean;
};

};  // namespace v3d::editor::script
