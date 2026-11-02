/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <cstdint>
#include <vector>

#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

namespace v3d::font {

class Face;

/**
 * A quadratic Bézier curve from p1 to p3, pulled towards p2. A straight line has p2 on p3.
 **/
struct QuadraticCurve final {
    glm::vec2 p1{ 0.0F };
    glm::vec2 p2{ 0.0F };
    glm::vec2 p3{ 0.0F };
};

/**
 * A glyph's outline as quadratic curves in em units, with y upwards, divided into bands so that
 * a pixel reads only the curves near it.
 *
 * A horizontal band is a strip of the glyph's height, and holds every curve whose height reaches
 * into it, except a horizontal line, which no horizontal ray crosses. Its curves are sorted from
 * the largest x to the smallest, so a ray cast to the right can stop at the first curve wholly to
 * its left. A vertical band holds the curves reaching into a strip of the glyph's width, except a
 * vertical line, sorted from the largest y. Bands overlap their neighbours by BAND_OVERLAP, so a
 * curve on the line between two is in both.
 *
 * The outline is closed, contour by contour, and filled by the nonzero rule.
 **/
struct CurveGlyph final {
    /**
     * How far a band reaches into the bands either side of it, in em units.
     **/
    static constexpr float BAND_OVERLAP = 1.0F / 1024.0F;

    glm::vec2 min{ 0.0F };                          /**< the outline's lower left, in em units **/
    glm::vec2 max{ 0.0F };                          /**< and its upper right **/
    std::vector<QuadraticCurve> curves;
    std::vector<std::vector<std::uint32_t>> horizontal;  /**< each band's curves, bottom to top **/
    std::vector<std::vector<std::uint32_t>> vertical;    /**< each band's curves, left to right **/

    /**
     * @return the scale and offset that turn a point in em units into the band it is in: x and y
     *         scale, z and w offset. Each axis is clamped to its bands by the reader
     **/
    glm::vec4 bandTransform() const;
};

/**
 * Divide closed outlines into bands, each axis into about the square root of the number of
 * curves, from 1 to 16 bands.
 *
 * @param curves closed contours in em units, filled by the nonzero rule
 * @return the outline banded, or one with no curves and no bands for none
 **/
CurveGlyph banded(std::vector<QuadraticCurve> curves);

/**
 * Read a glyph's outline as curves, unscaled and unhinted. A cubic curve, as a CFF face holds, is
 * split into four quadratics, each with its control point where the cubic's two would put it.
 *
 * Each axis is divided into about the square root of the number of curves, from 1 to 16 bands.
 *
 * @return the outline, or one with no curves for a glyph with none, such as a space, or one that
 *         cannot be loaded
 **/
CurveGlyph outline(const Face& face, unsigned int index);

};  // namespace v3d::font
