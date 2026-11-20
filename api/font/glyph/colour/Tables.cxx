/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include "Tables.h"

namespace v3d::font::glyph::colour {

bool Tables::any() const noexcept {
    return layers || paint || svg || cbdt || sbix;
}

};  // namespace v3d::font::glyph::colour
