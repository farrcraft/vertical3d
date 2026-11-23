/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <string>

namespace v3d::font::face {

/**
 * One axis of a variable font, in the units its values are given in.
 **/
struct Axis final {
    std::string tag;           /**< its four letter name, such as "wght" **/
    float min = 0.0F;
    float initial = 0.0F;      /**< the value the face is drawn at when none is given **/
    float max = 0.0F;
};

};  // namespace v3d::font::face
