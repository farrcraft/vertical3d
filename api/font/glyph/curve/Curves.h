/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <cstdint>
#include <vector>

#include "Glyph.h"
#include "Quadratic.h"

namespace v3d::font::face {

class Face;

};  // namespace v3d::font::face

namespace v3d::font::glyph::curve {

/**
 * Divide closed outlines into bands, each axis into about the square root of the number of
 * curves, from 1 to 16 bands.
 *
 * @param curves closed contours in em units, filled by the nonzero rule
 * @return the outline banded, or one with no curves and no bands for none
 **/
Glyph banded(std::vector<Quadratic> curves);

/**
 * Read a glyph's outline as curves, unscaled and unhinted, at a variable font's normalized
 * coordinates. A cubic curve, as a CFF face holds, is split into four quadratics, each with its
 * control point where the cubic's two would put it.
 *
 * Each axis is divided into about the square root of the number of curves, from 1 to 16 bands.
 *
 * @return the outline, or one with no curves for a glyph with none, such as a space, or one that
 *         cannot be loaded or set to the coordinates
 **/
Glyph outline(const face::Face& face, unsigned int index, const std::vector<std::int32_t>& coordinates = {});

};  // namespace v3d::font::glyph::curve
