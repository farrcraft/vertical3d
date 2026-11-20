/*
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <string>

namespace v3d::ui::style {

/**
 * What a style applies to: a component's own style name and its class, such as "button".
 */
struct Selector {
    std::string name;
    std::string className;
};

};  // namespace v3d::ui::style
