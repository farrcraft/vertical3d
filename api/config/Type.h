/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <string_view>

namespace v3d::config {
enum class Type {
    Unknown,
    Window,
    Binding,
    Ui,
    Sound,
    Camera,
    Layout,
    Sprite,
};

constexpr Type stringToType(const std::string_view& typeName) {
    if (typeName == "window") {
        return Type::Window;
    }
    if (typeName == "binding") {
        return Type::Binding;
    }
    if (typeName == "ui") {
        return Type::Ui;
    }
    if (typeName == "sound") {
        return Type::Sound;
    }
    if (typeName == "camera") {
        return Type::Camera;
    }
    if (typeName == "layout") {
        return Type::Layout;
    }
    if (typeName == "sprite") {
        return Type::Sprite;
    }
    return  Type::Unknown;
}

/**
 * The name a config entry gives a type, which is what Config files the document under -
 * the inverse of stringToType, and empty for Unknown.
 **/
constexpr std::string_view typeName(Type type) {
    switch (type) {
        case Type::Window: return "window";
        case Type::Binding: return "binding";
        case Type::Ui: return "ui";
        case Type::Sound: return "sound";
        case Type::Camera: return "camera";
        case Type::Layout: return "layout";
        case Type::Sprite: return "sprite";
        case Type::Unknown: break;
    }
    return std::string_view();
}
};  // namespace v3d::config
