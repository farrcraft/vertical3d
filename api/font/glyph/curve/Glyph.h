/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <cstdint>
#include <vector>

#include "Quadratic.h"

#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

/**
 * Glyph outlines as quadratic curves, divided into bands, and the store that keeps them for text
 * drawn at any size.
 **/
namespace v3d::font::glyph::curve {

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
struct Glyph final {
    /**
     * How far a band reaches into the bands either side of it, in em units.
     **/
    static constexpr float BAND_OVERLAP = 1.0F / 1024.0F;

    glm::vec2 min{ 0.0F };                          /**< the outline's lower left, in em units **/
    glm::vec2 max{ 0.0F };                          /**< and its upper right **/
    std::vector<Quadratic> curves;
    std::vector<std::vector<std::uint32_t>> horizontal;  /**< each band's curves, bottom to top **/
    std::vector<std::vector<std::uint32_t>> vertical;    /**< each band's curves, left to right **/

    /**
     * @return the scale and offset that turn a point in em units into the band it is in: x and y
     *         scale, z and w offset. Each axis is clamped to its bands by the reader
     **/
    glm::vec4 bandTransform() const;
};

};  // namespace v3d::font::glyph::curve
