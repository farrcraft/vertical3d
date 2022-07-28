/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#include <string_view>

namespace v3d::config {
    enum class Type {
        Unknown,
        Window,
        Binding,
        Ui,
        Sound,
    };

    constexpr Type stringToType(const std::string_view& typeName) {
        if (typeName == "window") {
            return Type::Window;
        } else if (typeName == "binding") {
            return Type::Binding;
        } else if (typeName == "ui") {
            return Type::Ui;
        } else if (typeName == "sound") {
            return Type::Sound;
        }
        return  Type::Unknown;
    }
};  // namespace v3d::config
