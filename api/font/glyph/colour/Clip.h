/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <api/font/glyph/curve/Quadratic.h>

#include <vector>

namespace v3d::font::glyph::colour {

/**
 * A closed outline a layer is drawn inside, in em units.
 **/
struct Clip final {
    std::vector<curve::Quadratic> curves;
    bool evenOdd = false;  /**< whether it is filled by the even-odd rule rather than the nonzero rule **/
};

};  // namespace v3d::font::glyph::colour
