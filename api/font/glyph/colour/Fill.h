/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <cstdint>
#include <vector>

#include "Stop.h"

#include <glm/mat3x3.hpp>
#include <glm/vec2.hpp>

namespace v3d::font::glyph::colour {

/**
 * What a gradient does past its first and last stops.
 **/
enum class Extend : std::uint8_t {
    Pad,     /**< the end colours carry on **/
    Repeat,  /**< the stops repeat **/
    Reflect  /**< the stops repeat, every other time reversed **/
};

/**
 * What fills a layer's clips: one colour, or a gradient.
 *
 * A gradient's points are in its own space, which its transform maps into em units. A linear
 * gradient runs from one point to another. A radial gradient runs from one circle to another,
 * and a sweep gradient around a centre, counter-clockwise from one angle to another.
 **/
struct Fill final {
    enum class Kind : std::uint8_t {
        Solid,
        Linear,
        Radial,
        Sweep
    };

    Kind kind = Kind::Solid;
    std::vector<Stop> stops{}; /**< in order of offset. A solid fill has one **/
    Extend extend = Extend::Pad;
    glm::vec2 from{ 0.0F };          /**< a linear gradient's start, a radial gradient's first centre, or a sweep's centre **/
    glm::vec2 to{ 0.0F };            /**< a linear gradient's end, or a radial gradient's second centre **/
    float fromRadius = 0.0F;
    float toRadius = 0.0F;
    float startAngle = 0.0F;         /**< a sweep's, in radians **/
    float endAngle = 0.0F;
    glm::mat3 transform{ 1.0F };     /**< from the gradient's space into em units **/
};

};  // namespace v3d::font::glyph::colour
