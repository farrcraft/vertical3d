/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "TextStyle.h"

namespace v3d::font {

bool TextStyle::shapesLike(const TextStyle& other) const {
    return fonts == other.fonts && face == other.face && size == other.size && features == other.features &&
        variations == other.variations && language == other.language;
}

};  // namespace v3d::font
